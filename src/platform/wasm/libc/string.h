// string.h — minimal stub for freestanding wasm32. Forwarders to
// __builtin_* where clang provides them; everything else is in libc.c.
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void*  memcpy (void* dst, const void* src, size_t n);
void*  memmove(void* dst, const void* src, size_t n);
void*  memset (void* dst, int v, size_t n);
int    memcmp (const void* a, const void* b, size_t n);
void*  memchr (const void* s, int c, size_t n);

size_t strlen (const char* s);
char*  strcpy (char* dst, const char* src);
char*  strncpy(char* dst, const char* src, size_t n);
int    strcmp (const char* a, const char* b);
int    strncmp(const char* a, const char* b, size_t n);
char*  strchr (const char* s, int c);
char*  strrchr(const char* s, int c);
char*  strcat (char* dst, const char* src);
char*  strncat(char* dst, const char* src, size_t n);
char*  strstr (const char* h, const char* n);

#ifdef __cplusplus
}
#endif
