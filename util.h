#ifndef UTIL
#define UTIL

#include <SDL2/SDL.h>

struct Vertex {
    float position[3];
    float color[3];
};


int die(SDL_Window * win, int status);


#endif
