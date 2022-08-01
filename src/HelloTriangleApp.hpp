#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <vector>

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
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;

    vk::CommandPool m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;
    std::vector<vk::Fence> m_imagesInFlight;
    size_t m_currentFrame = 0;

    bool m_frameBufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void initWindow();

    void createVulkanInstance();

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain();

    void createImageViews();

    void createRenderPass();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createCommandBuffers();

    void createSyncObjects();

    void initVulkan();

    void drawFrame();

    void mainLoop();

    void cleanupSwapChain() const;

    void cleanup() const;

    void recreateSwapChain();

    /* Checks if all of the requested layers are available */
    static auto checkValidationLayerSupport() -> bool;

    static void listSupportedExtensions();

    auto findQueueFamilies(vk::PhysicalDevice device) const -> QueueFamilyIndices;

    static auto checkDeviceExtensionSupport(vk::PhysicalDevice device) -> bool;

    auto querySwapChainSupport(vk::PhysicalDevice device) const -> SwapChainSupportDetails;

    auto isDeviceSuitable(vk::PhysicalDevice device) const -> bool;

    /* Surface Format = Color Depth */
    static auto chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) -> vk::SurfaceFormatKHR;

    /*
     * VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted are transferred to screen right away, which may result in
     * tearing. VK_PRESENT_MODE_FIFO_KHR: Swap chain is a queue where the display takes an image from the front of the
     * queue when the display is refreshed and inserts rendered images at the back of the queue. Similar to VSync in
     * modern games. VK_PRESENT_MODE_FIFO_RELAXED_KHR: Can result in screen tearing. VK_PRESENT_MODE_MAILBOX_KHR: Can be
     * used to implement triple buffering, which allows you to avoid tearing with significantly less latency issues than
     * standard VSync that used double buffering.
     */
    static auto chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) -> vk::PresentModeKHR;

    /* Swap Extent us the resolution of the swap chain images and its almost
     * always exactly equal to the resolution of the window that we're drawing to.
     */
    auto chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const -> vk::Extent2D;

    auto createShaderModule(const std::vector<char>& code) const -> vk::ShaderModule;
};
