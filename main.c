#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "perlin.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

#define APP_NAME "flock"

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
    VkDevice device;
    VkQueue queue;
    VkSurfaceKHR vk_surf;

	uint32_t physical_device_count = 0;
    uint32_t extension_count = 0;
    char * extension_names = NULL;

    // initialize SDL ---------------------------------------------------------

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0 )
    {
        printf("SDL failed to initialize. SDL_Error: %s\n", SDL_GetError());
        return die(win, 1, NULL);
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
        return die(win, 1, NULL);
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
            (const char **) &extension_names); // need malloc?

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

	// get physical devices, first calling enumerate for count, then for handles
	TRY(vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL));
	VkPhysicalDevice * const physical_devices = (VkPhysicalDevice *) 
		malloc( sizeof(VkPhysicalDevice) * physical_device_count );
	TRY(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));
	
	// get queue families from physical devices, pick a graphics capable one 
    uint8_t graphics_bit = 0;
	uint32_t physical_device_index = 0;
	uint32_t queue_family_index = 0;
    uint8_t queue_count = 0;

	for (uint32_t i = 0; i < physical_device_count; i++)
	{
		// call twice for queue family property count, then their handles
		uint32_t queue_family_properties_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties
		(
			physical_devices[i],				// device in question 
			&queue_family_properties_count,	// address to write property count
			0								// null (get count mode)
		);
		VkQueueFamilyProperties * const queue_family_properties = 
			(VkQueueFamilyProperties *) malloc
			(sizeof(VkQueueFamilyProperties) * queue_family_properties_count); 
		vkGetPhysicalDeviceQueueFamilyProperties
		(
			physical_devices[i],				// device in question 
			&queue_family_properties_count,	// address to write property count
			queue_family_properties			// memory we just allocated
		);
        
		// iterate over queue families to find a graphics capable one
		for(uint32_t j = 0; j < queue_family_properties_count; j++)
		{
			if((VK_QUEUE_GRAPHICS_BIT & queue_family_properties[j].queueFlags) != 0)
			{
				graphics_bit = 1;
                queue_family_index = j;
                physical_device_index = i;
				queue_count = queue_family_properties[j].queueCount;
                break;
			}
        }

        if (graphics_bit != 0) { continue; } // failed to find gfx

        // create logical device
        const float queue_priority = 1.0f; // one normalized float / queue
        const VkDeviceQueueCreateInfo device_queue_info =
        {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,	// structure type
            0,											// extension ptr
            0,											// flags
            queue_family_index,											// queue family idx
            1, /*queue_count,*/							// queue count
            &queue_priority
        };
			
        const VkDeviceCreateInfo device_info =
        {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,	// structure type
            0,										// extension pointer
            0,										// flags	
            1,										// queue create info count	
            &device_queue_info,    					// pointer to array of create infos
            0,										// enabled layers
            0,										// ppEnabledLayerNames
            extension_count,						// enabled extension count
            (const char * const *) extension_names,	// enabled extension names
            0										// enabled physical device features
        };

        vkCreateDevice(
                physical_devices[physical_device_index], // physical device
                &device_info,                            // device info above
                NULL,                                    // alloc callbk ptr
                &device );                               // logical device 


        // create vulkan surface, check compatibility with queue family
        SDL_Vulkan_CreateSurface(win, instance, &vk_surf);
        VkBool32 khr_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(
                physical_devices[physical_device_index],
                queue_family_index,
                vk_surf,
                &khr_support );

        if (khr_support != VK_TRUE) { die(win, 1, "No KHR support"); }

		// create logical queue
        vkGetDeviceQueue(
                device,
                queue_family_index,
                0,                  // queue_index (of queue_count, 1)
                &queue );



		// allocate memory
		// make in/out buffers out of memory
		// command buffer
		// shaders??
		// compile shaders

		// free loop's resources
        free(queue_family_properties);



    }





	// free resources
	free(physical_devices);
    //vkDestroyDevice(device);

    return die(win, 0, NULL);
}
