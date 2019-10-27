#include "cli.h"

#define DEFAULT_OUTPUT "index.sist2/"
#define DEFAULT_CONTENT_SIZE 4096
#define DEFAULT_QUALITY 15
#define DEFAULT_SIZE 200
#define DEFAULT_REWRITE_URL ""

#define DEFAULT_ES_URL "http://localhost:9200"

#define DEFAULT_BIND_ADDR "localhost"
#define DEFAULT_PORT "4090"


scan_args_t *scan_args_create() {
    scan_args_t *args = calloc(sizeof(scan_args_t), 1);
    return args;
}

int scan_args_validate(scan_args_t *args, int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Required positional argument: PATH.\n");
        return 1;
    }

    char *abs_path = abspath(argv[1]);
    if (abs_path == NULL) {
        fprintf(stderr, "File not found: %s", argv[1]);
        return 1;
    } else {
        args->path = abs_path;
    }

    if (args->incremental != NULL) {
        abs_path = abspath(args->incremental);
        if (abs_path == NULL) {
            fprintf(stderr, "File not found: %s", args->incremental);
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
    } else if (args->size <= 0) {
        fprintf(stderr, "Invalid size: %d\n", args->size);
        return 1;
    }

    if (args->content_size == 0) {
        args->content_size = DEFAULT_CONTENT_SIZE;
    } else if (args->content_size <= 0) {
        fprintf(stderr, "Invalid content-size: %d\n", args->content_size);
        return 1;
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

    if (args->name == NULL) {
        args->name = g_path_get_basename(args->output);
    }

    if (args->rewrite_url == NULL) {
        args->rewrite_url = DEFAULT_REWRITE_URL;
    }
    return 0;
}

#ifndef SIST_SCAN_ONLY
int index_args_validate(index_args_t *args, int argc, const char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Required positional argument: PATH.\n");
        return 1;
    }

    char *index_path = abspath(argv[1]);
    if (index_path == NULL) {
        fprintf(stderr, "File not found: %s", argv[1]);
        return 1;
    } else {
        args->index_path = argv[1];
    }

    if (args->es_url == NULL) {
        args->es_url = DEFAULT_ES_URL;
    }
    return 0;
}

int web_args_validate(web_args_t *args, int argc, const char **argv) {

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

    args->index_count = argc - 1;
    args->indices = argv + 1;

    for (int i = 0; i < args->index_count; i++) {
        char *abs_path = abspath(args->indices[i]);
        if (abs_path == NULL) {
            fprintf(stderr, "File not found: %s", abs_path);
            return 1;
        }
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
#endif

