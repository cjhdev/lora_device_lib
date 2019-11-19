#ifndef DEBUG_INCLUDE_H
#define DEBUG_INCLUDE_H

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#define LDL_ERROR(APP,...) do{fprintf(stderr,  "%s: %u: %s: error: ", __FILE__, __LINE__, __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);
#define LDL_DEBUG(APP,...) do{fprintf(stderr,  "%s: %u: %s: debug: ", __FILE__, __LINE__, __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);
#define LDL_INFO(APP,...) do{fprintf(stderr,   "%s: %u: %s: info: ", __FILE__, __LINE__, __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);

#define LDL_ASSERT(X) assert((X));
#define LDL_PEDANTIC(X) LDL_ASSERT(X)

#endif
