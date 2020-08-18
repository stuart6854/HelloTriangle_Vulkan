//
// Created by stuart on 17/08/2020.
//

#ifndef _HELLOTRIANGLE_HELLOTRIANGLEAPP_H
#define _HELLOTRIANGLE_HELLOTRIANGLEAPP_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <vector>

// TODO: Look into Validation Layers
// TODO: Look into Extensions
// TODO: Look into Queue Families
// TODO: Look into Physical Devices
// TODO: Look into Physical Device Features
// TODO: Look into Logical Devices
// TODO: Look into Surfaces

struct QueueFamilyIndices;
struct SwapChainSupportDetails;

class HelloTriangleApp
{
public:
    void run();

private:
    GLFWwindow *m_window;
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    
    VkSurfaceKHR m_surface;
    VkQueue m_presentQueue;
    
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    
    std::vector<VkImageView> m_swapChainImageViews;
    
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    
    void init_window();
    
    void create_vulkan_instance();
    
    void create_surface();
    
    void pick_physical_device();
    
    void create_logical_device();
    
    void create_swap_chain();
    
    void create_image_views();
    
    void create_render_pass();
    
    void create_graphics_pipeline();
    
    void init_vulkan();
    
    void main_loop();
    
    void cleanup();
    
    
    /* Checks if all of the requested layers are available */
    static bool check_validation_layer_support();
    
    static void list_supported_extensions();
    
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    
    bool check_device_extension_support(VkPhysicalDevice device);
    
    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
    
    bool is_device_suitable(VkPhysicalDevice device);
    
    /* Surface Format = Color Depth */
    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    
    /*
     * VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted are transferred to screen right away, which may result in tearing.
     * VK_PRESENT_MODE_FIFO_KHR: Swap chain is a queue where the display takes an image from the front of the queue when
     *                           the display is refreshed and inserts rendered images at the back of the queue.
     *                           Similar to VSync in modern games.
     * VK_PRESENT_MODE_FIFO_RELAXED_KHR: Can result in screen tearing.
     * VK_PRESENT_MODE_MAILBOX_KHR: Can be used to implement triple buffering, which allows you to avoid tearing with
     *                              significantly less latency issues than standard VSync that used double buffering.
     */
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    
    /* Swap Extent us the resolution of the swap chain images and its almost
     * always exactly equal to the resolution of the window that we're drawing to.
     */
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities);
    
    VkShaderModule create_shader_module(const std::vector<char> &code);
    
};

#endif //_HELLOTRIANGLE_HELLOTRIANGLEAPP_H
