from threading import Thread

import pycron
import time

from hexlib.db import PersistentState

from config import logger
from jobs import Sist2Job


def _check_schedule(db: PersistentState, run_job):
    for job in db["jobs"]:
        job: Sist2Job

        if job.schedule_enabled:
            if pycron.is_now(job.cron_expression):
                logger.info(f"Submit scan task to queue for [{job.name}]")
                run_job(job)


def _cron_thread(db, run_job):
    time.sleep(60 - (time.time() % 60))
    start = time.time()

    while True:
        _check_schedule(db, run_job)
        time.sleep(60 - ((time.time() - start) % 60))


def initialize(db, run_job):
    t = Thread(target=_cron_thread, args=(db, run_job), daemon=True, name="timer")
    t.start()
