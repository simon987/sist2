#include "ctx.h"

ScanCtx_t ScanCtx = {
        .stat_index_size = 0,
        .stat_tn_size = 0,
        .dbg_current_files = NULL,
        .pool = NULL
};
WebCtx_t WebCtx;
IndexCtx_t IndexCtx;
LogCtx_t LogCtx;
