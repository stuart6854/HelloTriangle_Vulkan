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
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 800;

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
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

const std::array<float, 4> CLEAR_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static auto get_binding_description() -> vk::VertexInputBindingDescription
    {
        vk::VertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);

        bindingDesc.inputRate = vk::VertexInputRate::eVertex;

        return bindingDesc;
    }

    static auto get_attrib_descriptions() -> std::array<vk::VertexInputAttributeDescription, 3>
    {
        std::array<vk::VertexInputAttributeDescription, 3> attribDescriptions{};

        attribDescriptions[0].binding = 0;
        attribDescriptions[0].location = 0;
        attribDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attribDescriptions[0].offset = offsetof(Vertex, pos);

        attribDescriptions[1].binding = 0;
        attribDescriptions[1].location = 1;
        attribDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attribDescriptions[1].offset = offsetof(Vertex, color);

        attribDescriptions[2].binding = 0;
        attribDescriptions[2].location = 2;
        attribDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attribDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attribDescriptions;
    }
};

const std::vector<Vertex> VERTICES = { { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                                       { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                                       { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                                       { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } } };

const std::vector<uint16_t> INDICES = {
    0, 1, 2, 2, 3, 0,
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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
    deviceFeatures.setSamplerAnisotropy(VK_TRUE);

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

    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_swapChainImageViews[i] = create_image_view(m_swapChainImages[i], m_swapChainImageFormat);
    }
}

void HelloTriangleApp::prepare_frames()
{
    vk::SemaphoreCreateInfo semaphoreInfo{};

    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (auto& frame : m_frames)
    {
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
        frame.cmdPool = m_device.createCommandPool(poolInfo);

        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setCommandPool(frame.cmdPool);
        allocInfo.setCommandBufferCount(1);
        allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        frame.cmd = m_device.allocateCommandBuffers(allocInfo)[0];

        frame.imageReadySemaphore = m_device.createSemaphore(semaphoreInfo);
        frame.renderDoneSemaphore = m_device.createSemaphore(semaphoreInfo);

        frame.cmdExecFence = m_device.createFence(fenceInfo);
    }
}

void HelloTriangleApp::create_offscreen_pass_resources()
{
    create_image(WIDTH,
                 HEIGHT,
                 vk::Format::eR8G8B8A8Srgb,
                 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                 m_offscreenPass.image,
                 m_offscreenPass.allocation);

    m_offscreenPass.view = create_image_view(m_offscreenPass.image, vk::Format::eR8G8B8A8Srgb);

    vk::DescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.setBinding(0);
    uboLayoutBinding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    uboLayoutBinding.setDescriptorCount(1);
    uboLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex);

    vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.setBinding(1);
    samplerLayoutBinding.setDescriptorCount(1);
    samplerLayoutBinding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    samplerLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        uboLayoutBinding,
        samplerLayoutBinding,
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.setBindings(bindings);
    m_offscreenPass.descriptorSetLayout = m_device.createDescriptorSetLayout(layoutInfo);

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.setDescriptorPool(m_descriptorPool);
    allocInfo.setSetLayouts(m_offscreenPass.descriptorSetLayout);
    allocInfo.setDescriptorSetCount(1);
    m_offscreenPass.descriptorSet = m_device.allocateDescriptorSets(allocInfo)[0];

    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.setBuffer(m_uniformBuffers[0]);
    bufferInfo.setRange(sizeof(UniformBufferObject));

    vk::WriteDescriptorSet writeUbo{};
    writeUbo.setDstSet(m_offscreenPass.descriptorSet);
    writeUbo.setDstBinding(0);
    writeUbo.setDescriptorCount(1);
    writeUbo.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    writeUbo.setBufferInfo(bufferInfo);

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.setImageView(m_texture.view);
    imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    imageInfo.setSampler(m_sampler);

    vk::WriteDescriptorSet writeTexture{};
    writeTexture.setDstSet(m_offscreenPass.descriptorSet);
    writeTexture.setDstBinding(1);
    writeTexture.setDescriptorCount(1);
    writeTexture.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    writeTexture.setImageInfo(imageInfo);
    m_device.updateDescriptorSets({ writeUbo, writeTexture }, {});
}

void HelloTriangleApp::create_final_pass_resources()
{
    vk::DescriptorSetLayoutBinding binding{};
    binding.setBinding(0);
    binding.setDescriptorCount(1);
    binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.setBindings(binding);
    m_finalPass.descriptorSetLayout = m_device.createDescriptorSetLayout(layoutInfo);

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.setDescriptorPool(m_descriptorPool);
    allocInfo.setSetLayouts(m_finalPass.descriptorSetLayout);
    allocInfo.setDescriptorSetCount(1);
    m_finalPass.descriptorSet = m_device.allocateDescriptorSets(allocInfo)[0];

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.setImageView(m_offscreenPass.view);
    imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    imageInfo.setSampler(m_sampler);

    vk::WriteDescriptorSet write{};
    write.setDstSet(m_finalPass.descriptorSet);
    write.setDstBinding(0);
    write.setDescriptorCount(1);
    write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    write.setImageInfo(imageInfo);
    m_device.updateDescriptorSets(write, {});
}

void HelloTriangleApp::create_offscreen_pipeline()
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
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
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
    pipelineLayoutInfo.setSetLayouts(m_offscreenPass.descriptorSetLayout);

    m_offscreenPass.pipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

    /* Create Pipeline */

    auto colorFormats = { vk::Format::eR8G8B8A8Srgb };
    vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.setColorAttachmentFormats(colorFormats);

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
    pipelineInfo.layout = m_offscreenPass.pipelineLayout;
    pipelineInfo.subpass = 0;

    m_offscreenPass.pipeline = m_device.createGraphicsPipeline({}, pipelineInfo).value;

    m_device.destroy(vertShaderModule);
    m_device.destroy(fragShaderModule);
}

void HelloTriangleApp::create_final_pipeline()
{
    /* Programmable Pipeline Stages */

    auto vertShaderCode = read_shader_binary("shaders/fullscreen_quad.vert.spv");
    auto fragShaderCode = read_shader_binary("shaders/fullscreen_quad.frag.spv");

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
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
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
    pipelineLayoutInfo.setSetLayouts(m_finalPass.descriptorSetLayout);

    m_finalPass.pipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

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
    pipelineInfo.layout = m_finalPass.pipelineLayout;
    pipelineInfo.subpass = 0;

    m_finalPass.pipeline = m_device.createGraphicsPipeline({}, pipelineInfo).value;

    m_device.destroy(vertShaderModule);
    m_device.destroy(fragShaderModule);
}

void HelloTriangleApp::create_texture_image()
{
    int texWidth{};
    int texHeight{};
    int texChannels{};
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image!");
    }

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    create_buffer(imageSize,
                  vk::BufferUsageFlagBits::eTransferSrc,
                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                  stagingBuffer,
                  stagingBufferMemory);

    void* data = m_device.mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    m_device.unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    imageInfo.format = vk::Format::eR8G8B8A8Srgb;

    imageInfo.tiling = vk::ImageTiling::eOptimal;

    imageInfo.initialLayout = vk::ImageLayout::eUndefined;

    // vk::ImageUsageFlagBits::eSampled allows shaders to access the image
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    imageInfo.samples = vk::SampleCountFlagBits::e1;

    create_image(texWidth,
                 texHeight,
                 vk::Format::eR8G8B8A8Srgb,
                 vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                 m_texture.image,
                 m_texture.allocation);

    transition_image_layout(m_texture.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    copy_buffer_to_image(stagingBuffer, m_texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    transition_image_layout(
        m_texture.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    m_device.destroy(stagingBuffer);
    m_device.free(stagingBufferMemory);
}

void HelloTriangleApp::create_texture_image_view()
{
    m_texture.view = create_image_view(m_texture.image, vk::Format::eR8G8B8A8Srgb);
}

void HelloTriangleApp::create_sampler()
{
    vk::SamplerCreateInfo createInfo{};
    createInfo.setMagFilter(vk::Filter::eLinear);
    createInfo.setMinFilter(vk::Filter::eLinear);
    createInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    createInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    createInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    createInfo.setAnisotropyEnable(VK_TRUE);
    createInfo.setMaxAnisotropy(16.0f);
    createInfo.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    createInfo.setUnnormalizedCoordinates(VK_FALSE);
    createInfo.setCompareEnable(VK_FALSE);
    createInfo.setCompareOp(vk::CompareOp::eAlways);
    createInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    createInfo.setMipLodBias(0.0f);
    createInfo.setMinLod(0.0f);
    createInfo.setMaxLod(0.0f);

    m_sampler = m_device.createSampler(createInfo);
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

void HelloTriangleApp::create_uniform_buffers()
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(m_swapChainImages.size());
    m_uniformBuffersMemory.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        create_buffer(bufferSize,
                      vk::BufferUsageFlagBits::eUniformBuffer,
                      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                      m_uniformBuffers[i],
                      m_uniformBuffersMemory[i]);
    }
}

void HelloTriangleApp::create_descriptor_pool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = 10;
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = 10;

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 10;

    m_descriptorPool = m_device.createDescriptorPool(poolInfo);
}

void HelloTriangleApp::init_vulkan()
{
    create_instance();
    create_surface();
    pick_physical_device();
    create_device();
    create_allocator();
    create_descriptor_pool();
    create_sampler();
    create_swapchain();

    prepare_frames();

    create_uniform_buffers();

    create_texture_image();
    create_texture_image_view();

    create_offscreen_pass_resources();
    create_offscreen_pipeline();

    create_final_pass_resources();
    create_final_pipeline();

    create_vertex_buffer();
    create_index_buffer();
}

void HelloTriangleApp::update_uniform_buffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);  //  glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(0, 0, -2));  // glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(60.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 10.0f);

    // GLM was designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
    ubo.proj[1][1] *= -1.0f;

    void* data = m_device.mapMemory(m_uniformBuffersMemory[currentImage], 0, sizeof(ubo));
    memcpy(data, &ubo, sizeof(ubo));
    m_device.unmapMemory(m_uniformBuffersMemory[currentImage]);
}

void HelloTriangleApp::record_cmd_buffer(const vk::CommandBuffer& cmd)
{
    /* Offscreen */

    vk::ImageMemoryBarrier barrier{};
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.image = m_offscreenPass.image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, barrier);

    vk::RenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.imageView = m_offscreenPass.view;
    colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfo.clearValue.color.setFloat32({ 0.232f, 0.304f, 0.540f, 1.0f });

    vk::RenderingInfo renderingInfo{};
    renderingInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderingInfo.renderArea.extent = vk::Extent2D(WIDTH, HEIGHT);
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;

    cmd.beginRendering(renderingInfo);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_offscreenPass.pipeline);

    std::vector<vk::Buffer> vertexBuffers = { m_vertexBuffer };
    std::vector<vk::DeviceSize> offsets = { 0 };
    cmd.bindVertexBuffers(0, 1, vertexBuffers.data(), offsets.data());

    cmd.bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint16);

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, m_offscreenPass.pipelineLayout, 0, 1, &m_offscreenPass.descriptorSet, 0, nullptr);

    cmd.drawIndexed(static_cast<uint32_t>(INDICES.size()), 1, 0, 0, 0);

    cmd.endRendering();

    barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.image = m_offscreenPass.image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

    /* Swapchain */

    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.image = m_swapChainImages[m_imageIndex];
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, barrier);

    colorAttachmentInfo.imageView = m_swapChainImageViews[m_imageIndex];
    colorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimal;
    colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfo.clearValue.color.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });

    renderingInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderingInfo.renderArea.extent = vk::Extent2D(WIDTH, HEIGHT);
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;

    cmd.beginRendering(renderingInfo);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_finalPass.pipeline);

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_finalPass.pipelineLayout, 0, m_finalPass.descriptorSet, {});

    cmd.draw(3, 1, 0, 0);

    cmd.endRendering();

    barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    barrier.dstAccessMask = {};
    barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
    barrier.image = m_swapChainImages[m_imageIndex];
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, barrier);
}

void HelloTriangleApp::draw_frame()
{
    m_frameIndex = (m_frameIndex + 1) % m_frames.size();
    auto& frame = m_frames[m_frameIndex];

    m_device.waitForFences(frame.cmdExecFence, VK_TRUE, UINT64_MAX);
    m_device.resetFences(frame.cmdExecFence);

    m_device.resetCommandPool(frame.cmdPool);

    m_imageIndex = m_device.acquireNextImageKHR(m_swapChain, UINT64_MAX, frame.imageReadySemaphore, {}).value;

    update_uniform_buffer(m_imageIndex);

    vk::CommandBufferBeginInfo beginInfo{};
    frame.cmd.begin(beginInfo);

    record_cmd_buffer(frame.cmd);

    frame.cmd.end();

    vk::SubmitInfo submitInfo{};

    std::vector<vk::Semaphore> waitSemaphores = { frame.imageReadySemaphore };
    std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.setCommandBuffers(frame.cmd);

    std::vector<vk::Semaphore> signalSemaphores = { frame.renderDoneSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    m_graphicsQueue.submit(submitInfo, frame.cmdExecFence);

    std::vector<vk::SwapchainKHR> swapChains = { m_swapChain };
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains.data();
    presentInfo.pImageIndices = &m_imageIndex;

    m_presentQueue.presentKHR(&presentInfo);
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

void HelloTriangleApp::cleanup() const
{
    m_device.waitIdle();

    m_device.destroy(m_offscreenPass.pipeline, nullptr);
    m_device.destroy(m_offscreenPass.pipelineLayout, nullptr);

    m_device.destroy(m_finalPass.pipeline, nullptr);
    m_device.destroy(m_finalPass.pipelineLayout, nullptr);

    for (auto imageView : m_swapChainImageViews)
    {
        m_device.destroy(imageView, nullptr);
    }

    m_device.destroy(m_swapChain, nullptr);

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_device.destroy(m_uniformBuffers[i]);
        m_device.free(m_uniformBuffersMemory[i]);
    }

    m_device.destroy(m_descriptorPool);

    m_device.destroy(m_sampler);
    m_device.destroy(m_texture.view);
    vmaDestroyImage(m_allocator, m_texture.image, m_texture.allocation);

    m_device.destroy(m_offscreenPass.descriptorSetLayout);
    m_device.destroy(m_finalPass.descriptorSetLayout);

    m_device.destroy(m_offscreenPass.view);
    vmaDestroyImage(m_allocator, m_offscreenPass.image, m_offscreenPass.allocation);

    m_device.destroy(m_vertexBuffer);
    m_device.free(m_vertexBufferMemory);

    m_device.destroy(m_indexBuffer);
    m_device.free(m_indexBufferMemory);

    for (auto& frame : m_frames)
    {
        m_device.destroy(frame.imageReadySemaphore);
        m_device.destroy(frame.renderDoneSemaphore);
        m_device.destroy(frame.cmdExecFence);

        m_device.destroy(frame.cmdPool);
    }

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

    create_swapchain();
    create_offscreen_pipeline();
    create_descriptor_pool();
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

    auto deviceFeatures = device.getFeatures();

    return indices.is_complete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
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
    vk::CommandBuffer commandBuffer = begin_single_time_commands();

    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    end_single_time_commands(commandBuffer);
}

auto HelloTriangleApp::begin_single_time_commands() -> vk::CommandBuffer
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = m_frames[m_frameIndex].cmdPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void HelloTriangleApp::end_single_time_commands(vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    m_graphicsQueue.submit(1, &submitInfo, {});
    m_graphicsQueue.waitIdle();

    m_device.free(m_frames[m_frameIndex].cmdPool, 1, &commandBuffer);
}

void HelloTriangleApp::create_image(
    uint32_t width, uint32_t height, vk::Format format, const vk::ImageUsageFlags& usage, vk::Image& image, VmaAllocation& allocation)
{
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkImageCreateInfo vkImageInfo = imageInfo;
    VkImage vkImage;
    vmaCreateImage(m_allocator, &vkImageInfo, &allocInfo, &vkImage, &allocation, nullptr);

    image = vkImage;
}

void HelloTriangleApp::copy_buffer_to_image(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
    vk::CommandBuffer commandBuffer = begin_single_time_commands();

    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D(0, 0, 0);
    region.imageExtent = vk::Extent3D(width, height, 1);

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

    end_single_time_commands(commandBuffer);
}

void HelloTriangleApp::transition_image_layout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::CommandBuffer commandBuffer = begin_single_time_commands();

    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::runtime_error("Unsupported layout transition!");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);

    end_single_time_commands(commandBuffer);
}

auto HelloTriangleApp::create_image_view(vk::Image image, vk::Format format) -> vk::ImageView
{
    vk::ImageViewCreateInfo createInfo{};
    createInfo.image = image;
    createInfo.viewType = vk::ImageViewType::e2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    return m_device.createImageView(createInfo);
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
