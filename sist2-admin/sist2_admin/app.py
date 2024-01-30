import asyncio
import os
import signal
from datetime import datetime
from time import sleep
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
from jobs import Sist2Job, Sist2ScanTask, TaskQueue, Sist2IndexTask, JobStatus, Sist2UserScriptTask
from notifications import Subscribe, Notifications
from sist2 import Sist2, Sist2SearchBackend
from state import migrate_v1_to_v2, RUNNING_FRONTENDS, TESSERACT_LANGS, DB_SCHEMA_VERSION, migrate_v3_to_v4, \
    get_log_files_to_remove, delete_log_file, create_default_search_backends
from web import Sist2Frontend
from script import UserScript, SCRIPT_TEMPLATES
from util import tail_sync, pid_is_running

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
        "logs_folder": LOG_FOLDER,
        "user_script_templates": list(SCRIPT_TEMPLATES.keys())
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


@app.get("/api/job")
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

    return "ok"


@app.get("/api/task")
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

    index_depends_on = scan_task
    script_tasks = []
    for script_name in job.user_scripts:
        script = db["user_scripts"][script_name]

        task = Sist2UserScriptTask(script, job, f"Script <{script_name}> [{job.name}]", depends_on=scan_task)
        script_tasks.append(task)
        index_depends_on = task

    index_task = Sist2IndexTask(job, f"Index [{job.name}]", depends_on=index_depends_on)

    task_queue.submit(scan_task)
    for task in script_tasks:
        task_queue.submit(task)
    task_queue.submit(index_task)


@app.get("/api/job/{name:str}/run")
async def run_job(name: str, full: bool = False):
    job: Sist2Job = db["jobs"][name]
    if not job:
        raise HTTPException(status_code=404)

    if full:
        job.do_full_scan = True

    _run_job(job)

    return "ok"


@app.get("/api/user_script/{name:str}/run")
def run_user_script(name: str, job: str):
    script = db["user_scripts"][name]
    if not script:
        raise HTTPException(status_code=404)
    job = db["jobs"][job]
    if not job:
        raise HTTPException(status_code=404)

    script_task = Sist2UserScriptTask(script, job, f"Script <{name}> [{job.name}]")

    task_queue.submit(script_task)

    return "ok"


@app.get("/api/job/{name:str}/logs_to_delete")
async def task_history(n: int, name: str):
    return get_log_files_to_remove(db, name, n)


@app.delete("/api/job/{name:str}")
async def delete_job(name: str):
    job: Sist2Job = db["jobs"][name]
    if not job:
        raise HTTPException(status_code=404)

    if any(name in frontend.jobs for frontend in db["frontends"]):
        raise HTTPException(status_code=400, detail="in use (frontend)")

    try:
        os.remove(job.previous_index)
    except:
        pass

    del db["jobs"][name]

    return "ok"


@app.delete("/api/frontend/{name:str}")
async def delete_frontend(name: str):
    if name in RUNNING_FRONTENDS:
        try:
            os.kill(RUNNING_FRONTENDS[name], signal.SIGTERM)
        except ProcessLookupError:
            pass
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
        r = requests.get(es_url, verify=not insecure, auth=auth)
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
    frontend.web_options.indices = [
        os.path.join(DATA_FOLDER, db["jobs"][j].index_path)
        for j in frontend.jobs
    ]

    backend_name = frontend.web_options.search_backend
    search_backend = db["search_backends"][backend_name]
    if search_backend is None:
        logger.error(
            f"Error while running task: search backend not found: {backend_name}")
        return -1

    logger.debug(f"Fetched search backend options for {backend_name}")

    pid = sist2.web(frontend.web_options, search_backend, frontend.name)

    sleep(0.2)
    if not pid_is_running(pid):
        frontend_log = frontend.get_log_path(LOG_FOLDER)
        logger.error(f"Frontend exited too quickly, check {frontend_log} for more details:")
        for line in tail_sync(frontend.get_log_path(LOG_FOLDER), 3):
            logger.error(line.strip())

        return False

    RUNNING_FRONTENDS[frontend.name] = pid
    return True


@app.post("/api/frontend/{name:str}/start")
async def start_frontend(name: str):
    frontend = db["frontends"][name]
    if not frontend:
        raise HTTPException(status_code=404)

    ok = start_frontend_(frontend)

    if not ok:
        raise HTTPException(status_code=500)

    return "ok"


@app.post("/api/frontend/{name:str}/stop")
async def stop_frontend(name: str):
    if name in RUNNING_FRONTENDS:
        os.kill(RUNNING_FRONTENDS[name], signal.SIGTERM)
        del RUNNING_FRONTENDS[name]


@app.get("/api/frontend")
async def get_frontends():
    res = []
    for frontend in db["frontends"]:
        frontend: Sist2Frontend
        frontend.running = frontend.name in RUNNING_FRONTENDS
        res.append(frontend)
    return res


@app.get("/api/search_backend")
async def get_search_backends():
    return list(db["search_backends"])


@app.put("/api/search_backend/{name:str}")
async def update_search_backend(name: str, backend: Sist2SearchBackend):
    if not db["search_backends"][name]:
        raise HTTPException(status_code=404)

    db["search_backends"][name] = backend
    return "ok"


@app.get("/api/search_backend/{name:str}")
def get_search_backend(name: str):
    backend = db["search_backends"][name]
    if not backend:
        raise HTTPException(status_code=404)

    return backend


@app.delete("/api/search_backend/{name:str}")
def delete_search_backend(name: str):
    backend: Sist2SearchBackend = db["search_backends"][name]
    if not backend:
        raise HTTPException(status_code=404)

    if any(frontend.web_options.search_backend == name for frontend in db["frontends"]):
        raise HTTPException(status_code=400, detail="in use (frontend)")

    if any(job.index_options.search_backend == name for job in db["jobs"]):
        raise HTTPException(status_code=400, detail="in use (job)")

    del db["search_backends"][name]

    try:
        os.remove(os.path.join(DATA_FOLDER, backend.search_index))
    except:
        pass

    return "ok"


@app.post("/api/search_backend/{name:str}")
def create_search_backend(name: str):
    if db["search_backends"][name] is not None:
        return HTTPException(status_code=400, detail="already exists")

    backend = Sist2SearchBackend.create_default(name)
    db["search_backends"][name] = backend

    return backend


@app.delete("/api/user_script/{name:str}")
def delete_user_script(name: str):
    if db["user_scripts"][name] is None:
        return HTTPException(status_code=404)

    if any(name in job.user_scripts for job in db["jobs"]):
        raise HTTPException(status_code=400, detail="in use (job)")

    script: UserScript = db["user_scripts"][name]
    script.delete_dir()

    del db["user_scripts"][name]

    return "ok"


@app.post("/api/user_script/{name:str}")
def create_user_script(name: str, template: str):
    if db["user_scripts"][name] is not None:
        return HTTPException(status_code=400, detail="already exists")

    script = SCRIPT_TEMPLATES[template](name)
    db["user_scripts"][name] = script

    return script


@app.get("/api/user_script")
async def get_user_scripts():
    return list(db["user_scripts"])


@app.get("/api/user_script/{name:str}")
async def get_user_script(name: str):
    backend = db["user_scripts"][name]
    if not backend:
        raise HTTPException(status_code=404)

    return backend


@app.put("/api/user_script/{name:str}")
async def update_user_script(name: str, script: UserScript):
    previous_version: UserScript = db["user_scripts"][name]

    if previous_version and previous_version.git_repository != script.git_repository:
        script.force_clone = True

    db["user_scripts"][name] = script

    return "ok"


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

    create_default_search_backends(db)

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

    if db["sist2_admin"]["info"]["version"] != DB_SCHEMA_VERSION:
        raise Exception(f"Incompatible database {db.dbfile}. "
                        f"Automatic migration is not available, please delete the database file to continue.")

    start_frontends()
    cron.initialize(db, _run_job)

    logger.info("Started sist2-admin. Hello!")

    main()
