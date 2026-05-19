// stdio.h — minimal stub. printf/fprintf forward to host_log; snprintf is
// the only one with real semantics MCL relies on (string formatting for
// LCD labels).
#pragma once

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FILE FILE;
extern FILE* const stdout;
extern FILE* const stderr;

int snprintf(char* dst, size_t cap, const char* fmt, ...);
int vsnprintf(char* dst, size_t cap, const char* fmt, va_list ap);
int sprintf(char* dst, const char* fmt, ...);

int printf(const char* fmt, ...);
int fprintf(FILE* f, const char* fmt, ...);
int vfprintf(FILE* f, const char* fmt, va_list ap);
int puts(const char* s);
int fputs(const char* s, FILE* f);
int fputc(int c, FILE* f);
int putchar(int c);

#ifdef __cplusplus
}
#endif
