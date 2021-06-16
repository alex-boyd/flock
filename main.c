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

	// get GPUs, first calling enumerate for count, then to get handles
	TRY(vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL));
	printf("%d physical devices found.\n", physical_device_count);
	VkPhysicalDevice * const physical_devices = (VkPhysicalDevice *) 
		malloc( sizeof(VkPhysicalDevice) * physical_device_count );
	TRY(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));

	
	// get queue families from physical devices, pick one 
	uint32_t queue_family_index = 0; // the one we are going to use
	for (uint32_t i = 0; i < physical_device_count; i++)
	{
		// print GPU properties
		VkPhysicalDeviceProperties physical_device_properties;
		vkGetPhysicalDeviceProperties
			(physical_devices[i], &physical_device_properties);
		printf("Physical device %d:\nDevice Name: %s\n", i, physical_device_properties.deviceName);
		switch(physical_device_properties.deviceType)
		{
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
				printf("  Device Type: unknown type.\n");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				printf("  Device Type: integrated GPU.\n");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				printf("  Device Type: discrete GPU.\n");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				printf("  Device Type: virtual GPU.\n");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				printf("  Device Type: CPU.\n");
				break;
		}

		// call twice for queue family property count, then their handles
		uint32_t queue_family_properties_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties
		(
			physical_devices[i],				// device in question 
			&queue_family_properties_count,	// address to write property count
			0								// null (get count mode)
		);
		printf("  Queue Families: %d\n", queue_family_properties_count);
		VkQueueFamilyProperties * const queue_family_properties = 
			(VkQueueFamilyProperties *) malloc
			(sizeof(VkQueueFamilyProperties) * queue_family_properties_count); 
		vkGetPhysicalDeviceQueueFamilyProperties
		(
			physical_devices[i],				// device in question 
			&queue_family_properties_count,	// address to write property count
			queue_family_properties			// memory we just allocated
		);

		// iterate over queue families, printing properties
		for(uint32_t j = 0; j < queue_family_properties_count; j++)
		{
			printf("  Queue Family %d:\n", j);
			printf("    queueFlags: %d\n", queue_family_properties[j].queueFlags);
			printf("    queueCount: %d\n", queue_family_properties[j].queueCount);
			printf("    timestampValidBits: %d\n", queue_family_properties->timestampValidBits);
			printf("    minImageTransferGranularity: %d by %d by %d\n",
					queue_family_properties[j].minImageTransferGranularity.width,
					queue_family_properties[j].minImageTransferGranularity.height,
					queue_family_properties[j].minImageTransferGranularity.depth
			);  

			// print queue capabilities as indicated by flags
			uint8_t compute = 0;
			uint8_t queue_count = 0;

			if((VK_QUEUE_GRAPHICS_BIT & queue_family_properties[j].queueFlags) != 0)
			{
				printf("    graphics capability (bit %d) detected.\n", VK_QUEUE_GRAPHICS_BIT);
			}
			if((VK_QUEUE_COMPUTE_BIT & queue_family_properties[j].queueFlags) != 0)
			{
				// give goahead to make device and run program
				printf("    compute capability (bit %d) detected.\n", VK_QUEUE_COMPUTE_BIT);
				compute = 1;
				queue_count = queue_family_properties[j].queueCount;
			}
			if((VK_QUEUE_TRANSFER_BIT & queue_family_properties[j].queueFlags) != 0)
			{
				printf("    transfer capability (bit %d) detected.\n", VK_QUEUE_TRANSFER_BIT);
			}
			if((VK_QUEUE_SPARSE_BINDING_BIT & queue_family_properties[j].queueFlags) != 0)
			{
				printf("    sparse binding capability (bit %d) detected.\n", VK_QUEUE_SPARSE_BINDING_BIT);
			}
			if((VK_QUEUE_PROTECTED_BIT & queue_family_properties[j].queueFlags) != 0)
			{
				printf("    protected memory capability (bit %d) detected.\n", VK_QUEUE_PROTECTED_BIT);
			}

			// run compute demo only if we have compute capabilities
			if(compute != 1) {continue;}

			// create logical device
			const float queue_priority = 1.0f; // one normalized float per queue
			const VkDeviceQueueCreateInfo device_info =
			{
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,	// structure type
				0,											// extension ptr
				0,											// flags
				j,											// queue family idx
				1, /*queue_count,*/							// queue count
				&queue_priority
			};
			
            /*
			const VkDeviceCreateInfo =
			{
				VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,	// structure type
				0,										// extension pointer
				0,										// flags	
				1,										// queue create info count	
				&deviceQueueCreateInfo,					// pointer to array of create infos
				0,										// enabled layers
				0,										// ppEnabledLayerNames
				0,										// enabled extension count
				0,										// enabled extension names
				0										// enabled physical device features
			};
            */

		}
		// create logical device
		// create logical queue
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





    return die(win, 0);
}
