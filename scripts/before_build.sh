#!/usr/bin/env bash

rm -rf index.sist2/

rm src/static/js/bundle.js 2> /dev/null
cat `ls src/static/js/*.min.js` > src/static/js/bundle.js
cat src/static/js/{util,dom,search}.js >> src/static/js/bundle.js

rm src/static/css/bundle*.css 2> /dev/null
cat src/static/css/*.min.css > src/static/css/bundle.css
cat src/static/css/light.css >> src/static/css/bundle.css
cat src/static/css/*.min.css > src/static/css/bundle_dark.css
cat src/static/css/dark.css >> src/static/css/bundle_dark.css

python3 scripts/mime.py > src/parsing/mime_generated.c
python3 scripts/serve_static.py > src/web/static_generated.c
python3 scripts/index_static.py > src/index/static_generated.c
