//
// Created by stuart on 17/08/2020.
//

#include "HelloTriangleApp.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <fstream>
#include <iostream>
#include <optional>
#include <set>

#include <glm/glm.hpp>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

const std::vector DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    auto is_complete() const -> bool
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;     // Color Depth
    std::vector<vk::PresentModeKHR> presentModes;  // Conditions for "swapping" images to the screen
};

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::array<float, 4> CLEAR_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static auto get_binding_description() -> vk::VertexInputBindingDescription
    {
        vk::VertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);

        bindingDesc.inputRate = vk::VertexInputRate::eVertex;

        return bindingDesc;
    }

    static auto get_attrib_descriptions() -> std::array<vk::VertexInputAttributeDescription, 2>
    {
        std::array<vk::VertexInputAttributeDescription, 2> attribDescriptions{};

        attribDescriptions[0].binding = 0;
        attribDescriptions[0].location = 0;
        attribDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attribDescriptions[0].offset = offsetof(Vertex, pos);

        attribDescriptions[1].binding = 0;
        attribDescriptions[1].location = 1;
        attribDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attribDescriptions[1].offset = offsetof(Vertex, color);

        return attribDescriptions;
    }
};

const std::vector<Vertex> VERTICES = {
    { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
    { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f } },
};

const std::vector<uint16_t> INDICES = {
    0, 1, 2, 2, 3, 0,
};

static auto read_shader_binary(const std::string& filename) -> std::vector<uint32_t>
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file!");
    }

    size_t fileSize = file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    file.close();

    return buffer;
}

void HelloTriangleApp::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* app = static_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
    app->m_frameBufferResized = true;
}

void HelloTriangleApp::run()
{
    init_window();
    init_vulkan();
    main_loop();
    cleanup();
}

void HelloTriangleApp::init_window()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan - Hello Triangle", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void HelloTriangleApp::create_instance()
{
#if _DEBUG
    if (!check_validation_layer_support())
    {
        throw std::runtime_error("Vulkan validation layers requested, but not available!");
    }
#endif

    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Vulkan - Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    vk::InstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    instanceCreateInfo.enabledExtensionCount = glfwExtensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions;

#if _DEBUG
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#endif

    m_instance = createInstance(instanceCreateInfo);
}

void HelloTriangleApp::create_surface()
{
    VkSurfaceKHR rawSurface = nullptr;
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &rawSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }

    m_surface = static_cast<vk::SurfaceKHR>(rawSurface);
}

void HelloTriangleApp::pick_physical_device()
{
    std::vector<vk::PhysicalDevice> devices = m_instance.enumeratePhysicalDevices();

    if (devices.empty())
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    for (const auto& device : devices)
    {
        if (is_device_suitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }
}

void HelloTriangleApp::create_device()
{
    QueueFamilyIndices indices = find_queue_families(m_physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};

    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
    dynamicRenderingFeatures.dynamicRendering = true;

    vk::DeviceCreateInfo createInfo{};
    createInfo.pNext = &dynamicRenderingFeatures;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

#ifdef _DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#endif

    m_device = m_physicalDevice.createDevice(createInfo);

    m_graphicsQueue = m_device.getQueue(indices.graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(indices.presentFamily.value(), 0);
}

void HelloTriangleApp::create_allocator()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.instance = m_instance;
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;

    vmaCreateAllocator(&allocatorInfo, &m_allocator);
}

void HelloTriangleApp::create_swapchain()
{
    SwapChainSupportDetails swapChainSupport = query_swap_chain_support(m_physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.presentModes);
    vk::Extent2D extent = choose_swap_extent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = m_surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    QueueFamilyIndices indices = find_queue_families(m_physicalDevice);
    std::vector<uint32_t> queueFamilyIndices = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }
    else
    {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    m_swapChain = m_device.createSwapchainKHR(createInfo);

    m_swapChainImages = m_device.getSwapchainImagesKHR(m_swapChain);

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void HelloTriangleApp::create_image_views()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = m_swapChainImageFormat;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        m_swapChainImageViews[i] = m_device.createImageView(createInfo);
    }
}

void HelloTriangleApp::create_graphics_pipeline()
{
    /* Programmable Pipeline Stages */

    auto vertShaderCode = read_shader_binary("shaders/shader.vert.spv");
    auto fragShaderCode = read_shader_binary("shaders/shader.frag.spv");

    vk::ShaderModule vertShaderModule = create_shader_module(vertShaderCode);
    vk::ShaderModule fragShaderModule = create_shader_module(fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

    /* Fixed Function Pipeline Stages */

    // Vertex Input
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

    auto bindingDescription = Vertex::get_binding_description();
    auto attribDescriptions = Vertex::get_attrib_descriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attribDescriptions.data();

    // Input Assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and Scissors
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = m_swapChainExtent;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    // Point)
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    // Depth and Stencil Testing

    // Color Blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Dynamic State

    // Pipeline Layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};

    m_pipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

    /* Create Pipeline */

    vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &m_swapChainImageFormat;

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages.data();

    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.subpass = 0;

    m_graphicsPipeline = m_device.createGraphicsPipeline({}, pipelineInfo).value;

    m_device.destroy(vertShaderModule);
    m_device.destroy(fragShaderModule);
}

void HelloTriangleApp::create_command_pool()
{
    QueueFamilyIndices queueFamilyIndices = find_queue_families(m_physicalDevice);

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    m_commandPool = m_device.createCommandPool(poolInfo);
}

void HelloTriangleApp::create_vertex_buffer()
{
    vk::DeviceSize bufferSize = sizeof(VERTICES[0]) * VERTICES.size();  // Buffer size in bytes

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    create_buffer(bufferSize,
                  vk::BufferUsageFlagBits::eTransferSrc,
                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                  stagingBuffer,
                  stagingBufferMemory);

    void* data = m_device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, VERTICES.data(), static_cast<size_t>(bufferSize));
    m_device.unmapMemory(stagingBufferMemory);

    create_buffer(bufferSize,
                  vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                  vk::MemoryPropertyFlagBits::eDeviceLocal,
                  m_vertexBuffer,
                  m_vertexBufferMemory);

    copy_buffer(stagingBuffer, m_vertexBuffer, bufferSize);

    m_device.destroy(stagingBuffer);
    m_device.free(stagingBufferMemory);
}

void HelloTriangleApp::create_index_buffer()
{
    vk::DeviceSize bufferSize = sizeof(INDICES[0]) * INDICES.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    create_buffer(bufferSize,
                  vk::BufferUsageFlagBits::eTransferSrc,
                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                  stagingBuffer,
                  stagingBufferMemory);

    void* data = m_device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, INDICES.data(), static_cast<size_t>(bufferSize));
    m_device.unmapMemory(stagingBufferMemory);

    create_buffer(bufferSize,
                  vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                  vk::MemoryPropertyFlagBits::eDeviceLocal,
                  m_indexBuffer,
                  m_indexBufferMemory);

    copy_buffer(stagingBuffer, m_indexBuffer, bufferSize);

    m_device.destroy(stagingBuffer);
    m_device.free(stagingBufferMemory);
}

void HelloTriangleApp::create_command_buffers()
{
    m_commandBuffers.resize(m_swapChainImages.size());

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (m_device.allocateCommandBuffers(&allocInfo, m_commandBuffers.data()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for (size_t i = 0; i < m_commandBuffers.size(); i++)
    {
        vk::CommandBufferBeginInfo beginInfo{};

        if (m_commandBuffers[i].begin(&beginInfo) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        {
            vk::ImageMemoryBarrier barrier{};
            barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            barrier.oldLayout = vk::ImageLayout::eUndefined;
            barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
            barrier.image = m_swapChainImages[i];
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            m_commandBuffers[i].pipelineBarrier(
                vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, barrier);
        }

        vk::RenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.imageView = m_swapChainImageViews[i];
        colorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimal;
        colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachmentInfo.clearValue.color.setFloat32({ 0.232f, 0.304f, 0.540f, 1.0f });

        vk::RenderingInfo renderingInfo{};
        renderingInfo.renderArea.offset = vk::Offset2D(0, 0);
        renderingInfo.renderArea.extent = vk::Extent2D(WIDTH, HEIGHT);
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachmentInfo;

        m_commandBuffers[i].beginRendering(renderingInfo);

        // Basic drawing commands

        m_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);

        std::vector<vk::Buffer> vertexBuffers = { m_vertexBuffer };
        std::vector<vk::DeviceSize> offsets = { 0 };
        m_commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers.data(), offsets.data());

        m_commandBuffers[i].bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint16);

        m_commandBuffers[i].drawIndexed(static_cast<uint32_t>(INDICES.size()), 1, 0, 0, 0);

        m_commandBuffers[i].endRendering();

        {
            vk::ImageMemoryBarrier barrier{};
            barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
            barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
            barrier.image = m_swapChainImages[i];
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            m_commandBuffers[i].pipelineBarrier(
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, barrier);
        }

        m_commandBuffers[i].end();
    }
}

void HelloTriangleApp::create_sync_objects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(m_swapChainImages.size(), {});

    vk::SemaphoreCreateInfo semaphoreInfo{};

    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (m_device.createSemaphore(&semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != vk::Result::eSuccess ||
            m_device.createSemaphore(&semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != vk::Result::eSuccess ||
            m_device.createFence(&fenceInfo, nullptr, &m_inFlightFences[i]) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create synchronisation objects for a frame!");
        }
    }
}

void HelloTriangleApp::init_vulkan()
{
    create_instance();
    create_surface();
    pick_physical_device();
    create_device();
    create_swapchain();
    create_image_views();
    create_graphics_pipeline();
    create_command_pool();
    create_vertex_buffer();
    create_index_buffer();
    create_command_buffers();
    create_sync_objects();
}

void HelloTriangleApp::draw_frame()
{
    m_device.waitForFences(m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    vk::Result result = m_device.acquireNextImageKHR(m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], {}, &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreate_swapchain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    if (m_imagesInFlight[imageIndex])
    {
        m_device.waitForFences(1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    vk::SubmitInfo submitInfo{};

    std::vector<vk::Semaphore> waitSemaphores = { m_imageAvailableSemaphores[m_currentFrame] };
    std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    std::vector<vk::Semaphore> signalSemaphores = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    m_device.resetFences(1, &m_inFlightFences[m_currentFrame]);

    m_graphicsQueue.submit(submitInfo, m_inFlightFences[m_currentFrame]);

    std::vector<vk::SwapchainKHR> swapChains = { m_swapChain };
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains.data();
    presentInfo.pImageIndices = &imageIndex;

    result = m_presentQueue.presentKHR(&presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_frameBufferResized)
    {
        m_frameBufferResized = false;
        recreate_swapchain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApp::main_loop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        draw_frame();
    }

    m_device.waitIdle();
}

void HelloTriangleApp::clean_swap_chain() const
{
    m_device.freeCommandBuffers(m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

    m_device.destroy(m_graphicsPipeline, nullptr);
    m_device.destroy(m_pipelineLayout, nullptr);

    for (auto imageView : m_swapChainImageViews)
    {
        m_device.destroy(imageView, nullptr);
    }

    m_device.destroy(m_swapChain, nullptr);
}

void HelloTriangleApp::cleanup() const
{
    clean_swap_chain();

    m_device.destroy(m_vertexBuffer);
    m_device.free(m_vertexBufferMemory);

    m_device.destroy(m_indexBuffer);
    m_device.free(m_indexBufferMemory);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_device.destroy(m_imageAvailableSemaphores[i]);
        m_device.destroy(m_renderFinishedSemaphores[i]);
        m_device.destroy(m_inFlightFences[i]);
    }

    m_device.destroy(m_commandPool);

    vmaDestroyAllocator(m_allocator);

    m_device.destroy();
    m_instance.destroy(m_surface);
    m_instance.destroy();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void HelloTriangleApp::recreate_swapchain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    m_device.waitIdle();

    clean_swap_chain();

    create_swapchain();
    create_image_views();
    create_graphics_pipeline();
    create_command_buffers();
}

auto HelloTriangleApp::check_validation_layer_support() -> bool
{
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char* layerName : VALIDATION_LAYERS)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

void HelloTriangleApp::listSupportedExtensions()
{
    std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

    std::cout << "Vulkan supported extensions:\n";

    for (const auto& extension : extensions)
    {
        std::cout << '\t' << extension.extensionName << std::endl;
    }
}

auto HelloTriangleApp::find_queue_families(vk::PhysicalDevice device) -> QueueFamilyIndices
{
    QueueFamilyIndices indices{};

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }

        vk::Bool32 presentSupport = VK_FALSE;
        device.getSurfaceSupportKHR(i, m_surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.is_complete())
        {
            break;
        }

        i++;
    }

    return indices;
}

auto HelloTriangleApp::check_device_extension_support(vk::PhysicalDevice device) -> bool
{
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

auto HelloTriangleApp::query_swap_chain_support(vk::PhysicalDevice device) -> SwapChainSupportDetails
{
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(m_surface);
    details.formats = device.getSurfaceFormatsKHR(m_surface);
    details.presentModes = device.getSurfacePresentModesKHR(m_surface);

    return details;
}

auto HelloTriangleApp::is_device_suitable(vk::PhysicalDevice device) -> bool
{
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensionsSupported = check_device_extension_support(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.is_complete() && extensionsSupported && swapChainAdequate;
}

auto HelloTriangleApp::choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats) -> vk::SurfaceFormatKHR
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

auto HelloTriangleApp::choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& availablePresentModes) -> vk::PresentModeKHR
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    // Default to double buffering (VSync)
    return vk::PresentModeKHR::eFifo;
}

auto HelloTriangleApp::choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities) -> vk::Extent2D
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    int width{};
    int height{};
    glfwGetFramebufferSize(m_window, &width, &height);

    vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

auto HelloTriangleApp::create_shader_module(const std::vector<uint32_t>& code) -> vk::ShaderModule
{
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.setCode(code);

    return m_device.createShaderModule(createInfo);
}

auto HelloTriangleApp::find_memory_type(uint32_t typeFilter, const vk::MemoryPropertyFlags& properties) -> uint32_t
{
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    m_physicalDevice.getMemoryProperties(&memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void HelloTriangleApp::create_buffer(vk::DeviceSize size,
                                     const vk::BufferUsageFlags& usage,
                                     const vk::MemoryPropertyFlags& properties,
                                     vk::Buffer& buffer,
                                     vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = m_device.createBuffer(bufferInfo);

    vk::MemoryRequirements memoryRequirements = m_device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memoryRequirements.memoryTypeBits, properties);

    bufferMemory = m_device.allocateMemory(allocInfo);

    m_device.bindBufferMemory(buffer, bufferMemory, 0);
}

void HelloTriangleApp::copy_buffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);

    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    m_graphicsQueue.submit(1, &submitInfo, {});
    m_graphicsQueue.waitIdle();

    m_device.free(m_commandPool, 1, &commandBuffer);
}

auto main(int argc, char** argv) -> int
{
    HelloTriangleApp app{};

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
