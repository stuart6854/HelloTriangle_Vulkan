//
// Created by stuart on 17/08/2020.
//

#ifndef _HELLOTRIANGLE_HELLTRIANGLEAPP_H
#define _HELLOTRIANGLE_HELLTRIANGLEAPP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct QueueFamilyIndices;

class HelloTriangleApp
{
public:
    void run();
    
private:
    GLFWwindow* m_window;
    VkInstance m_instance;
    
    void init_window();
    
    /* Checks if all of the requested layers are available */
    static bool check_validation_layer_support();
    static void list_supported_extensions();
    static QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    void create_vulkan_instance();
    static bool is_device_suitable(VkPhysicalDevice device);
    void pick_physical_device();
    void init_vulkan();
    
    void main_loop();
    void cleanup();
    
};

#endif //_HELLOTRIANGLE_HELLTRIANGLEAPP_H
