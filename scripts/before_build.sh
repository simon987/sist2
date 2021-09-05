#!/usr/bin/env bash

rm -rf index.sist2/

python3 scripts/mime.py > src/parsing/mime_generated.c
python3 scripts/serve_static.py > src/web/static_generated.c
python3 scripts/index_static.py > src/index/static_generated.c

printf "static const char *const Sist2CommitHash = \"%s\";\n" $(git rev-parse HEAD) > src/git_hash.h
printf "static const char *const LibScanCommitHash = \"%s\";\n" $(cd third-party/libscan/ && git rev-parse HEAD) >> src/git_hash.h
