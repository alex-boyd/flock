/*
   main.c
   initializes SDL and Vulkan, and runs our program 

   boilerplate adapted from:
   https://gist.github.com/Overv/7ac07356037592a121225172d7d78f2d
*/

/*
   void setupVulkan() {
		x createInstance();
		?  createDebugCallback();
		x createWindowSurface();
		x findPhysicalDevice();
		x checkSwapChainSupport();
		x findQueueFamilies();
		x createLogicalDevice();
		x createSemaphores();
		x createCommandPool();
		createVertexBuffer();
		createUniformBuffer();
		x createSwapChain();
		createRenderPass();
		createImageViews();
		createFramebuffers();
		createGraphicsPipeline();
		createDescriptorPool();
		createDescriptorSet();
		createCommandBuffers(); }

	void mainLoop() {
			updateUniformData();
			draw();

		}
	}

*/


// includes ------------------------------------------------------------------

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "perlin.h"

// globals and macros --------------------------------------------------------

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

// min and max from freebsd
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// main ----------------------------------------------------------------------

int main(int argc, char** argv)
{
    // sdl vars
    SDL_Window * win = NULL;
    SDL_Surface * surf = NULL;

    // vk vars
	VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    VkSemaphore image_available;
    VkSemaphore rendering_finished;
    VkCommandPool command_pool;
    VkSurfaceKHR vk_surf;
    VkSwapchainKHR swapchain;
    VkImage * swapchain_images = NULL;


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

    // get our device and make a queue ----------------------------------------

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

        if (graphics_bit != 1) { continue; } // failed to find gfx

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

        // create logical device
        physical_device = physical_devices[physical_device_index];
        vkCreateDevice(
                physical_devices[physical_device_index], // physical device
                &device_info,                            // device info above
                NULL,                                    // alloc callbk ptr
                &device );                               // logical device 

		// create logical queue
        vkGetDeviceQueue(
                device,
                queue_family_index,
                0,                  // queue_index (of queue_count, 1)
                &queue );

        // create semaphores ------------------------------------------------
        VkSemaphoreCreateInfo semaphore_info;
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if ( vkCreateSemaphore(
                    device, 
                    &semaphore_info, 
                    NULL, 
                    &image_available ) != VK_SUCCESS || 
             vkCreateSemaphore(
                    device, 
                    &semaphore_info, 
                    NULL, 
                    &rendering_finished ) != VK_SUCCESS )
        {
            printf("failed to create semaphores\n");
            die(win, 0);
        }

        // create command pool -----------------------------------------------
        VkCommandPoolCreateInfo command_pool_info;
        command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_info.queueFamilyIndex = queue_family_index;
        
        if ( vkCreateCommandPool( 
                    device,
                    &command_pool_info,
                    NULL,
                    &command_pool ) != VK_SUCCESS )
        {
            printf("Failed to create command pool.\n");
            die(win, 0);
        } 

        // KHR surface world // swapchain creation ---------------------------

        // create vulkan surface, check compatibility with queue family
        SDL_Vulkan_CreateSurface(win, instance, &vk_surf);
        VkBool32 khr_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(
                physical_devices[physical_device_index],
                queue_family_index,
                vk_surf,
                &khr_support );
        if (khr_support != VK_TRUE) { die(win, 1); }

        // get the surface's capabilities
        VkSurfaceCapabilitiesKHR surface_capabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physical_device,
                vk_surf,
                &surface_capabilities ) != VK_SUCCESS) 
        { 
            printf("Error: couldnt get surface capabilities. \n");
            die (win, 1); 
        }

        // get the surface's supported color formats
        uint32_t format_count;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(
                physical_device,
                vk_surf,
                &format_count,
                NULL ) != VK_SUCCESS) 
        { 
            printf("Error: couldnt get surface format count. \n");
            die (win, 1); 
        }

        VkSurfaceFormatKHR * surface_formats = 
            malloc(sizeof(VkSurfaceFormatKHR) * format_count); // not freed
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(
                physical_device,
                vk_surf,
                &format_count,
                surface_formats ) != VK_SUCCESS) 
        { 
            printf("Error: couldnt get surface formats . \n");
            die (win, 1); 
        }

        // find surface's supported presentation modes
        uint32_t present_mode_count;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(
                physical_device,
                vk_surf,
                &present_mode_count,
                NULL ) != VK_SUCCESS) 
        { 
            printf("Error: couldnt get presentation mode count. \n");
            die (win, 1); 
        }

        VkPresentModeKHR * present_modes = 
            malloc(sizeof(VkPresentModeKHR) * format_count); // not freed
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(
                physical_device,
                vk_surf,
                &present_mode_count,
                present_modes ) != VK_SUCCESS) 
        { 
            printf("Error: couldnt get presentation modes. \n");
            die (win, 1); 
        }

        // determine number of images in the swapchain
        uint32_t image_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount != 0 && 
                image_count > surface_capabilities.maxImageCount)
        {
            image_count = surface_capabilities.maxImageCount;
        }

        // select a surface format -------------------------
        VkSurfaceFormatKHR surface_format;

        // if undefined, try standard 32 bit color
        if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
        {
            surface_format.format = VK_FORMAT_R8G8B8A8_UNORM; 
            surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        }
        else 
        {
            // try to find standard 32 bit color
            int standard_found = 0;
            for (int i = 0; i < format_count; i++)
            {
                if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
                {
                    surface_format = surface_formats[i];
                    standard_found = 1;
                    break;
                }
            }

            // if you cant, just take the first available format
            if (standard_found != 0)
            {
                surface_format = surface_formats[0];
            }
        }
        
        // select a swapchain size ------------------------
        VkExtent2D swap_extent;

        if (surface_capabilities.currentExtent.width == -1)
        {
            swap_extent.width = MIN( 
                    MAX(SCREEN_WIDTH, surface_capabilities.minImageExtent.width), 
                    surface_capabilities.maxImageExtent.width);

            swap_extent.height = MIN( 
                    MAX(SCREEN_HEIGHT, surface_capabilities.minImageExtent.height), 
                    surface_capabilities.maxImageExtent.height);
        }

        // select a surface transform (hopefully none)  ----
        VkSurfaceTransformFlagBitsKHR surface_transform;
        if (surface_capabilities.supportedTransforms & 
                VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        } else
        {
            surface_transform = surface_capabilities.currentTransform;
        }

        // select a presentation mode (prefer mailbox -> triple buffered?)
        VkPresentModeKHR present_mode;
        int mailbox = 0;
        for(int i = 0; i < present_mode_count; i++)
        {
            if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = present_modes[i];
                mailbox = 1;
            }
        }
        if (mailbox == 0)
        {
            present_mode = VK_PRESENT_MODE_FIFO_KHR;
        }

        // actually create swapchain with given specifications ---------

        VkSwapchainCreateInfoKHR swapchain_info;
        swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_info.surface = vk_surf;
        swapchain_info.minImageCount = image_count;
        swapchain_info.imageFormat = surface_format.format;
        swapchain_info.imageColorSpace = surface_format.colorSpace;
		swapchain_info.imageExtent = swap_extent;
		swapchain_info.imageArrayLayers = 1;
		swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.queueFamilyIndexCount = 0;
		swapchain_info.pQueueFamilyIndices = NULL;
		swapchain_info.preTransform = surface_transform;
		swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_info.presentMode = present_mode;
		swapchain_info.clipped = VK_TRUE;
		swapchain_info.oldSwapchain = VK_NULL_HANDLE; // ?

        if ( vkCreateSwapchainKHR(
                    device, 
                    &swapchain_info,
                    NULL,
                    &swapchain ) != VK_SUCCESS )
        {
            die(win, 0);
        }

        // get swapchain image count, then set the image count  --------------
        uint32_t actual_image_count = 0;
        // get number
        if( vkGetSwapchainImagesKHR(
                    device, 
                    swapchain,
                    &actual_image_count,
                    swapchain_images) != VK_SUCCESS ||
            actual_image_count == 0)
        {
            die(win, 0);
        }

        // proceed with creating actual_image_count swapchain images
        swapchain_images = malloc( actual_image_count * sizeof(VkImage) );
        if( vkGetSwapchainImagesKHR(
                    device, 
                    swapchain,
                    &actual_image_count,
                    swapchain_images) != VK_SUCCESS )
        {
            die(win, 0);
        }

        // swapchain complete!

        // create vertex buffer ----------------------------------------------
        
        // placeholder triangle vertices


        // create uniform buffer ---------------------------------------------





                    

		// allocate memory
        //const VkDeviceSize memory_size;
        //for (uint32_t k = 0; k & lt;


		// make in/out buffers out of memory
                
		// command buffer
		// shaders??
		// compile shaders

		// free loop's resources
        free(queue_family_properties);

        SDL_Delay(3000);
    }

	// free resources
	free(physical_devices);
    //vkDestroyDevice(device);

    return die(win, 0);
}



