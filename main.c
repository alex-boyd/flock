#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "perlin.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

#define APP_NAME "world"

// exit failure if result operation was not a success
#define TRY(result) 														\
	if (VK_SUCCESS != (result)) 											\
	{																		\
		printf("Fatal error in %s on line %u. :(\n", __FILE__, __LINE__); 	\
		exit(1);															\
	}

int main(int argc, char** argv)
{
    // sdl vars
    SDL_Window * win = NULL;
    SDL_Surface * surf = NULL;

    // vk vars
	VkInstance instance;
	uint32_t physical_device_count = 0;
    uint32_t extension_count = 0;
    char * extension_names = NULL;

    // initialize SDL ---------------------------------------------------------

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0 )
    {
        printf("SDL failed to initialize. SDL_Error: %s\n", SDL_GetError());
        return die(win, 1);
    }

    // create SDL window
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

    // initialize Vulkan ------------------------------------------------------

	// define program info data frame 
	const VkApplicationInfo app_info = 
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,	// VkStructureType
		0,									// pointer to extension
		APP_NAME,       					// name
		0,									// application version
		APP_NAME,							// engine name if any
		0,									// engine version if any
		VK_MAKE_VERSION(1,2,170) 			// vulkan API version
	};

    // get vk extensions for SDL compatibility (first count, then names)
    SDL_Vulkan_GetInstanceExtensions(win, &extension_count, NULL);
    SDL_Vulkan_GetInstanceExtensions(win, &extension_count, 
            (const char **) &extension_names);

	// define vulkan instance (state machine) data frame
	const VkInstanceCreateInfo instance_info =
	{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,	// VkStructureType
		0,										// pointer to extension
		0,										// flags
		&app_info,								// application info
		0,										// enabled layer count
		0,										// enabled layer names
		extension_count,						// enabled extension count	
	    (const char * const *) extension_names	// enabled extension names
	};

	// create instance of vulkan
	TRY(vkCreateInstance(&instance_info, 0, &instance));




    return die(win, 0);
}
