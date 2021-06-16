/*
   C Perlin noise implementation from Wikipedia
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "perlin.h"

int permutation [] = { 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 
                      103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 
                      26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 
                      87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 
                      77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 
                      46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 
                      187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 
                      198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 
                      255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 
                      170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 
                      172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 
                      104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 
                      241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 
                      157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 
                      93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 };


vec2 gradients [] = 
{
    (vec2) {.x =  1, .y =  0 },
    (vec2) {.x =  0, .y =  1 },
    (vec2) {.x = -1, .y =  0 },
    (vec2) {.x =  0, .y = -1 },
    (vec2) {.x =   SQRT22, .y =  SQRT22},
    (vec2) {.x =  -SQRT22, .y =  SQRT22},
    (vec2) {.x =  -SQRT22, .y = -SQRT22},
    (vec2) {.x =   SQRT22, .y = -SQRT22}
};

// linear interpolation
float lerp(float a, float b, float t)
{
    return (b - a) * t + a;
}

// yields deterministic random gradient using hashtables 
vec2 get_gradient(int y, int x)
{
    return gradients[permutation[(permutation[x % 256] + y) % 256] % 8];
}

// dot distance and gradient vectors
float distance_dot_gradient(int y, int x, float gy, float gx)
{
    // retrieve gradient at our coordinates
    vec2 gradient = get_gradient(y, x);

    // compute distance vector
    float dy = gy - (float) y;
    float dx = gx - (float) x;

    // return dot product
    //printf("dot : %5f\n", (dy*gradient.y + dx*gradient.x));
    return (dy*gradient.y + dx*gradient.x);
}

// compute perlin noise at coordinates y, x
float perlin_raw(float y, float x) 
{
    // get points of the grid square
    int y1 = (int)y;
    int y2 = y1 + 1;
    int x1 = (int)x;
    int x2 = x1 + 1;

    // get weights based on distance to each grid point
    float wy = y - (float) y1;
    float wx = x - (float) x1;
    //printf("wy: %5f\n", wy);
    //printf("wx: %5f\n", wx);

    // interpolate between gradient vectors along y axis, then x axis
    float g1, g2, i1, i2;

    //printf("g\n");
    g1 = distance_dot_gradient(y1, x1, y, x); 
    g2 = distance_dot_gradient(y2, x1, y, x); 
    i1 = lerp(g1, g2, wy);
    //printf("g1: %5f\n", g1);
    //printf("g2: %5f\n", g2);
    //printf("i1: %5f\n", i1);

    g1 = distance_dot_gradient(y1, x2, y, x); 
    g2 = distance_dot_gradient(y2, x2, y, x); 
    i2 = lerp(g1, g2, wy);
    //printf("g1: %5f\n", g1);
    //printf("g2: %5f\n", g2);
    //printf("i2: %5f\n", i2);

    //printf("\nRESULT: %5f\n", lerp(i1, i2, wx));

    return lerp(i1, i2, wx); 
}

// returns perlin noise value from 0 to 255 given integer coordinates
int perlin(int y, int x)
{
    // convert integer coordinates into suitable floats (using seed?)
    float yf = (y + 0.5) / 8;
    float xf = (x + 0.5) / 8;

    // get raw perlin noise value
    float raw = perlin_raw(yf, xf);
    int quantized = ((SQRT22 + raw) / 2 / SQRT22 * 255);

    // clamp perlins range from +/-sqrt2/2 to 0 to 255
    return quantized;
}
