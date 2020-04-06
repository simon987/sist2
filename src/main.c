#include "sist.h"
#include "ctx.h"

#include <third-party/argparse/argparse.h>
#include <glib.h>

#include "cli.h"
#include "io/serialize.h"
#include "io/store.h"
#include "tpool.h"
#include "io/walk.h"
#include "index/elastic.h"
#include "web/serve.h"
#include "parsing/mime.h"
#include "parsing/parse.h"

#define DESCRIPTION "Lightning-fast file system indexer and search tool."

#define EPILOG "Made by simon987 <me@simon987.net>. Released under GPL-3.0"


static const char *const Version = "2.0.0";
static const char *const usage[] = {
        "sist2 scan [OPTION]... PATH",
        "sist2 index [OPTION]... INDEX",
        "sist2 web [OPTION]... INDEX...",
        NULL,
};

void init_dir(const char *dirpath) {
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%sdescriptor.json", dirpath);

    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, ScanCtx.index.desc.uuid);
    time(&ScanCtx.index.desc.timestamp);
    strcpy(ScanCtx.index.desc.version, Version);
    strcpy(ScanCtx.index.desc.type, INDEX_TYPE_BIN);

    write_index_descriptor(path, &ScanCtx.index.desc);
}

void scan_print_header() {
    LOG_INFOF("main.c", "sist2 v%s", Version)
}

void _store(char *key, size_t key_len, char *buf, size_t buf_len) {
    store_write(ScanCtx.index.store, key, key_len, buf, buf_len);
}

void initialize_scan_context(scan_args_t *args) {

    // Arc
    ScanCtx.arc_ctx.mode = args->archive_mode;
    ScanCtx.arc_ctx.log = sist_log;
    ScanCtx.arc_ctx.logf = sist_logf;
    ScanCtx.arc_ctx.parse = (parse_callback_t) parse;

    // Cbr
    ScanCtx.cbr_ctx.log = sist_log;
    ScanCtx.cbr_ctx.logf = sist_logf;
    ScanCtx.cbr_ctx.store = _store;
    ScanCtx.cbr_ctx.cbr_mime = mime_get_mime_by_string(ScanCtx.mime_table, "application/x-cbr");

    // Ebook
    pthread_mutex_init(&ScanCtx.ebook_ctx.mupdf_mutex, NULL);
    ScanCtx.ebook_ctx.content_size = args->content_size;
    ScanCtx.ebook_ctx.tn_size = args->size;
    ScanCtx.ebook_ctx.tesseract_lang = args->tesseract_lang;
    ScanCtx.ebook_ctx.tesseract_path = args->tesseract_path;
    ScanCtx.ebook_ctx.log = sist_log;
    ScanCtx.ebook_ctx.logf = sist_logf;
    ScanCtx.ebook_ctx.store = _store;

    // Font
    ScanCtx.font_ctx.enable_tn = args->size > 0;
    ScanCtx.font_ctx.log = sist_log;
    ScanCtx.font_ctx.logf = sist_logf;
    ScanCtx.font_ctx.store = _store;

    // Media
    ScanCtx.media_ctx.tn_qscale = args->quality;
    ScanCtx.media_ctx.tn_size = args->size;
    ScanCtx.media_ctx.content_size = args->content_size;
    ScanCtx.media_ctx.log = sist_log;
    ScanCtx.media_ctx.logf = sist_logf;
    ScanCtx.media_ctx.store = _store;
    init_media();

    // OOXML
    ScanCtx.ooxml_ctx.content_size = args->content_size;
    ScanCtx.ooxml_ctx.log = sist_log;
    ScanCtx.ooxml_ctx.logf = sist_logf;

    ScanCtx.threads = args->threads;
    ScanCtx.depth = args->depth;

    strncpy(ScanCtx.index.path, args->output, sizeof(ScanCtx.index.path));
    strncpy(ScanCtx.index.desc.name, args->name, sizeof(ScanCtx.index.desc.name));
    strncpy(ScanCtx.index.desc.root, args->path, sizeof(ScanCtx.index.desc.root));
    strncpy(ScanCtx.index.desc.rewrite_url, args->rewrite_url, sizeof(ScanCtx.index.desc.rewrite_url));
    ScanCtx.index.desc.root_len = (short) strlen(ScanCtx.index.desc.root);
    ScanCtx.fast = args->fast;
}


void sist2_scan(scan_args_t *args) {

    ScanCtx.mime_table = mime_get_mime_table();
    ScanCtx.ext_table = mime_get_ext_table();

    initialize_scan_context(args);

    init_dir(ScanCtx.index.path);

    char store_path[PATH_MAX];
    snprintf(store_path, PATH_MAX, "%sthumbs", ScanCtx.index.path);
    mkdir(store_path, S_IWUSR | S_IRUSR | S_IXUSR);
    ScanCtx.index.store = store_create(store_path);

    scan_print_header();

    if (args->incremental != NULL) {
        ScanCtx.original_table = incremental_get_table();
        ScanCtx.copy_table = incremental_get_table();

        DIR *dir = opendir(args->incremental);
        if (dir == NULL) {
            LOG_FATALF("main.c", "Could not open original index for incremental scan: %s", strerror(errno))
        }

        char descriptor_path[PATH_MAX];
        snprintf(descriptor_path, PATH_MAX, "%s/descriptor.json", args->incremental);
        index_descriptor_t original_desc = read_index_descriptor(descriptor_path);

        if (strcmp(original_desc.version, Version) != 0) {
            LOG_FATALF("main.c", "Version mismatch! Index is %s but executable is %s/%s", original_desc.version,
                       Version, INDEX_VERSION_EXTERNAL)
        }

        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (strncmp(de->d_name, "_index_", sizeof("_index_") - 1) == 0) {
                char file_path[PATH_MAX];
                snprintf(file_path, PATH_MAX, "%s/%s", args->incremental, de->d_name);
                incremental_read(ScanCtx.original_table, file_path);
            }
        }
        closedir(dir);

        LOG_INFOF("main.c", "Loaded %d items in to mtime table.", g_hash_table_size(ScanCtx.original_table))
    }

    ScanCtx.pool = tpool_create(args->threads, thread_cleanup);
    tpool_start(ScanCtx.pool);
    walk_directory_tree(ScanCtx.index.desc.root);
    tpool_wait(ScanCtx.pool);
    tpool_destroy(ScanCtx.pool);

    if (args->incremental != NULL) {
        char dst_path[PATH_MAX];
        snprintf(store_path, PATH_MAX, "%sthumbs", args->incremental);
        snprintf(dst_path, PATH_MAX, "%s_index_original", ScanCtx.index.path);
        store_t *source = store_create(store_path);

        DIR *dir = opendir(args->incremental);
        if (dir == NULL) {
            perror("opendir");
            return;
        }
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (strncmp(de->d_name, "_index_", sizeof("_index_") - 1) == 0) {
                char file_path[PATH_MAX];
                snprintf(file_path, PATH_MAX, "%s/%s", args->incremental, de->d_name);
                incremental_copy(source, ScanCtx.index.store, file_path, dst_path, ScanCtx.copy_table);
            }
        }
        closedir(dir);
        store_destroy(source);
    }

    store_destroy(ScanCtx.index.store);
}

void sist2_index(index_args_t *args) {

    IndexCtx.es_url = args->es_url;
    IndexCtx.batch_size = args->batch_size;

    if (!args->print) {
        elastic_init(args->force_reset);
    }

    char descriptor_path[PATH_MAX];
    snprintf(descriptor_path, PATH_MAX, "%s/descriptor.json", args->index_path);

    index_descriptor_t desc = read_index_descriptor(descriptor_path);

    LOG_DEBUGF("main.c", "descriptor version %s (%s)", desc.version, desc.type)

    if (strcmp(desc.version, Version) != 0 && strcmp(desc.version, INDEX_VERSION_EXTERNAL) != 0) {
        LOG_FATALF("main.c", "Version mismatch! Index is %s but executable is %s/%s", desc.version, Version,
                   INDEX_VERSION_EXTERNAL)
    }

    DIR *dir = opendir(args->index_path);
    if (dir == NULL) {
        LOG_FATALF("main.c", "Could not open index %s: %s", args->index_path, strerror(errno))
    }

    index_func f;
    if (args->print) {
        f = print_json;
    } else {
        f = index_json;
    }

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strncmp(de->d_name, "_index_", sizeof("_index_") - 1) == 0) {
            char file_path[PATH_MAX];
            snprintf(file_path, PATH_MAX, "%s/%s", args->index_path, de->d_name);
            read_index(file_path, desc.uuid, desc.type, f);
        }
    }
    closedir(dir);

    if (!args->print) {
        elastic_flush();
        destroy_indexer(args->script, desc.uuid);
    }
}

void sist2_web(web_args_t *args) {

    WebCtx.es_url = args->es_url;
    WebCtx.index_count = args->index_count;
    WebCtx.b64credentials = args->b64credentials;

    for (int i = 0; i < args->index_count; i++) {
        char *abs_path = abspath(args->indices[i]);
        if (abs_path == NULL) {
            return;
        }
        char path_tmp[PATH_MAX];

        snprintf(path_tmp, PATH_MAX, "%sthumbs", abs_path);
        WebCtx.indices[i].store = store_create(path_tmp);

        snprintf(path_tmp, PATH_MAX, "%sdescriptor.json", abs_path);
        WebCtx.indices[i].desc = read_index_descriptor(path_tmp);

        strcpy(WebCtx.indices[i].path, abs_path);
        printf("Loaded index: %s\n", WebCtx.indices[i].desc.name);
        free(abs_path);
    }

    serve(args->bind, args->port);
}


int main(int argc, const char *argv[]) {

    scan_args_t *scan_args = scan_args_create();
    index_args_t *index_args = index_args_create();
    web_args_t *web_args = web_args_create();

    int arg_version = 0;

    char *common_es_url = NULL;

    struct argparse_option options[] = {
            OPT_HELP(),

            OPT_BOOLEAN('v', "version", &arg_version, "Show version and exit"),
            OPT_BOOLEAN(0, "verbose", &LogCtx.verbose, "Turn on logging"),
            OPT_BOOLEAN(0, "very-verbose", &LogCtx.very_verbose, "Turn on debug messages"),

            OPT_GROUP("Scan options"),
            OPT_INTEGER('t', "threads", &scan_args->threads, "Number of threads. DEFAULT=1"),
            OPT_FLOAT('q', "quality", &scan_args->quality,
                      "Thumbnail quality, on a scale of 1.0 to 31.0, 1.0 being the best. DEFAULT=5"),
            OPT_INTEGER(0, "size", &scan_args->size,
                        "Thumbnail size, in pixels. Use negative value to disable. DEFAULT=500"),
            OPT_INTEGER(0, "content-size", &scan_args->content_size,
                        "Number of bytes to be extracted from text documents. Use negative value to disable. DEFAULT=32768"),
            OPT_STRING(0, "incremental", &scan_args->incremental,
                       "Reuse an existing index and only scan modified files."),
            OPT_STRING('o', "output", &scan_args->output, "Output directory. DEFAULT=index.sist2/"),
            OPT_STRING(0, "rewrite-url", &scan_args->rewrite_url, "Serve files from this url instead of from disk."),
            OPT_STRING(0, "name", &scan_args->name, "Index display name. DEFAULT: (name of the directory)"),
            OPT_INTEGER(0, "depth", &scan_args->depth, "Scan up to DEPTH subdirectories deep. "
                                                       "Use 0 to only scan files in PATH. DEFAULT: -1"),
            OPT_STRING(0, "archive", &scan_args->archive, "Archive file mode (skip|list|shallow|recurse). "
                                                          "skip: Don't parse, list: only get file names as text, "
                                                          "shallow: Don't parse archives inside archives. DEFAULT: recurse"),
            OPT_STRING(0, "ocr", &scan_args->tesseract_lang, "Tesseract language (use tesseract --list-langs to see "
                                                             "which are installed on your machine)"),
            OPT_STRING('e', "exclude", &scan_args->exclude_regex, "Files that match this regex will not be scanned"),
            OPT_BOOLEAN(0, "fast", &scan_args->fast, "Only index file names & mime type"),

            OPT_GROUP("Index options"),
            OPT_STRING(0, "es-url", &common_es_url, "Elasticsearch url with port. DEFAULT=http://localhost:9200"),
            OPT_BOOLEAN('p', "print", &index_args->print, "Just print JSON documents to stdout."),
            OPT_STRING(0, "script-file", &index_args->script_path, "Path to user script."),
            OPT_INTEGER(0, "batch-size", &index_args->batch_size, "Index batch size. DEFAULT: 100"),
            OPT_BOOLEAN('f', "force-reset", &index_args->force_reset, "Reset Elasticsearch mappings and settings. "
                                                                      "(You must use this option the first time you use the index command)"),

            OPT_GROUP("Web options"),
            OPT_STRING(0, "es-url", &common_es_url, "Elasticsearch url. DEFAULT=http://localhost:9200"),
            OPT_STRING(0, "bind", &web_args->bind, "Listen on this address. DEFAULT=localhost"),
            OPT_STRING(0, "port", &web_args->port, "Listen on this port. DEFAULT=4090"),
            OPT_STRING(0, "auth", &web_args->credentials, "Basic auth in user:password format"),

            OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, DESCRIPTION, EPILOG);
    argc = argparse_parse(&argparse, argc, argv);

    if (arg_version) {
        printf(Version);
        goto end;
    }

    if (LogCtx.very_verbose != 0) {
        LogCtx.verbose = 1;
    }

    web_args->es_url = common_es_url;
    index_args->es_url = common_es_url;

    if (argc == 0) {
        argparse_usage(&argparse);
        goto end;
    } else if (strcmp(argv[0], "scan") == 0) {

        int err = scan_args_validate(scan_args, argc, argv);
        if (err != 0) {
            goto end;
        }
        sist2_scan(scan_args);

    } else if (strcmp(argv[0], "index") == 0) {

        int err = index_args_validate(index_args, argc, argv);
        if (err != 0) {
            goto end;
        }
        sist2_index(index_args);

    } else if (strcmp(argv[0], "web") == 0) {

        int err = web_args_validate(web_args, argc, argv);
        if (err != 0) {
            goto end;
        }
        sist2_web(web_args);

    } else {
        fprintf(stderr, "Invalid command: '%s'\n", argv[0]);
        argparse_usage(&argparse);
        goto end;
    }
    printf("\n");

    end:
    scan_args_destroy(scan_args);
    index_args_destroy(index_args);
    web_args_destroy(web_args);

    return 0;
}
