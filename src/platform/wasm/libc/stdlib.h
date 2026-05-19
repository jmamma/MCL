// stdlib.h — minimal freestanding stub for wasm32.
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tiny pseudo-random; MCL only uses it for UI animation seeds.
int  rand(void);
void srand(unsigned int seed);

// Heap. The plugin-side wasm runtime is expected to provide these — for
// the first wasm cut we forward to host imports; later WAMR's built-in
// heap can take over.
void* malloc(size_t n);
void  free  (void* p);
void* calloc(size_t n, size_t sz);
void* realloc(void* p, size_t n);

#define abs(x) ((x) < 0 ? -(x) : (x))
int atoi(const char* s);
long atol(const char* s);

#ifdef __cplusplus
}
#endif
