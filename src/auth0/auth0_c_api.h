#ifndef SIST2_AUTH0_C_API_H
#define SIST2_AUTH0_C_API_H


#ifdef __cplusplus
#define EXTERNC extern "C"
#include "cstdlib"
#else
#define EXTERNC
#include "stdlib.h"
#endif

#define AUTH0_OK (0)
#define AUTH0_ERR_EXPIRED (1)
#define AUTH0_ERR_SIG_FORMAT (2)
#define AUTH0_ERR_DECODE (3)
#define AUTH0_ERR_VERIFICATION (4)
#define AUTH0_ERR_AUDIENCE (5)

EXTERNC int auth0_verify_jwt(const char *secret, const char *token, const char* audience);

#endif
