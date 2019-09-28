#include "sist.h"
#include "ctx.h"

static const char *const Version = "1.0.0";

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
    uuid_generate_time_safe(uuid);
    uuid_unparse(uuid, ScanCtx.index.desc.uuid);
    time(&ScanCtx.index.desc.timestamp);
    strcpy(ScanCtx.index.desc.version, Version);

    write_index_descriptor(path, &ScanCtx.index.desc);
}

void scan_print_header() {
    printf("sist2 V%s\n", Version);
    printf("---------------------\n");
    printf("threads\t\t%d\n", ScanCtx.threads);
    printf("tn_qscale\t%.1f/31.0\n", ScanCtx.tn_qscale);
    printf("tn_size\t\t%dpx\n", ScanCtx.tn_size);
    printf("output\t\t%s\n", ScanCtx.index.path);
}

void sist2_scan(const char *path, const char *incremental_from) {

    av_log_set_level(AV_LOG_QUIET);

    strcpy(ScanCtx.index.desc.root, abspath(path));
    ScanCtx.index.desc.root_len = (short) strlen(ScanCtx.index.desc.root);

    init_dir(ScanCtx.index.path);

    ScanCtx.mime_table = mime_get_mime_table();
    ScanCtx.ext_table = mime_get_ext_table();

    char store_path[PATH_MAX];
    snprintf(store_path, PATH_MAX, "%sthumbs", ScanCtx.index.path);
    mkdir(store_path, S_IWUSR | S_IRUSR | S_IXUSR);
    ScanCtx.index.store = store_create(store_path);

    scan_print_header();

    if (incremental_from != NULL) {
        incremental_from = abspath(incremental_from);
        ScanCtx.original_table = incremental_get_table();
        ScanCtx.copy_table = incremental_get_table();

        DIR *dir = opendir(incremental_from);
        if (dir == NULL) {
            perror("opendir");
            return;
        }
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (strncmp(de->d_name, "_index_", sizeof("_index_") - 1) == 0) {
                char file_path[PATH_MAX];
                snprintf(file_path, PATH_MAX, "%s/%s", incremental_from, de->d_name);
                incremental_read(ScanCtx.original_table, file_path);
            }
        }
        closedir(dir);

        printf("Loaded %d items in to mtime table.", g_hash_table_size(ScanCtx.original_table));
    }

    walk_directory_tree(ScanCtx.index.desc.root);
    tpool_wait(ScanCtx.pool);

    if (incremental_from != NULL) {
        char dst_path[PATH_MAX];
        snprintf(store_path, PATH_MAX, "%sthumbs", incremental_from);
        snprintf(dst_path, PATH_MAX, "%s_index_original", ScanCtx.index.path);
        store_t *source = store_create(store_path);

        DIR *dir = opendir(incremental_from);
        if (dir == NULL) {
            perror("opendir");
            return;
        }
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (strncmp(de->d_name, "_index_", sizeof("_index_") - 1) == 0) {
                char file_path[PATH_MAX];
                snprintf(file_path, PATH_MAX, "%s/%s", incremental_from, de->d_name);
                incremental_copy(source, ScanCtx.index.store, file_path, dst_path, ScanCtx.copy_table);
            }
        }
        closedir(dir);
        store_destroy(source);
    }

    store_destroy(ScanCtx.index.store);
    tpool_destroy(ScanCtx.pool);
}

void sist2_index(const char *path, int print_index, int arg_force_reset) {
    if (!print_index) {
        elastic_init(arg_force_reset);
    }
    char *index_path = abspath(path);
    char descriptor_path[PATH_MAX];
    snprintf(descriptor_path, PATH_MAX, "%s/descriptor.json", index_path);

    index_descriptor_t desc = read_index_descriptor(descriptor_path);
    if (strcmp(desc.version, Version) != 0) {
        fprintf(stderr, "Version mismatch! Index is v%s but executable is v%s\n", desc.version, Version);
        return;
    }

    DIR *dir = opendir(index_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    index_func f;
    if (print_index) {
        f = print_json;
    } else {
        f = index_json;
    }

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strncmp(de->d_name, "_index_", sizeof("_index_") - 1) == 0) {
            char file_path[PATH_MAX];
            snprintf(file_path, PATH_MAX, "%s/%s", index_path, de->d_name);
            read_index(file_path, desc.uuid, f);
        }
    }

    if (!print_index) {
        elastic_flush();
        destroy_indexer();
    }
}

void sist2_web(const char *indices[], int index_count, const char *host, const char *port) {

    for (int i = 0; i < index_count; i++) {
        char *abs_path = abspath(indices[i]);

        char path_tmp[PATH_MAX];

        snprintf(path_tmp, PATH_MAX, "%sthumbs", abs_path);
        WebCtx.indices[i].store = store_create(path_tmp);

        snprintf(path_tmp, PATH_MAX, "%sdescriptor.json", abs_path);
        WebCtx.indices[i].desc = read_index_descriptor(path_tmp);

        strcpy(WebCtx.indices[i].path, abs_path);
        printf("Loaded index: %s\n", WebCtx.indices[i].desc.name);
    }

    WebCtx.index_count = index_count;

    serve(host, port);
}

int main(int argc, const char *argv[]) {

    curl_global_init(CURL_GLOBAL_NOTHING);

    float arg_quality = 0;
    int arg_size = 0;
    int arg_content_size = 0;
    int arg_threads = 0;
    char *arg_incremental = NULL;
    char *arg_output = NULL;
    char *arg_rewrite_url = NULL;
    char *arg_name = NULL;

    char *arg_es_url = NULL;
    int arg_print_index = 0;
    int arg_force_reset = 0;

    char *arg_web_host = NULL;
    char *arg_web_port = NULL;

    struct argparse_option options[] = {
            OPT_HELP(),

            OPT_GROUP("Scan options"),
            OPT_INTEGER('t', "threads", &arg_threads, "Number of threads. DEFAULT=1"),
            OPT_FLOAT('q', "quality", &arg_quality,
                      "Thumbnail quality, on a scale of 1.0 to 31.0, 1.0 being the best. DEFAULT=15"),
            OPT_INTEGER(0, "size", &arg_size, "Thumbnail size, in pixels. DEFAULT=200"),
            OPT_INTEGER(0, "content-size", &arg_content_size,
                        "Number of bytes to be extracted from text documents. DEFAULT=4096"),
            OPT_STRING(0, "incremental", &arg_incremental, "Reuse an existing index and only scan modified files."),
            OPT_STRING('o', "output", &arg_output, "Output directory. DEFAULT=index.sist2/"),
            OPT_STRING(0, "rewrite-url", &arg_rewrite_url, "Serve files from this url instead of from disk."),
            OPT_STRING(0, "name", &arg_name, "Index display name. DEFAULT: (name of the directory)"),

            OPT_GROUP("Index options"),
            OPT_STRING(0, "es-url", &arg_es_url, "Elasticsearch url. DEFAULT=http://localhost:9200"),
            OPT_BOOLEAN('p', "print", &arg_print_index, "Just print JSON documents to stdout."),
            OPT_BOOLEAN('f', "force-reset", &arg_force_reset, "Reset Elasticsearch mappings and settings. "
                                                              "(You must use this option the first time you use the index command)"),

            OPT_GROUP("Web options"),
            OPT_STRING(0, "es-url", &arg_es_url, "Elasticsearch url. DEFAULT=http://localhost:9200"),
            OPT_STRING(0, "bind", &arg_web_host, "Listen on this address. DEFAULT=localhost"),
            OPT_STRING(0, "port", &arg_web_port, "Listen on this port. DEFAULT=4090"),

            OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(
            &argparse,
            "\nLightning-fast file system indexer and search tool.",
            "\nMade by simon987 <me@simon987.net>. Released under GPL-3.0"
    );

    argc = argparse_parse(&argparse, argc, argv);

    //Set defaults
    if (arg_quality == 0) {
        arg_quality = 15;
    } else if (arg_quality < 1 || arg_quality > 31) {
        fprintf(stderr, "Invalid quality: %f\n", arg_quality);
        return 1;
    }

    if (arg_size == 0) {
        arg_size = 200;
    } else if (arg_size <= 0) {
        fprintf(stderr, "Invalid size: %d\n", arg_size);
        return 1;
    }

    if (arg_content_size == 0) {
        arg_content_size = 4096;
    } else if (arg_content_size <= 0) {
        fprintf(stderr, "Invalid content-size: %d\n", arg_content_size);
        return 1;
    }

    if (arg_threads == 0) {
        arg_threads = 1;
    } else if (arg_threads < 0) {
        fprintf(stderr, "Invalid threads: %d\n", arg_threads);
        return 1;
    }

    if (arg_output == NULL) {
        arg_output = "index.sist2/";
    }

    if (arg_es_url == NULL) {
        arg_es_url = "http://localhost:9200";
    }

    if (arg_web_host == NULL) {
        arg_web_host = "localhost";
    }

    if (arg_web_port == NULL) {
        arg_web_port = "4090";
    }

    // Commands
    if (argc == 0) {
        argparse_usage(&argparse);
    } else if (strcmp(argv[0], "scan") == 0) {
        if (argc < 2) {
            fprintf(stderr, "Required positional argument: PATH.\n");
            argparse_usage(&argparse);
            return 1;
        }

        if (arg_name == NULL) {
            arg_name = g_path_get_basename(argv[1]);
        }

        int ret = mkdir(arg_output, S_IRUSR | S_IWUSR | S_IXUSR);
        if (ret != 0) {
            fprintf(stderr, "Invalid output: '%s' (%s).\n", arg_output, strerror(errno));
            return 1;
        }

        ScanCtx.tn_qscale = arg_quality;
        ScanCtx.tn_size = arg_size;
        ScanCtx.content_size = arg_content_size;
        ScanCtx.pool = tpool_create(arg_threads, serializer_cleanup);
        ScanCtx.threads = arg_threads;
        strncpy(ScanCtx.index.path, arg_output, sizeof(ScanCtx.index.path));
        strncpy(ScanCtx.index.desc.name, arg_name, sizeof(ScanCtx.index.desc.name));
        if (arg_rewrite_url == NULL) {
            strcpy(ScanCtx.index.desc.rewrite_url, "");
        } else {
            strcpy(ScanCtx.index.desc.rewrite_url, arg_rewrite_url);
        }
        sist2_scan(argv[1], arg_incremental);
    } else if (strcmp(argv[0], "index") == 0) {
        if (argc < 2) {
            fprintf(stderr, "Required positional argument: PATH.\n");
            argparse_usage(&argparse);
            return 1;
        }
        IndexCtx.es_url = arg_es_url;

        sist2_index(argv[1], arg_print_index, arg_force_reset);
    } else if (strcmp(argv[0], "web") == 0) {
        if (argc < 2) {
            fprintf(stderr, "Required positional argument: PATH.\n");
            argparse_usage(&argparse);
            return 1;
        }
        WebCtx.es_url = arg_es_url;

        sist2_web(argv + 1, argc - 1, arg_web_host, arg_web_port);
    } else {
        fprintf(stderr, "Invalid command: '%s'\n", argv[0]);
        argparse_usage(&argparse);
        return 1;
    }
    printf("\n");
    return 0;
}
