#include "ctx.h"

ScanCtx_t ScanCtx = {
        .pool = NULL,
        .index.path = {0,},
};
WebCtx_t WebCtx;
IndexCtx_t IndexCtx;
LogCtx_t LogCtx;
__thread ProcData_t ProcData;
