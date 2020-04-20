#ifndef SIST2_LOG_H
#define SIST2_LOG_H


#define LOG_MAX_LENGTH 8192

#define SIST_DEBUG 0
#define SIST_INFO 1
#define SIST_WARNING 2
#define SIST_ERROR 3
#define SIST_FATAL 4

#define LOG_DEBUGF(filepath, fmt, ...) \
    if (LogCtx.very_verbose) {sist_logf(filepath, SIST_DEBUG, fmt, __VA_ARGS__);}
#define LOG_DEBUG(filepath, str) \
    if (LogCtx.very_verbose) {sist_log(filepath, SIST_DEBUG, str);}

#define LOG_INFOF(filepath, fmt, ...) \
    if (LogCtx.verbose) {sist_logf(filepath, SIST_INFO, fmt, __VA_ARGS__);}
#define LOG_INFO(filepath, str) \
    if (LogCtx.verbose) {sist_log(filepath, SIST_INFO, str);}

#define LOG_WARNINGF(filepath, fmt, ...) \
    if (LogCtx.verbose) {sist_logf(filepath, SIST_WARNING, fmt, __VA_ARGS__);}
#define LOG_WARNING(filepath, str) \
    if (LogCtx.verbose) {sist_log(filepath, SIST_WARNING, str);}

#define LOG_ERRORF(filepath, fmt, ...) \
    if (LogCtx.verbose) {sist_logf(filepath, SIST_ERROR, fmt, __VA_ARGS__);}
#define LOG_ERROR(filepath, str) \
    if (LogCtx.verbose) {sist_log(filepath, SIST_ERROR, str);}

#define LOG_FATALF(filepath, fmt, ...) \
    sist_logf(filepath, SIST_FATAL, fmt, __VA_ARGS__);\
    exit(-1);
#define LOG_FATAL(filepath, str) \
    sist_log(filepath, SIST_FATAL, str);\
    exit(-1);

#include "sist.h"

void sist_logf(const char *filepath, int level, char *format, ...);

void sist_log(const char *filepath, int level, char *str);

#endif
