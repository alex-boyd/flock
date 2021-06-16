#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "perlin.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main(int argc, char** argv)
{
    SDL_Window * win = NULL;
    SDL_Surface * surf = NULL;

    if (SDL_INIT(SDL_INIT_VIDEO) < 0 )
    {
        printf("SDL failed to initialize. SDL_Error: %s\n", SDL_GetError());
        return die(win, 1);
    }

    win = SDL_CreateWindow(
            "Hello World", 
            SDL_WINDOWPOS_UNDEFINED, 
            SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN );

    if (win == NULL)
    {
        printf("Window could not be created. SDL_Error: %s\n", SDL_GetError());
        return die(win, 1);
    }

    surf = SDL_GetWindowSurface(win);

    // hello world
    SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 0xFF, 0xFF, 0xFF));
    SDL_UpdateWindowSurface(win);
    SDL_Delay(2000);

    return die(win, 0);
}
