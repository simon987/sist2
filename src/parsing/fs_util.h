#ifndef SIST2_FS_UTIL_H
#define SIST2_FS_UTIL_H

#include "src/sist.h"

#define CLOSE_FILE(f) if ((f).close != NULL) {(f).close(&(f));};

static int fs_read(struct vfile *f, void *buf, size_t size) {
    if (f->fd == -1) {
        SHA1_Init(&f->sha1_ctx);

        f->fd = open(f->filepath, O_RDONLY);
        if (f->fd == -1) {
            return -1;
        }
    }

    int ret = (int) read(f->fd, buf, size);

    if (ret != 0 && f->calculate_checksum) {
        f->has_checksum = TRUE;
        safe_sha1_update(&f->sha1_ctx, (unsigned char *) buf, ret);
    }

    return ret;
}

static void fs_close(struct vfile *f) {
    if (f->fd != -1) {
        SHA1_Final(f->sha1_digest, &f->sha1_ctx);
        close(f->fd);
    }
}

static void fs_reset(struct vfile *f) {
    if (f->fd != -1) {
        lseek(f->fd, 0, SEEK_SET);
    }
}

#endif
