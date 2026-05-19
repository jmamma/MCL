// math.h — minimal stub. MCL uses sin/cos for UI animation + a few
// trig helpers in Osc.cpp. Backed by libm via host imports, or simple
// approximations in libc.c.
#pragma once

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif
#ifndef M_E
#define M_E     2.71828182845904523536
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923
#endif

#ifdef __cplusplus
extern "C" {
#endif

double sin (double x);
double cos (double x);
double tan (double x);
double sqrt(double x);
double fabs(double x);
double floor(double x);
double ceil(double x);
double pow(double x, double y);
double exp(double x);
double log(double x);

float sinf (float x);
float cosf (float x);
float sqrtf(float x);
float powf (float x, float y);
float fabsf(float x);
float floorf(float x);
float expf(float x);
float logf(float x);

#ifdef __cplusplus
}
#endif
