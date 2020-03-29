#include "src/sist.h"

#include <onion/handler.h>
#include <onion/onion.h>

onion_handler *auth_basic(const char *b64credentials, onion_handler *inside_level);
