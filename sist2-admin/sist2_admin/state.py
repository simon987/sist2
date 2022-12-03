from typing import Dict

from hexlib.db import Table
import pickle

from tesseract import get_tesseract_langs

RUNNING_FRONTENDS: Dict[str, int] = {}

TESSERACT_LANGS = get_tesseract_langs()

DB_SCHEMA_VERSION = "1"

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

