import sqlite3
import orjson as json
import os
import string
from hashlib import md5
import random
from tqdm import tqdm

schema = """
CREATE TABLE thumbnail (
  id TEXT NOT NULL CHECK (
    length(id) = 32
  ), 
  num INTEGER NOT NULL, 
  data BLOB NOT NULL, 
  PRIMARY KEY(id, num)
) WITHOUT ROWID;
CREATE TABLE version (
  id INTEGER PRIMARY KEY AUTOINCREMENT, 
  date TEXT NOT NULL DEFAULT (CURRENT_TIMESTAMP)
);
CREATE TABLE document (
  id TEXT PRIMARY KEY NOT NULL CHECK (
    length(id) = 32
  ), 
  marked INTEGER NOT NULL DEFAULT (1), 
  version INTEGER NOT NULL REFERENCES version(id), 
  mtime INTEGER NOT NULL, 
  size INTEGER NOT NULL, 
  json_data TEXT NOT NULL CHECK (
    json_valid(json_data)
  )
);
CREATE TABLE delete_list (
  id TEXT PRIMARY KEY CHECK (
    length(id) = 32
  )
) WITHOUT ROWID;
CREATE TABLE tag (
  id TEXT NOT NULL, 
  tag TEXT NOT NULL, 
  PRIMARY KEY (id, tag)
);
CREATE TABLE document_sidecar (
  id TEXT PRIMARY KEY NOT NULL, json_data TEXT NOT NULL
) WITHOUT ROWID;
CREATE TABLE descriptor (
  id TEXT NOT NULL, version_major INTEGER NOT NULL, 
  version_minor INTEGER NOT NULL, version_patch INTEGER NOT NULL, 
  root TEXT NOT NULL, name TEXT NOT NULL, 
  rewrite_url TEXT, timestamp INTEGER NOT NULL
);
CREATE TABLE stats_treemap (
  path TEXT NOT NULL, size INTEGER NOT NULL
);
CREATE TABLE stats_size_agg (
  bucket INTEGER NOT NULL, count INTEGER NOT NULL
);
CREATE TABLE stats_date_agg (
  bucket INTEGER NOT NULL, count INTEGER NOT NULL
);
CREATE TABLE stats_mime_agg (
  mime TEXT NOT NULL, size INTEGER NOT NULL, 
  count INTEGER NOT NULL
);
CREATE TABLE embedding (
  id TEXT REFERENCES document(id), 
  model_id INTEGER NOT NULL references model(id), 
  start INTEGER NOT NULL, 
  end INTEGER, 
  embedding BLOB NOT NULL, 
  PRIMARY KEY (id, model_id, start)
);
CREATE TABLE model (
  id INTEGER PRIMARY KEY, 
  name TEXT NOT NULL UNIQUE CHECK (
    length(name) < 16
  ), 
  url TEXT, 
  path TEXT NOT NULL UNIQUE, 
  size INTEGER NOT NULL, 
  type TEXT NOT NULL CHECK (
    type IN ('flat', 'nested')
  )
);
"""

content = "".join(random.choices(string.ascii_letters, k=500))


def gen_document():
    return [
        md5(random.randbytes(8)).hexdigest(),
        json.dumps({
            "content": content,
            "mime": "image/jpeg",
            "extension": "jpeg",
            "name": "test",
            "path": "",
        })
    ]


if __name__ == "__main__":
    DB_NAME = "big_index.sist2"
    SIZE = 30_000_000

    os.remove(DB_NAME)
    db = sqlite3.connect(DB_NAME)
    db.executescript(schema)

    db.executescript("""
    PRAGMA journal_mode = OFF;
    PRAGMA synchronous = 0;
    """)

    for _ in tqdm(range(SIZE), total=SIZE):
        db.execute(
            "INSERT INTO document (id, version, mtime, size, json_data) VALUES (?, 1, 1000000, 10000, ?)",
            gen_document()
        )

    # 1. Enable rowid from document
    # 2. CREATE TABLE marked (
    #     id INTEGER PRIMARY KEY,
    #     marked int
    #    );
    # 3. Set FK for document_sidecar, embedding, tag, thumbnail
    # 4. Toggle FK if debug

    db.commit()
