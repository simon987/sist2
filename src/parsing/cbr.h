#ifndef SIST2_CBR_H
#define SIST2_CBR_H

#include "src/sist.h"

void cbr_init();

int is_cbr(unsigned int mime);

void parse_cbr(void *buf, size_t buf_len, document_t *doc);

#endif
