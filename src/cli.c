#include "cli.h"
#include "ctx.h"

#include <tesseract/capi.h>

#define DEFAULT_OUTPUT "index.sist2/"
#define DEFAULT_CONTENT_SIZE 32768
#define DEFAULT_QUALITY 5
#define DEFAULT_SIZE 500
#define DEFAULT_REWRITE_URL ""

#define DEFAULT_ES_URL "http://localhost:9200"
#define DEFAULT_BATCH_SIZE 100

#define DEFAULT_BIND_ADDR "localhost"
#define DEFAULT_PORT "4090"


scan_args_t *scan_args_create() {
    scan_args_t *args = calloc(sizeof(scan_args_t), 1);

    args->depth = -1;

    return args;
}

void scan_args_destroy(scan_args_t *args) {
    if (args->name != NULL) {
        free(args->name);
    }
    if (args->path != NULL) {
        free(args->path);
    }
    if (args->output != NULL) {
        free(args->output);
    }
    free(args);
}

void index_args_destroy(index_args_t *args) {
    //todo
    free(args);
}

void web_args_destroy(web_args_t *args) {
    //todo
    free(args);
}

int scan_args_validate(scan_args_t *args, int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Required positional argument: PATH.\n");
        return 1;
    }

    char *abs_path = abspath(argv[1]);
    if (abs_path == NULL) {
        fprintf(stderr, "File not found: %s\n", argv[1]);
        return 1;
    } else {
        args->path = abs_path;
    }

    if (args->incremental != NULL) {
        abs_path = abspath(args->incremental);
        if (abs_path == NULL) {
            fprintf(stderr, "File not found: %s\n", args->incremental);
            return 1;
        }
    }

    if (args->quality == 0) {
        args->quality = DEFAULT_QUALITY;
    } else if (args->quality < 1 || args->quality > 31) {
        fprintf(stderr, "Invalid quality: %f\n", args->quality);
        return 1;
    }

    if (args->size == 0) {
        args->size = DEFAULT_SIZE;
    } else if (args->size > 0 && args->size < 32) {
        printf("Invalid size: %d\n", args->content_size);
        return 1;
    }

    if (args->content_size == 0) {
        args->content_size = DEFAULT_CONTENT_SIZE;
    }

    if (args->threads == 0) {
        args->threads = 1;
    } else if (args->threads < 0) {
        fprintf(stderr, "Invalid threads: %d\n", args->threads);
        return 1;
    }

    if (args->output == NULL) {
        args->output = malloc(strlen(DEFAULT_OUTPUT) + 1);
        strcpy(args->output, DEFAULT_OUTPUT);
    } else {
        args->output = expandpath(args->output);
    }

    int ret = mkdir(args->output, S_IRUSR | S_IWUSR | S_IXUSR);
    if (ret != 0) {
        fprintf(stderr, "Invalid output: '%s' (%s).\n", args->output, strerror(errno));
        return 1;
    }

    if (args->depth < 0) {
        args->depth = G_MAXINT32;
    } else {
        args->depth += 1;
    }

    if (args->name == NULL) {
        args->name = g_path_get_basename(args->output);
    }

    if (args->rewrite_url == NULL) {
        args->rewrite_url = DEFAULT_REWRITE_URL;
    }

    if (args->archive == NULL || strcmp(args->archive, "recurse") == 0) {
        args->archive_mode = ARC_MODE_RECURSE;
    } else if (strcmp(args->archive, "list") == 0) {
        args->archive_mode = ARC_MODE_LIST;
    } else if (strcmp(args->archive, "shallow") == 0) {
        args->archive_mode = ARC_MODE_SHALLOW;
    } else if (strcmp(args->archive, "skip") == 0) {
        args->archive_mode = ARC_MODE_SKIP;
    } else {
        fprintf(stderr, "Archive mode must be one of (skip, list, shallow, recurse), got '%s'", args->archive);
        return 1;
    }

    if (args->tesseract_lang != NULL) {
        TessBaseAPI *api = TessBaseAPICreate();
        ret = TessBaseAPIInit3(api, TESS_DATAPATH, args->tesseract_lang);
        if (ret != 0) {
            fprintf(stderr, "Could not initialize tesseract with lang '%s'\n", args->tesseract_lang);
            return 1;
        }
        TessBaseAPIEnd(api);
        TessBaseAPIDelete(api);
    }

    LOG_DEBUGF("cli.c", "arg quality=%f", args->quality)
    LOG_DEBUGF("cli.c", "arg size=%d", args->size)
    LOG_DEBUGF("cli.c", "arg content_size=%d", args->content_size)
    LOG_DEBUGF("cli.c", "arg threads=%d", args->threads)
    LOG_DEBUGF("cli.c", "arg incremental=%s", args->incremental)
    LOG_DEBUGF("cli.c", "arg output=%s", args->output)
    LOG_DEBUGF("cli.c", "arg rewrite_url=%s", args->rewrite_url)
    LOG_DEBUGF("cli.c", "arg name=%s", args->name)
    LOG_DEBUGF("cli.c", "arg depth=%d", args->depth)
    LOG_DEBUGF("cli.c", "arg path=%s", args->path)
    LOG_DEBUGF("cli.c", "arg archive=%s", args->archive)
    LOG_DEBUGF("cli.c", "arg ocr=%s", args->tesseract_lang)

    return 0;
}

int index_args_validate(index_args_t *args, int argc, const char **argv) {

    LogCtx.verbose = 1;

    if (argc < 2) {
        fprintf(stderr, "Required positional argument: PATH.\n");
        return 1;
    }

    char *index_path = abspath(argv[1]);
    if (index_path == NULL) {
        fprintf(stderr, "File not found: %s\n", argv[1]);
        return 1;
    } else {
        args->index_path = argv[1];
        free(index_path);
    }

    if (args->es_url == NULL) {
        args->es_url = DEFAULT_ES_URL;
    }

    if (args->script_path != NULL) {
        struct stat info;
        int res = stat(args->script_path, &info);

        if (res == -1) {
            fprintf(stderr, "Error opening script file '%s': %s\n", args->script_path, strerror(errno));
            return 1;
        }

        int fd = open(args->script_path, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Error opening script file '%s': %s\n", args->script_path, strerror(errno));
            return 1;
        }

        args->script = malloc(info.st_size + 1);
        res = read(fd, args->script, info.st_size);
        if (res == -1) {
            fprintf(stderr, "Error reading script file '%s': %s\n", args->script_path, strerror(errno));
            return 1;
        }

        *(args->script + info.st_size) = '\0';
        close(fd);
    }

    if (args->batch_size == 0) {
        args->batch_size = DEFAULT_BATCH_SIZE;
    }

    LOG_DEBUGF("cli.c", "arg es_url=%s", args->es_url)
    LOG_DEBUGF("cli.c", "arg index_path=%s", args->index_path)
    LOG_DEBUGF("cli.c", "arg script_path=%s", args->script_path)
    LOG_DEBUGF("cli.c", "arg script=%s", args->script)
    LOG_DEBUGF("cli.c", "arg print=%d", args->print)
    LOG_DEBUGF("cli.c", "arg batch_size=%d", args->batch_size)
    LOG_DEBUGF("cli.c", "arg force_reset=%d", args->force_reset)

    return 0;
}

int web_args_validate(web_args_t *args, int argc, const char **argv) {

    LogCtx.verbose = 1;

    if (argc < 2) {
        fprintf(stderr, "Required positional argument: PATH.\n");
        return 1;
    }

    if (args->es_url == NULL) {
        args->es_url = DEFAULT_ES_URL;
    }

    if (args->bind == NULL) {
        args->bind = DEFAULT_BIND_ADDR;
    }

    if (args->port == NULL) {
        args->port = DEFAULT_PORT;
    }

    if (args->credentials != NULL) {
        args->b64credentials = onion_base64_encode(args->credentials, (int) strlen(args->credentials));
        //Remove trailing newline
        *(args->b64credentials + strlen(args->b64credentials) - 1) = '\0';
    }

    args->index_count = argc - 1;
    args->indices = argv + 1;

    for (int i = 0; i < args->index_count; i++) {
        char *abs_path = abspath(args->indices[i]);
        if (abs_path == NULL) {
            fprintf(stderr, "File not found: %s\n", args->indices[i]);
            return 1;
        }
    }

    LOG_DEBUGF("cli.c", "arg es_url=%s", args->es_url)
    LOG_DEBUGF("cli.c", "arg bind=%s", args->bind)
    LOG_DEBUGF("cli.c", "arg port=%s", args->port)
    LOG_DEBUGF("cli.c", "arg credentials=%s", args->credentials)
    LOG_DEBUGF("cli.c", "arg b64credentials=%s", args->b64credentials)
    LOG_DEBUGF("cli.c", "arg index_count=%d", args->index_count)
    for (int i = 0; i < args->index_count; i++) {
        LOG_DEBUGF("cli.c", "arg indices[%d]=%s", i, args->indices[i])
    }

    return 0;
}

index_args_t *index_args_create() {
    index_args_t *args = calloc(sizeof(index_args_t), 1);
    return args;
}

web_args_t *web_args_create() {
    web_args_t *args = calloc(sizeof(web_args_t), 1);
    return args;
}

