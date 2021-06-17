#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

int die(SDL_Window * win, int status, char * error)
{
    if(error != NULL) { printf(error); }
    SDL_DestroyWindow(win);
    SDL_Quit();
    return status;
}

