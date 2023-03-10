#ifndef SIST2_PARSE_H
#define SIST2_PARSE_H

#include "../sist.h"
#include "src/tpool.h"

#define MAGIC_BUF_SIZE (4096 * 6)

int fs_read(struct vfile *f, void *buf, size_t size);
void fs_close(struct vfile *f);
void fs_reset(struct vfile *f);

void parse_job(parse_job_t *job);
void parse(tpool_work_arg_shm_t *arg);

void cleanup_parse();

#endif
