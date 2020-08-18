//
// Created by stuart on 17/08/2020.
//

#ifndef _HELLOTRIANGLE_HELLTRIANGLEAPP_H
#define _HELLOTRIANGLE_HELLTRIANGLEAPP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// TODO: Look into Validation Layers
// TODO: Look into Extensions
// TODO: Look into Queue Families
// TODO: Look into Physical Devices
// TODO: Look into Physical Device Features
// TODO: Look into Logical Devices
// TODO: Look into Surfaces

struct QueueFamilyIndices;

class HelloTriangleApp
{
public:
    void run();
    
private:
    GLFWwindow* m_window;
    VkInstance m_instance;
    VkPhysicalDevice physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    
    VkSurfaceKHR m_surface;
    VkQueue m_presentQueue;
    
    void init_window();
    
    /* Checks if all of the requested layers are available */
    static bool check_validation_layer_support();
    static void list_supported_extensions();
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    void create_vulkan_instance();
    void create_surface();
    bool is_device_suitable(VkPhysicalDevice device);
    void pick_physical_device();
    void create_logical_device();
    void init_vulkan();
    
    void main_loop();
    void cleanup();
    
};

#endif //_HELLOTRIANGLE_HELLTRIANGLEAPP_H
