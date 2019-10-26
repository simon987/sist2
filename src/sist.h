#ifndef SIST_H
#define SIST_H

#define UUID_STR_LEN 37

#include <glib-2.0/glib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ftw.h>
#include <uuid.h>
#include <magic.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <ctype.h>
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include "argparse/argparse.h"
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <sys/stat.h>
#include <wordexp.h>
#include <onion/onion.h>
#include <onion/handler.h>
#include <onion/block.h>
#include <onion/shortcuts.h>
#include <curl/curl.h>


#include "cJSON/cJSON.h"

#include "types.h"
#include "tpool.h"
#include "util.h"
#include "src/index/elastic.h"
#include "io/store.h"
#include "io/serialize.h"
#include "io/walk.h"
#include "parsing/parse.h"
#include "parsing/mime.h"
#include "parsing/text.h"
#include "parsing/pdf.h"
#include "parsing/media.h"
#include "parsing/font.h"
#include "index/web.h"
#include "web/serve.h"
#include "cli.h"

;

#endif
