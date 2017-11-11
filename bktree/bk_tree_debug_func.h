#ifndef BK_TREE_DEBUG_CALLS
#define BK_TREE_DEBUG_CALLS

#include "postgres.h"

// #define fprintf_to_ereport(msg, ...)  ereport(NOTICE, (errmsg_internal(msg, ##__VA_ARGS__)))
#define fprintf_to_ereport(msg, ...)

#endif // BK_TREE_DEBUG_CALLS
