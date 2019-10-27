#!/usr/bin/env bash

rm -rf index.sist2/

rm web/js/bundle.js 2> /dev/null
cat `ls -v web/js/*.min.js` > web/js/bundle.js
cat web/js/{util,dom,search}.js >> web/js/bundle.js

rm web/css/bundle.css 2> /dev/null
cat web/css/*.min.css > web/css/bundle.css
cat web/css/main.css >> web/css/bundle.css

python3 scripts/mime.py > src/parsing/mime_generated.c
python3 scripts/serve_static.py > src/web/static_generated.c
python3 scripts/index_static.py > src/index/static_generated.c
