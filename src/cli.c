#include "cli.h"
#include "ctx.h"
#include <tesseract/capi.h>

#define DEFAULT_OUTPUT "index.sist2/"
#define DEFAULT_CONTENT_SIZE 32768
#define DEFAULT_QUALITY 5
#define DEFAULT_SIZE 500
#define DEFAULT_REWRITE_URL ""

#define DEFAULT_ES_URL "http://localhost:9200"
#define DEFAULT_ES_INDEX "sist2"
#define DEFAULT_BATCH_SIZE 100

#define DEFAULT_LISTEN_ADDRESS "localhost:4090"
#define DEFAULT_TREEMAP_THRESHOLD 0.0005

#define DEFAULT_MAX_MEM_BUFFER 2000

const char *TESS_DATAPATHS[] = {
        "/usr/share/tessdata/",
        "/usr/share/tesseract-ocr/tessdata/",
        "./",
        NULL
};


scan_args_t *scan_args_create() {
    scan_args_t *args = calloc(sizeof(scan_args_t), 1);

    args->depth = -1;

    return args;
}

exec_args_t *exec_args_create() {
    exec_args_t *args = calloc(sizeof(exec_args_t), 1);
    return args;
}

void scan_args_destroy(scan_args_t *args) {
    if (args->name != NULL) {
        free(args->name);
    }
    if (args->incremental != NULL) {
        free(args->incremental);
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
    if (args->es_mappings_path) {
        free(args->es_mappings);
    }
    if (args->es_settings_path) {
        free(args->es_settings);
    }
    free(args);
}

void web_args_destroy(web_args_t *args) {
    //todo
    free(args);
}

void exec_args_destroy(exec_args_t *args) {
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
        args->incremental = abspath(args->incremental);
        if (abs_path == NULL) {
            sist_log("main.c", SIST_WARNING, "Could not open original index! Disabled incremental scan feature.");
            args->incremental = NULL;
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

    if (args->depth <= 0) {
        args->depth = G_MAXINT32;
    } else {
        args->depth += 1;
    }

    if (args->name == NULL) {
        args->name = g_path_get_basename(args->output);
    } else {
        char* tmp = malloc(strlen(args->name) + 1);
        strcpy(tmp, args->name);
        args->name = tmp;
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

        char filename[128];
        sprintf(filename, "%s.traineddata", args->tesseract_lang);
        const char *path = find_file_in_paths(TESS_DATAPATHS, filename);
        if (path == NULL) {
            LOG_FATAL("cli.c", "Could not find tesseract language file!");
        }

        ret = TessBaseAPIInit3(api, path, args->tesseract_lang);
        if (ret != 0) {
            fprintf(stderr, "Could not initialize tesseract with lang '%s'\n", args->tesseract_lang);
            return 1;
        }
        TessBaseAPIEnd(api);
        TessBaseAPIDelete(api);

        args->tesseract_path = path;
    }

    if (args->exclude_regex != NULL) {
        const char *error;
        int error_offset;

        pcre *re = pcre_compile(args->exclude_regex, 0, &error, &error_offset, 0);
        if (error != NULL) {
            LOG_FATALF("cli.c", "pcre_compile returned error: %s (offset:%d)", error, error_offset)
        }

        pcre_extra *re_extra = pcre_study(re, 0, &error);
        if (error != NULL) {
            LOG_FATALF("cli.c", "pcre_study returned error: %s", error)
        }

        ScanCtx.exclude = re;
        ScanCtx.exclude_extra = re_extra;
    } else {
        ScanCtx.exclude = NULL;
    }

    if (args->treemap_threshold_str == 0) {
        args->treemap_threshold = DEFAULT_TREEMAP_THRESHOLD;
    } else {
        args->treemap_threshold = atof(args->treemap_threshold_str);
    }

    if (args->max_memory_buffer == 0) {
        args->max_memory_buffer = DEFAULT_MAX_MEM_BUFFER;
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
    LOG_DEBUGF("cli.c", "arg archive_passphrase=%s", args->archive_passphrase)
    LOG_DEBUGF("cli.c", "arg tesseract_lang=%s", args->tesseract_lang)
    LOG_DEBUGF("cli.c", "arg tesseract_path=%s", args->tesseract_path)
    LOG_DEBUGF("cli.c", "arg exclude=%s", args->exclude_regex)
    LOG_DEBUGF("cli.c", "arg fast=%d", args->fast)
    LOG_DEBUGF("cli.c", "arg treemap_threshold=%f", args->treemap_threshold)
    LOG_DEBUGF("cli.c", "arg max_memory_buffer=%d", args->max_memory_buffer)

    return 0;
}

int load_external_file(const char *file_path, char **dst) {
    struct stat info;
    int res = stat(file_path, &info);

    if (res == -1) {
        LOG_ERRORF("cli.c", "Error opening file '%s': %s\n", file_path, strerror(errno))
        return 1;
    }

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        LOG_ERRORF("cli.c", "Error opening file '%s': %s\n", file_path, strerror(errno))
        return 1;
    }

    *dst = malloc(info.st_size + 1);
    res = read(fd, *dst, info.st_size);
    if (res < 0) {
        LOG_ERRORF("cli.c", "Error reading file '%s': %s\n", file_path, strerror(errno))
        return 1;
    }

    *(*dst + info.st_size) = '\0';
    close(fd);

    return 0;
}

int index_args_validate(index_args_t *args, int argc, const char **argv) {

    LogCtx.verbose = 1;

    if (argc < 2) {
        fprintf(stderr, "Required positional argument: PATH.\n");
        return 1;
    }

    if (args->threads == 0) {
        args->threads = 1;
    } else if (args->threads < 0) {
        fprintf(stderr, "Invalid threads: %d\n", args->threads);
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

    if (args->es_index == NULL) {
        args->es_index = DEFAULT_ES_INDEX;
    }

    if (args->script_path != NULL) {
        if (load_external_file(args->script_path, &args->script) != 0) {
            return 1;
        }
    }

    if (args->es_settings_path != NULL) {
        if (load_external_file(args->es_settings_path, &args->es_settings) != 0) {
            return 1;
        }
    }

    if (args->es_mappings_path != NULL) {
        if (load_external_file(args->es_mappings_path, &args->es_mappings) != 0) {
            return 1;
        }
    }

    if (args->batch_size == 0) {
        args->batch_size = DEFAULT_BATCH_SIZE;
    }

    LOG_DEBUGF("cli.c", "arg es_url=%s", args->es_url)
    LOG_DEBUGF("cli.c", "arg es_index=%s", args->es_index)
    LOG_DEBUGF("cli.c", "arg index_path=%s", args->index_path)
    LOG_DEBUGF("cli.c", "arg script_path=%s", args->script_path)
    LOG_DEBUGF("cli.c", "arg async_script=%s", args->async_script)
    LOG_DEBUGF("cli.c", "arg script=%s", args->script)
    LOG_DEBUGF("cli.c", "arg print=%d", args->print)
    LOG_DEBUGF("cli.c", "arg es_mappings_path=%s", args->es_mappings_path)
    LOG_DEBUGF("cli.c", "arg es_mappings=%s", args->es_mappings)
    LOG_DEBUGF("cli.c", "arg es_settings_path=%s", args->es_settings_path)
    LOG_DEBUGF("cli.c", "arg es_settings=%s", args->es_settings)
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

    if (args->listen_address == NULL) {
        args->listen_address = DEFAULT_LISTEN_ADDRESS;
    }

    if (args->es_index == NULL) {
        args->es_index = DEFAULT_ES_INDEX;
    }

    if (args->credentials != NULL) {
        char *ptr = strstr(args->credentials, ":");
        if (ptr == NULL) {
            fprintf(stderr, "Invalid --auth format, see usage\n");
            return 1;
        }

        strncpy(args->auth_user, args->credentials, (ptr - args->credentials));
        strcpy(args->auth_pass, ptr + 1);

        if (strlen(args->auth_user) == 0) {
            fprintf(stderr, "--auth username must be at least one character long");
            return 1;
        }

        args->auth_enabled = TRUE;
    } else {
        args->auth_enabled = FALSE;
    }

    if (args->tag_credentials != NULL && args->credentials != NULL) {
        fprintf(stderr, "--auth and --tag-auth are mutually exclusive");
        return 1;
    }

    if (args->tag_credentials != NULL) {
        char *ptr = strstr(args->tag_credentials, ":");
        if (ptr == NULL) {
            fprintf(stderr, "Invalid --tag-auth format, see usage\n");
            return 1;
        }

        strncpy(args->auth_user, args->tag_credentials, (ptr - args->tag_credentials));
        strcpy(args->auth_pass, ptr + 1);

        if (strlen(args->auth_user) == 0) {
            fprintf(stderr, "--tag-auth username must be at least one character long");
            return 1;
        }

        args->tag_auth_enabled = TRUE;
    } else {
        args->tag_auth_enabled = FALSE;
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
    LOG_DEBUGF("cli.c", "arg es_index=%s", args->es_index)
    LOG_DEBUGF("cli.c", "arg listen=%s", args->listen_address)
    LOG_DEBUGF("cli.c", "arg credentials=%s", args->credentials)
    LOG_DEBUGF("cli.c", "arg tag_credentials=%s", args->tag_credentials)
    LOG_DEBUGF("cli.c", "arg auth_user=%s", args->auth_user)
    LOG_DEBUGF("cli.c", "arg auth_pass=%s", args->auth_pass)
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

int exec_args_validate(exec_args_t *args, int argc, const char **argv) {

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

    if (args->es_index == NULL) {
        args->es_index = DEFAULT_ES_INDEX;
    }

    if (args->script_path == NULL) {
        LOG_FATAL("cli.c", "--script-file argument is required");
    }

    if (load_external_file(args->script_path, &args->script) != 0) {
        return 1;
    }

    LOG_DEBUGF("cli.c", "arg script_path=%s", args->script_path)
    LOG_DEBUGF("cli.c", "arg script=%s", args->script)
    return 0;
}
