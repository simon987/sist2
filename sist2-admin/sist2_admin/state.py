from typing import Dict
import shutil

from hexlib.db import Table, PersistentState
import pickle

from tesseract import get_tesseract_langs

RUNNING_FRONTENDS: Dict[str, int] = {}

TESSERACT_LANGS = get_tesseract_langs()

DB_SCHEMA_VERSION = "3"

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
