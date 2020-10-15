#ifndef DEBUG_INCLUDE_H
#define DEBUG_INCLUDE_H

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#define LDL_ERROR(...) do{fprintf(stderr,  "ERROR: %s: ", __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);
#define LDL_DEBUG(...) do{fprintf(stderr,  "DEBUG: %s: ", __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);
#define LDL_INFO(...) do{fprintf(stderr,   "INFO: %s: ", __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);

void print_hex(FILE *fd, const uint8_t *data, size_t size);
extern FILE * trace_desc;

#define LDL_TRACE_BEGIN() fprintf(trace_desc, "TRACE: %s: ", __FUNCTION__);
#define LDL_TRACE_PART(...) fprintf(trace_desc, __VA_ARGS__);
#define LDL_TRACE_HEX(PTR, LEN) print_hex(trace_desc, PTR, LEN);
#define LDL_TRACE_FINAL() fprintf(trace_desc, "\n");

#define LDL_ASSERT(X) assert((X));
#define LDL_PEDANTIC(X) LDL_ASSERT(X)

#endif
