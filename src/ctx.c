#include "ctx.h"

ScanCtx_t ScanCtx = {
        .stat_index_size = 0,
        .stat_tn_size = 0,
        .pool = NULL,
        .index.path = {0,},
};
WebCtx_t WebCtx;
IndexCtx_t IndexCtx;
LogCtx_t LogCtx;
__thread ProcData_t ProcData;
