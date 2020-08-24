//
// Created by stuart on 17/08/2020.
//

#ifndef _HELLOTRIANGLE_HELLOTRIANGLEAPP_H
#define _HELLOTRIANGLE_HELLOTRIANGLEAPP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VULKAN_HPP_NO_NODISCARD_WARNINGS
#include <vulkan/vulkan.hpp>

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
    GLFWwindow* m_window;

    vk::Instance m_instance;
    vk::PhysicalDevice m_physicalDevice;
    vk::Device m_device;
    vk::Queue m_graphicsQueue;

    vk::SurfaceKHR m_surface;
    vk::Queue m_presentQueue;

    vk::SwapchainKHR m_swapChain;
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;

    std::vector<vk::ImageView> m_swapChainImageViews;
    std::vector<vk::Framebuffer> m_swapChainFramebuffers;

    vk::RenderPass m_renderPass;
    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;

    vk::Buffer m_vertexBuffer;
    vk::DeviceMemory m_vertexBufferMemory;
    vk::Buffer m_indexBuffer;
    vk::DeviceMemory m_indexBufferMemory;

    std::vector<vk::Buffer> m_uniformBuffers;
    std::vector<vk::DeviceMemory> m_uniformBuffersMemory;

    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    vk::CommandPool m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;
    std::vector<vk::Fence> m_imagesInFlight;
    size_t m_currentFrame = 0;

    vk::Image m_textureImage;
    vk::DeviceMemory m_textureImageMemory;
    vk::ImageView m_textureImageView;
    vk::Sampler m_textureSampler;

    bool m_frameBufferResized = false;

    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

    void init_window();

    void create_vulkan_instance();

    void create_surface();

    void pick_physical_device();

    void create_logical_device();

    void create_swap_chain();

    void create_image_views();

    void create_render_pass();

    void create_descriptor_set_layout();

    void create_graphics_pipeline();

    void create_framebuffers();

    void create_command_pool();

    void create_texture_image();

    void create_texture_image_view();
    
    void create_texture_sampler();
    
    void create_vertex_buffer();

    void create_index_buffer();

    void create_uniform_buffers();

    void create_descriptor_pool();

    void create_descriptor_sets();

    void create_command_buffers();

    void create_sync_objects();

    void init_vulkan();

    void update_uniform_buffer(uint32_t currentImage);

    void draw_frame();

    void main_loop();

    void cleanup_swap_chain();

    void cleanup();

    void recreate_swap_chain();

    /* Checks if all of the requested layers are available */
    static auto check_validation_layer_support() -> bool;

    static void list_supported_extensions();

    auto find_queue_families(vk::PhysicalDevice device) -> QueueFamilyIndices;

    static auto check_device_extension_support(vk::PhysicalDevice device) -> bool;

    auto query_swap_chain_support(vk::PhysicalDevice device) -> SwapChainSupportDetails;

    auto is_device_suitable(vk::PhysicalDevice device) -> bool;

    /* Surface Format = Color Depth */
    static auto choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
        -> vk::SurfaceFormatKHR;

    /*
     * vk::PresentModeKHR::eImmediate: Images submitted are transferred to screen right away, which may result in
     * tearing. vk::PresentModeKHR::eFifo: Swap chain is a queue where the display takes an image from the front of the
     *                               queue when the display is refreshed and inserts rendered images at the back of the
     * queue. Similar to VSync in modern games. vk::PresentModeKHR::eFifoRelaxed: Can result in screen tearing.
     * Vvk::PresentModeKHR::eMailbox: Can be used to implement triple buffering, which allows you to avoid tearing with
     * significantly less latency issues than standard VSync that used double buffering.
     */
    static auto choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
        -> vk::PresentModeKHR;

    /* Swap Extent us the resolution of the swap chain images and its almost
     * always exactly equal to the resolution of the window that we're drawing to.
     */
    auto choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities) -> vk::Extent2D;

    auto create_shader_module(const std::vector<char>& code) -> vk::ShaderModule;

    auto find_memory_type(uint32_t typeFilter, const vk::MemoryPropertyFlags& properties) -> uint32_t;

    void create_buffer(vk::DeviceSize size,
                       const vk::BufferUsageFlags& usage,
                       const vk::MemoryPropertyFlags& properties,
                       vk::Buffer& buffer,
                       vk::DeviceMemory& bufferMemory);

    void copy_buffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

    auto begin_single_time_commands() -> vk::CommandBuffer;

    void end_single_time_commands(vk::CommandBuffer commandBuffer);

    void create_image(uint32_t width,
                      uint32_t height,
                      vk::Format format,
                      vk::ImageTiling tiling,
                      const vk::ImageUsageFlags& usage,
                      const vk::MemoryPropertyFlags& properties,
                      vk::Image& image,
                      vk::DeviceMemory& imageMemory);

    void copy_buffer_to_image(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

    void transition_image_layout(vk::Image image,
                                 vk::Format format,
                                 vk::ImageLayout oldLayout,
                                 vk::ImageLayout newLayout);
    
    auto create_image_view(vk::Image image, vk::Format format) -> vk::ImageView;
};

#endif  //_HELLOTRIANGLE_HELLOTRIANGLEAPP_H
