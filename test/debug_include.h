#ifndef DEBUG_INCLUDE_H
#define DEBUG_INCLUDE_H

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#define LORA_ERROR(...) do{fprintf(stderr,  "%s: %u: %s: error: ", __FILE__, __LINE__, __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);
#define LORA_DEBUG(...) do{fprintf(stderr,  "%s: %u: %s: debug: ", __FILE__, __LINE__, __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);
#define LORA_INFO(...) do{fprintf(stderr,   "%s: %u: %s: info: ", __FILE__, __LINE__, __FUNCTION__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");}while(0);

#define LORA_ASSERT(X) assert((X));
#define LORA_PEDANTIC(X) LORA_ASSERT(X)

#endif
