import asyncio
import os
import signal
from datetime import datetime
from urllib.parse import urlparse

import requests
import uvicorn
from fastapi import FastAPI, HTTPException
from hexlib.db import PersistentState
from requests import ConnectionError
from requests.exceptions import SSLError
from starlette.middleware.cors import CORSMiddleware
from starlette.responses import RedirectResponse
from starlette.staticfiles import StaticFiles
from starlette.websockets import WebSocket
from websockets.exceptions import ConnectionClosed

import cron
from config import LOG_FOLDER, logger, WEBSERVER_PORT, DATA_FOLDER, SIST2_BINARY
from jobs import Sist2Job, Sist2ScanTask, TaskQueue, Sist2IndexTask, JobStatus
from notifications import Subscribe, Notifications
from sist2 import Sist2
from state import migrate_v1_to_v2, RUNNING_FRONTENDS, TESSERACT_LANGS, DB_SCHEMA_VERSION, migrate_v3_to_v4, \
    get_log_files_to_remove, delete_log_file
from web import Sist2Frontend

sist2 = Sist2(SIST2_BINARY, DATA_FOLDER)
db = PersistentState(dbfile=os.path.join(DATA_FOLDER, "state.db"))
notifications = Notifications()
task_queue = TaskQueue(sist2, db, notifications)

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_credentials=True,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.mount("/ui/", StaticFiles(directory="./frontend/dist", html=True), name="static")


@app.get("/")
async def home():
    return RedirectResponse("ui")


@app.get("/api")
async def api():
    return {
        "tesseract_langs": TESSERACT_LANGS,
        "logs_folder": LOG_FOLDER
    }


@app.get("/api/job/{name:str}")
async def get_job(name: str):
    job = db["jobs"][name]
    if not job:
        raise HTTPException(status_code=404)
    return job


@app.get("/api/frontend/{name:str}")
async def get_frontend(name: str):
    frontend = db["frontends"][name]
    frontend: Sist2Frontend
    if frontend:
        frontend.running = frontend.name in RUNNING_FRONTENDS
        return frontend
    raise HTTPException(status_code=404)


@app.get("/api/job/")
async def get_jobs():
    return list(db["jobs"])


@app.put("/api/job/{name:str}")
async def update_job(name: str, new_job: Sist2Job):
    new_job.last_modified = datetime.utcnow()
    job = db["jobs"][name]
    if not job:
        raise HTTPException(status_code=404)

    args_that_trigger_full_scan = [
        "path",
        "thumbnail_count",
        "thumbnail_quality",
        "thumbnail_size",
        "content_size",
        "depth",
        "archive",
        "archive_passphrase",
        "ocr_lang",
        "ocr_images",
        "ocr_ebooks",
        "fast",
        "checksums",
        "read_subtitles",
    ]
    for arg in args_that_trigger_full_scan:
        if getattr(new_job.scan_options, arg) != getattr(job.scan_options, arg):
            new_job.do_full_scan = True

    db["jobs"][name] = new_job


@app.put("/api/frontend/{name:str}")
async def update_frontend(name: str, frontend: Sist2Frontend):
    db["frontends"][name] = frontend

    # TODO: Check etag

    return "ok"


@app.get("/api/task/")
async def get_tasks():
    return list(map(lambda t: t.json(), task_queue.tasks()))


@app.get("/api/task/history")
async def task_history():
    return list(db["task_done"].sql("ORDER BY started DESC"))


@app.post("/api/task/{task_id:str}/kill")
async def kill_job(task_id: str):
    return task_queue.kill_task(task_id)


@app.post("/api/task/{task_id:str}/delete_logs")
async def delete_task_logs(task_id: str):
    if not db["task_done"][task_id]:
        raise HTTPException(status_code=404)

    delete_log_file(db, task_id)

    return "ok"


def _run_job(job: Sist2Job):
    job.last_modified = datetime.utcnow()
    if job.status == JobStatus("created"):
        job.status = JobStatus("started")
    db["jobs"][job.name] = job

    scan_task = Sist2ScanTask(job, f"Scan [{job.name}]")
    index_task = Sist2IndexTask(job, f"Index [{job.name}]", depends_on=scan_task)

    task_queue.submit(scan_task)
    task_queue.submit(index_task)


@app.get("/api/job/{name:str}/run")
async def run_job(name: str):
    job = db["jobs"][name]
    if not job:
        raise HTTPException(status_code=404)

    _run_job(job)

    return "ok"


@app.get("/api/job/{name:str}/logs_to_delete")
async def task_history(n: int, name: str):
    return get_log_files_to_remove(db, name, n)


@app.delete("/api/job/{name:str}")
async def delete_job(name: str):
    job = db["jobs"][name]
    if job:
        del db["jobs"][name]
    else:
        raise HTTPException(status_code=404)


@app.delete("/api/frontend/{name:str}")
async def delete_frontend(name: str):
    if name in RUNNING_FRONTENDS:
        os.kill(RUNNING_FRONTENDS[name], signal.SIGTERM)
        del RUNNING_FRONTENDS[name]

    frontend = db["frontends"][name]
    if frontend:
        del db["frontends"][name]
    else:
        raise HTTPException(status_code=404)


@app.post("/api/job/{name:str}")
async def create_job(name: str):
    if db["jobs"][name]:
        raise ValueError("Job with the same name already exists")

    job = Sist2Job.create_default(name)
    db["jobs"][name] = job

    return job


@app.post("/api/frontend/{name:str}")
async def create_frontend(name: str):
    if db["frontends"][name]:
        raise ValueError("Frontend with the same name already exists")

    frontend = Sist2Frontend.create_default(name)
    db["frontends"][name] = frontend

    return frontend


@app.get("/api/ping_es")
async def ping_es(url: str, insecure: bool):
    return check_es_version(url, insecure)


def check_es_version(es_url: str, insecure: bool):
    try:
        url = urlparse(es_url)
        if url.username:
            auth = (url.username, url.password)
            es_url = f"{url.scheme}://{url.hostname}:{url.port}"
        else:
            auth = None
        r = requests.get(es_url, verify=insecure, auth=auth)
    except SSLError:
        return {
            "ok": False,
            "message": "Invalid SSL certificate"
        }
    except ConnectionError as e:
        return {
            "ok": False,
            "message": "Connection refused"
        }
    except ValueError as e:
        return {
            "ok": False,
            "message": str(e)
        }

    if r.status_code == 401:
        return {
            "ok": False,
            "message": "Authentication failure"
        }

    try:
        return {
            "ok": True,
            "message": "Elasticsearch version " + r.json()["version"]["number"]
        }
    except:
        return {
            "ok": False,
            "message": "Could not read version"
        }


def start_frontend_(frontend: Sist2Frontend):
    frontend.web_options.indices = list(map(lambda j: db["jobs"][j].index_path, frontend.jobs))

    pid = sist2.web(frontend.web_options, frontend.name)
    RUNNING_FRONTENDS[frontend.name] = pid


@app.post("/api/frontend/{name:str}/start")
async def start_frontend(name: str):
    frontend = db["frontends"][name]
    if not frontend:
        raise HTTPException(status_code=404)

    start_frontend_(frontend)


@app.post("/api/frontend/{name:str}/stop")
async def stop_frontend(name: str):
    if name in RUNNING_FRONTENDS:
        os.kill(RUNNING_FRONTENDS[name], signal.SIGTERM)
        del RUNNING_FRONTENDS[name]


@app.get("/api/frontend/")
async def get_frontends():
    res = []
    for frontend in db["frontends"]:
        frontend: Sist2Frontend
        frontend.running = frontend.name in RUNNING_FRONTENDS
        res.append(frontend)
    return res


def tail(filepath: str, n: int):
    with open(filepath) as file:

        reached_eof = False
        buffer = []

        line = ""
        while True:
            tmp = file.readline()
            if tmp:
                line += tmp

                if line.endswith("\n"):

                    if reached_eof:
                        yield line
                    else:
                        if len(buffer) > n:
                            buffer.pop(0)
                        buffer.append(line)
                line = ""
            else:
                if not reached_eof:
                    reached_eof = True
                    yield from buffer
                yield None


@app.websocket("/notifications")
async def ws_tail_log(websocket: WebSocket):
    await websocket.accept()

    try:
        await websocket.receive_text()

        async with Subscribe(notifications) as ob:
            async for notification in ob.notifications():
                await websocket.send_json(notification)

    except ConnectionClosed:
        return


@app.websocket("/log/{task_id}")
async def ws_tail_log(websocket: WebSocket, task_id: str, n: int):
    log_file = os.path.join(LOG_FOLDER, f"sist2-{task_id}.log")

    await websocket.accept()

    try:
        await websocket.receive_text()
    except ConnectionClosed:
        return

    while True:
        for line in tail(log_file, n):

            try:
                if line:
                    await websocket.send_text(line)
                else:
                    await websocket.send_json({"ping": ""})
                    await asyncio.sleep(0.1)
            except ConnectionClosed:
                return


def main():
    uvicorn.run(app, port=WEBSERVER_PORT, host="0.0.0.0", timeout_graceful_shutdown=0)


def initialize_db():
    db["sist2_admin"]["info"] = {"version": DB_SCHEMA_VERSION}

    frontend = Sist2Frontend.create_default("default")
    db["frontends"]["default"] = frontend

    logger.info("Initialized database.")


def start_frontends():
    for frontend in db["frontends"]:
        frontend: Sist2Frontend
        if frontend.auto_start and len(frontend.jobs) > 0:
            start_frontend_(frontend)


if __name__ == '__main__':

    if not db["sist2_admin"]["info"]:
        initialize_db()
    if db["sist2_admin"]["info"]["version"] == "1":
        logger.info("Migrating to v2 database schema")
        migrate_v1_to_v2(db)
    if db["sist2_admin"]["info"]["version"] == "2":
        logger.error("Cannot migrate database from v2 to v3. Delete state.db to proceed.")
        exit(-1)
    if db["sist2_admin"]["info"]["version"] == "3":
        logger.info("Migrating to v4 database schema")
        migrate_v3_to_v4(db)

    start_frontends()
    cron.initialize(db, _run_job)

    logger.info("Started sist2-admin. Hello!")

    main()
