#!/bin/bash
set -e
DATE=$(date +%Y_%m_%d)
CONTENT=/zpool/nextcloud/v-yadli
ORIG=/mnt/ssd/sist-index/nextcloud.idx
NEW=/mnt/ssd/sist-index/nextcloud_$DATE.idx
EXCLUDE='Yatao|.*263418493\\/Image\\/.*'
NAME=NextCloud
# REWRITE_URL="http://localhost:33333/activate?collection=$NAME&path="
REWRITE_URL=""

sist2 scan \
  --threads 14 \
  --mem-throttle 32768 \
  --quality 1.0 \
  --name $NAME \
  --ocr-lang=eng+chi_sim \
  --ocr-ebooks \
  --ocr-images \
  --exclude=$EXCLUDE \
  --rewrite-url=$REWRITE_URL \
  --incremental=$ORIG \
  --output=$NEW \
  $CONTENT
echo ">>> Scan complete"
rm -rf $ORIG
mv $NEW $ORIG 

unset http_proxy
unset https_proxy
unset HTTP_PROXY
unset HTTPS_PROXY
sist2 index $ORIG --incremental-index
