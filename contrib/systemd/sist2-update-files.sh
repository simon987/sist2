#!/bin/bash
set -e
DATE=$(date +%Y_%m_%d)
CONTENT=/zpool/files
ORIG=/mnt/ssd/sist-index/files.idx
NEW=/mnt/ssd/sist-index/files_$DATE.idx
EXCLUDE='ZArchives|TorrentStore|TorrentDownload|624f0c59-1fef-44f6-95e9-7483296f2833|ubuntu-full-2021-12-07'
NAME=Files
#REWRITE_URL="http://localhost:33333/activate?collection=$NAME&path="
REWRITE_URL=""

sist2 scan \
  --threads 14 \
  --mem-throttle 32768 \
  --thumbnail-quality 2 \
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
echo ">>> Index complete"
