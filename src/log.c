#include "log.h"

#include <pthread.h>
#include <stdarg.h>

const char *log_colors[] = {
        "\033[34m", "\033[01;34m", "\033[0m",
        "\033[01;33m", "\033[31m", "\033[01;31m"
};

const char *log_levels[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

void sist_logf(char *filepath, int level, char *format, ...) {

    static int is_tty = -1;
    if (is_tty == -1) {
        is_tty = isatty(STDERR_FILENO);
    }

    char log_str[LOG_MAX_LENGTH];

    unsigned long long pid = (unsigned long long) pthread_self();

    char datetime[32];
    time_t t;
    struct tm result;
    t = time(NULL);
    localtime_r(&t, &result);
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", &result);

    int log_len;
    if (is_tty) {
        log_len = snprintf(
                log_str, sizeof(log_str),
                "\033[%dm[%04llX]%s [%s] [%s %s] ",
                31 + ((unsigned int) (pid)) % 7, pid, log_colors[level],
                datetime, log_levels[level], filepath
        );
    } else {
        log_len = snprintf(
                log_str, sizeof(log_str),
                "[%04llX] [%s] [%s %s] ",
                pid, datetime, log_levels[level], filepath
        );
    }

    va_list ap;
    va_start(ap, format);
    size_t maxsize = sizeof(log_str) - log_len;
    log_len += vsnprintf(log_str + log_len, maxsize, format, ap);
    va_end(ap);

    if (is_tty) {
        log_len += sprintf(log_str + log_len, "\033[0m\n");
    } else {
        *(log_str + log_len) = '\n';
        log_len += 1;
    }

    int ret = write(STDERR_FILENO, log_str, log_len);
    if (ret == -1) {
        LOG_FATALF("serialize.c", "Could not write index descriptor: %s", strerror(errno));
    }
}

void sist_log(char *filepath, int level, char *str) {

    static int is_tty = -1;
    if (is_tty == -1) {
        is_tty = isatty(STDERR_FILENO);
    }

    char log_str[LOG_MAX_LENGTH];

    unsigned long long pid = (unsigned long long) pthread_self();

    char datetime[32];
    time_t t;
    struct tm result;
    t = time(NULL);
    localtime_r(&t, &result);
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", &result);

    int log_len;
    if (is_tty) {
        log_len = snprintf(
                log_str, sizeof(log_str),
                "\033[%dm[%04llX]%s [%s] [%s %s] %s \033[0m\n",
                31 + ((unsigned int) (pid)) % 7, pid, log_colors[level],
                datetime, log_levels[level], filepath,
                str
        );
    } else {
        log_len = snprintf(
                log_str, sizeof(log_str),
                "[%04llX] [%s] [%s %s] %s \n",
                pid, datetime, log_levels[level], filepath,
                str
        );
    }

    int ret = write(STDERR_FILENO, log_str, log_len);
    if (ret == -1) {
        LOG_FATALF("serialize.c", "Could not write index descriptor: %s", strerror(errno));
    }
}
