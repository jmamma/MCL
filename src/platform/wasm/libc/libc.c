// libc.c — wasm-side libc implementations.
//
// Clang on wasm32 also emits calls to memcpy/memset/memcmp for
// struct copies and compound-literal initialisation, so we must export
// these as plain C symbols. Use __builtin_* where clang accepts them to
// keep the code tiny and to let the optimiser specialise.
//
// Math + stdio are forwarded to host imports — see host_imports.h.
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "../host_imports.h"

// --------- string.h -------------------------------------------------------

void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--) d[i - 1] = s[i - 1];
    }
    return dst;
}

void* memset(void* dst, int v, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    unsigned char vv = (unsigned char)v;
    for (size_t i = 0; i < n; i++) d[i] = vv;
    return dst;
}

int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* x = (const unsigned char*)a;
    const unsigned char* y = (const unsigned char*)b;
    for (size_t i = 0; i < n; i++) {
        if (x[i] != y[i]) return (int)x[i] - (int)y[i];
    }
    return 0;
}

void* memchr(const void* s, int c, size_t n) {
    const unsigned char* p = (const unsigned char*)s;
    unsigned char cc = (unsigned char)c;
    for (size_t i = 0; i < n; i++) if (p[i] == cc) return (void*)(p + i);
    return 0;
}

size_t strlen(const char* s) {
    const char* p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

char* strcpy(char* dst, const char* src) {
    char* d = dst;
    while ((*d++ = *src++)) { }
    return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
    return dst;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i] || a[i] == 0)
            return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
    }
    return 0;
}

char* strchr(const char* s, int c) {
    char cc = (char)c;
    for (; *s; ++s) if (*s == cc) return (char*)s;
    return cc == 0 ? (char*)s : 0;
}

char* strrchr(const char* s, int c) {
    char cc = (char)c;
    const char* last = 0;
    for (; *s; ++s) if (*s == cc) last = s;
    if (cc == 0) return (char*)s;
    return (char*)last;
}

char* strcat(char* dst, const char* src) {
    char* d = dst + strlen(dst);
    strcpy(d, src);
    return dst;
}

char* strncat(char* dst, const char* src, size_t n) {
    char* d = dst + strlen(dst);
    size_t i = 0;
    for (; i < n && src[i]; i++) d[i] = src[i];
    d[i] = 0;
    return dst;
}

char* strstr(const char* h, const char* n) {
    if (!*n) return (char*)h;
    for (; *h; ++h) {
        const char* a = h;
        const char* b = n;
        while (*a && *b && *a == *b) { ++a; ++b; }
        if (!*b) return (char*)h;
    }
    return 0;
}

// --------- stdlib.h -------------------------------------------------------

static unsigned int _seed = 1;

int  rand(void) {
    _seed = _seed * 1103515245u + 12345u;
    return (int)((_seed >> 16) & 0x7FFFu);
}
void srand(unsigned int seed) { _seed = seed; }

// Heap: forwarded to a wasm-side bump allocator from MCL's wasm runtime if
// available; for now route through dlmalloc-style host imports. The plugin
// side will hook these to a per-instance arena.
void* malloc(size_t n) {
    extern void* host_malloc(size_t);
    return host_malloc(n);
}
void  free  (void* p) {
    extern void host_free(void*);
    host_free(p);
}
void* calloc(size_t n, size_t sz) {
    size_t total = n * sz;
    void* p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}
void* realloc(void* p, size_t n) {
    extern void* host_realloc(void*, size_t);
    return host_realloc(p, n);
}

int atoi(const char* s) {
    int sign = 1, v = 0;
    while (*s == ' ' || *s == '\t') ++s;
    if (*s == '-') { sign = -1; ++s; }
    else if (*s == '+') ++s;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return sign * v;
}
long atol(const char* s) { return (long)atoi(s); }

// --------- stdio.h --------------------------------------------------------

// stdout/stderr exist as opaque non-null sentinels. Implementations only
// look at "is this stdout or stderr?" via pointer compare or just write.
static int _stdout_marker = 1;
static int _stderr_marker = 2;
FILE* const stdout = (FILE*)&_stdout_marker;
FILE* const stderr = (FILE*)&_stderr_marker;

// vsnprintf is the one piece of formatted output MCL leans on. Tiny impl
// covers %d/%u/%x/%s/%c with optional zero-pad width.
static char* itoa10(int v, char* end) {
    int neg = 0; unsigned u;
    if (v < 0) { neg = 1; u = (unsigned)(-v); } else { u = (unsigned)v; }
    *--end = 0;
    if (u == 0) *--end = '0';
    while (u) { *--end = (char)('0' + u % 10); u /= 10; }
    if (neg) *--end = '-';
    return end;
}
static char* utoax(unsigned v, char* end, int base) {
    *--end = 0;
    if (v == 0) *--end = '0';
    while (v) {
        unsigned d = v % (unsigned)base;
        *--end = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned)base;
    }
    return end;
}

int vsnprintf(char* dst, size_t cap, const char* fmt, va_list ap) {
    size_t out = 0;
    char tmp[32];
    while (*fmt && out + 1 < cap) {
        if (*fmt != '%') { dst[out++] = *fmt++; continue; }
        ++fmt;
        // skip flags/width
        while (*fmt == '0' || (*fmt >= '1' && *fmt <= '9') || *fmt == '-' || *fmt == '+') ++fmt;
        // length mod
        while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z') ++fmt;
        switch (*fmt++) {
            case 's': {
                const char* s = va_arg(ap, const char*);
                if (!s) s = "(null)";
                while (*s && out + 1 < cap) dst[out++] = *s++;
                break;
            }
            case 'c': {
                int c = va_arg(ap, int);
                dst[out++] = (char)c;
                break;
            }
            case 'd': case 'i': {
                int v = va_arg(ap, int);
                char* p = itoa10(v, tmp + sizeof(tmp));
                while (*p && out + 1 < cap) dst[out++] = *p++;
                break;
            }
            case 'u': {
                unsigned v = va_arg(ap, unsigned);
                char* p = utoax(v, tmp + sizeof(tmp), 10);
                while (*p && out + 1 < cap) dst[out++] = *p++;
                break;
            }
            case 'x': case 'X': case 'p': {
                unsigned v = va_arg(ap, unsigned);
                char* p = utoax(v, tmp + sizeof(tmp), 16);
                while (*p && out + 1 < cap) dst[out++] = *p++;
                break;
            }
            case '%': dst[out++] = '%'; break;
            default:  dst[out++] = '?'; break;
        }
    }
    if (cap) dst[out] = 0;
    return (int)out;
}

int snprintf(char* dst, size_t cap, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(dst, cap, fmt, ap);
    va_end(ap);
    return r;
}
int sprintf(char* dst, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(dst, (size_t)-1, fmt, ap);
    va_end(ap);
    return r;
}

// Output goes through host_log. printf/fprintf/etc share a small line
// buffer to batch formatted output; in practice MCL barely uses these.
static char _print_buf[256];
int printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(_print_buf, sizeof(_print_buf), fmt, ap);
    va_end(ap);
    host_log(_print_buf);
    return n;
}
int fprintf(FILE* /*f*/, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(_print_buf, sizeof(_print_buf), fmt, ap);
    va_end(ap);
    host_log(_print_buf);
    return n;
}
int vfprintf(FILE* /*f*/, const char* fmt, va_list ap) {
    int n = vsnprintf(_print_buf, sizeof(_print_buf), fmt, ap);
    host_log(_print_buf);
    return n;
}
int puts(const char* s)              { host_log(s); return 0; }
int fputs(const char* s, FILE* /*f*/) { host_log(s); return 0; }
int fputc(int c, FILE* /*f*/)         { char buf[2]; buf[0] = (char)c; buf[1] = 0; host_log(buf); return c; }
int putchar(int c)                    { return fputc(c, stdout); }

// --------- math.h ---------------------------------------------------------
//
// Forwarded to host imports. WAMR can register libm-backed natives, so the
// host gets to pick the precision/perf tradeoff. For sinf/cosf/sqrtf we
// cast through double.

extern double host_math_sin(double);
extern double host_math_cos(double);
extern double host_math_tan(double);
extern double host_math_sqrt(double);
extern double host_math_pow(double, double);
extern double host_math_exp(double);
extern double host_math_log(double);

double sin (double x) { return host_math_sin(x); }
double cos (double x) { return host_math_cos(x); }
double tan (double x) { return host_math_tan(x); }
double sqrt(double x) { return host_math_sqrt(x); }
double fabs(double x) { return x < 0 ? -x : x; }
double floor(double x){ double r = (double)(long long)x; return (x < 0 && r != x) ? r - 1.0 : r; }
double ceil (double x){ double r = (double)(long long)x; return (x > 0 && r != x) ? r + 1.0 : r; }
double pow(double x, double y){ return host_math_pow(x, y); }
double exp(double x) { return host_math_exp(x); }
double log(double x) { return host_math_log(x); }

float sinf (float x) { return (float)host_math_sin((double)x); }
float cosf (float x) { return (float)host_math_cos((double)x); }
float sqrtf(float x) { return (float)host_math_sqrt((double)x); }
float powf (float x, float y) { return (float)host_math_pow((double)x, (double)y); }
float fabsf(float x) { return x < 0 ? -x : x; }
float floorf(float x) { return (float)floor((double)x); }
float expf(float x) { return (float)host_math_exp((double)x); }
float logf(float x) { return (float)host_math_log((double)x); }
