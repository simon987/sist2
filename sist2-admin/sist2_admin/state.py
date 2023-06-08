from typing import Dict
import os
import shutil

from hexlib.db import Table, PersistentState
import pickle

from tesseract import get_tesseract_langs
import sqlite3
from config import LOG_FOLDER, logger
from sist2 import SearchBackendType, Sist2SearchBackend

RUNNING_FRONTENDS: Dict[str, int] = {}

TESSERACT_LANGS = get_tesseract_langs()

DB_SCHEMA_VERSION = "4"

from pydantic import BaseModel


def _serialize(item):
    if isinstance(item, BaseModel):
        return pickle.dumps(item)
    if isinstance(item, bytes):
        raise Exception("FIXME: bytes in PickleTable")
    return item


def _deserialize(item):
    if isinstance(item, bytes):
        return pickle.loads(item)
    return item


class PickleTable(Table):

    def __getitem__(self, item):
        row = super().__getitem__(item)
        if row:
            return dict((k, _deserialize(v)) for k, v in row.items())
        return row

    def __setitem__(self, key, value):
        value = dict((k, _serialize(v)) for k, v in value.items())
        super().__setitem__(key, value)

    def __iter__(self):
        for row in super().__iter__():
            yield dict((k, _deserialize(v)) for k, v in row.items())

    def sql(self, where_clause, *params):
        for row in super().sql(where_clause, *params):
            yield dict((k, _deserialize(v)) for k, v in row.items())


def get_log_files_to_remove(db: PersistentState, job_name: str, n: int):
    if n < 0:
        return []

    counter = 0
    to_remove = []

    for row in db["task_done"].sql("WHERE has_logs=1 ORDER BY started DESC"):
        if row["name"].endswith(f"[{job_name}]"):
            counter += 1

            if counter > n:
                to_remove.append(row)

    return to_remove


def delete_log_file(db: PersistentState, task_id: str):
    db["task_done"][task_id] = {
        "has_logs": 0
    }

    try:
        os.remove(os.path.join(LOG_FOLDER, f"sist2-{task_id}.log"))
    except:
        pass


def migrate_v1_to_v2(db: PersistentState):
    shutil.copy(db.dbfile, db.dbfile + "-before-migrate-v2.bak")

    # Frontends
    db._table_factory = PickleTable
    frontends = [row["frontend"] for row in db["frontends"]]
    del db["frontends"]

    db._table_factory = Table
    for frontend in frontends:
        db["frontends"][frontend.name] = frontend
    list(db["frontends"])

    # Jobs
    db._table_factory = PickleTable
    jobs = [row["job"] for row in db["jobs"]]
    del db["jobs"]

    db._table_factory = Table
    for job in jobs:
        db["jobs"][job.name] = job
    list(db["jobs"])

    db["sist2_admin"]["info"] = {
        "version": "2"
    }


def create_default_search_backends(db: PersistentState):
    es_backend = Sist2SearchBackend.create_default(name="elasticsearch",
                                                   backend_type=SearchBackendType("elasticsearch"))
    db["search_backends"]["elasticsearch"] = es_backend
    sqlite_backend = Sist2SearchBackend.create_default(name="sqlite", backend_type=SearchBackendType("sqlite"))
    db["search_backends"]["sqlite"] = sqlite_backend


def migrate_v3_to_v4(db: PersistentState):
    shutil.copy(db.dbfile, db.dbfile + "-before-migrate-v4.bak")

    create_default_search_backends(db)

    try:
        conn = sqlite3.connect(db.dbfile)
        conn.execute("ALTER TABLE task_done ADD COLUMN has_logs INTEGER DEFAULT 1")
        conn.commit()
        conn.close()
    except Exception as e:
        logger.exception(e)

    db["sist2_admin"]["info"] = {
        "version": "4"
    }
