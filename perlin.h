/*
   C Perlin noise implementation from Wikipedia
*/

#ifndef PERLIN
#define PERLIN

#include <math.h>

#define SQRT22 0.707106781187f

typedef struct 
{
    float x, y;
} vec2;


// linear interpolation between floats
float lerp(float a, float b, float t);

// yields deterministic random gradient vec2 using very arbitrary coefficients
vec2 get_gradient_slow(int y, int x);

vec2 get_gradient(int y, int x);

// dot distance and gradient vectors
float distance_dot_gradient(int y, int x, float gy, float gx);

// compute perlin noise at coordinates y, x
float perlin_raw(float x, float y);

// compute noise from 0 to 255
int perlin(int y, int x);


#endif
