//
// Created by stuart on 17/08/2020.
//

#include "HelloTriangleApp.hpp"

#include <fstream>
#include <iostream>
#include <optional>
#include <set>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

const std::vector DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
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

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

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
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApp::initWindow()
{
    // Initialise GLFW
    glfwInit();

    // Tell GLFW not to create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Handling resized windows requires special care, so disable it
    //    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create GLFW window
    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan - Hello Triangle", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void HelloTriangleApp::createVulkanInstance()
{
#if _DEBUG
    if (!checkValidationLayerSupport())
    {
        throw std::runtime_error("Vulkan validation layers requested, but not available!");
    }
#endif

    // list_supported_extensions();

    // Fill a struct with some info about our application. This is technically
    // optional, but may provide some useful information to the driver in order
    // to optimise our specific application.
    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Vulkan - Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // This struct tells the Vulkan driver which global (entire program)
    // extensions and validation layers we want to use. This is NOT optional.
    vk::InstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.pApplicationInfo = &appInfo;

    // Specify the desired global extensions. Vulkan is a platform agnostic API,
    // which means we need a an extension to interface with the window system.
    // GLFW provides a handy function that returns the extensions it needs.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    instanceCreateInfo.enabledExtensionCount = glfwExtensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions;

#if _DEBUG
    // What global validation layers to enable
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#endif

    // Create the Vulkan instance
    m_instance = createInstance(instanceCreateInfo);
}

void HelloTriangleApp::createSurface()
{
    VkSurfaceKHR rawSurface = nullptr;
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &rawSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }

    m_surface = static_cast<vk::SurfaceKHR>(rawSurface);
}

void HelloTriangleApp::pickPhysicalDevice()
{
    // After initialising Vulkan, we need to look for and select
    // a graphics card in the system that supports the features we need.
    // In fact we can select any number of graphics cards and use them
    // simultaneously, but here we will stick with the first graphics card.

    // The graphics card we select will be stored in a 'vk::PhysicalDevice'
    // handle. This object will be implicitly destroyed when the 'vk::Instance'
    // is destroyed, so we don't need to do it ourselves in cleanup().

    // Query the instance for all physical devices
    std::vector<vk::PhysicalDevice> devices = m_instance.enumeratePhysicalDevices();

    // If there are 0 devices with Vulkan support then there is no point in
    // continuing
    if (devices.empty())
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    // Now we evaluate each of them and check if they are suitable for the
    // operations we want to perform, because not all graphics cards are created
    // equal.
    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }
}

void HelloTriangleApp::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        // This structure describes the number of queues we
        // want for a single queue family
        vk::DeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Next we need to specify the set of device features we'll be using.
    // These were queried for earlier with PhysicalDevice.getFeatures()
    vk::PhysicalDeviceFeatures deviceFeatures{};

    // First add pointers to the queue creation info and device features structs
    // The remainder of the information bears resemblance to
    // vk::InstanceCreateInfo struct and requires you to specify extensions and
    // validation layers. The difference is that these are device specific this
    // time. NOTE: Newer versions of Vulkan removed the distinction between
    // Instance and Device specific validation layers.
    //       This means that the enabledLayerCount and ppEnabledLayerNames
    //       fields of vk::CreateInfo are ignored by up-to-date implementations.
    //       However, it is still a good idea to set them anyway to be
    //       compatible with older implementations.

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

    // Instantiate the Logical Device
    m_device = m_physicalDevice.createDevice(createInfo);

    m_graphicsQueue = m_device.getQueue(indices.graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(indices.presentFamily.value(), 0);
}

void HelloTriangleApp::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Decide how many images to have in swap chain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // We should also make sure not to exceed the maximum number of images
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
    createInfo.imageArrayLayers = 1;                                   // Amount of layers each image consists of
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;  // The kind of operations
    // we'll use the swap chain
    // images for

    // Specify how to handle swap chain images that will be used across multiple
    // queue families.
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    std::vector<uint32_t> queueFamilyIndices = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        // An image is owned by one family queue at a time and ownership must be
        // explicitly transferred before using it in another queue family. This
        // option offers the best performance.
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }
    else
    {
        // Images can be used across multiple queue families without explicit
        // ownership transfers
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    // We can specify that a certain transform should be applied to images in
    // the swap chain if its supported, like a 90 degree clockwise rotation or
    // horizontal flip. To specify that you don't want any transform, simply
    // specify the current transformation.
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // Specifies if the alpha channel should be used for blending space with
    // other windows in the window system. You'll almost always want to simply
    // ignore the alpha channel, hence vk::CompositeAlphaFlagBitsKHR::eOpaque.
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    createInfo.presentMode = presentMode;
    // If TRUE we don't care about the color of pixels that are obscured, for
    // example because another window is in front of them.
    createInfo.clipped = VK_TRUE;

    // Its possible that your swap chain becomes invalid or unoptimised while
    // your application is running, for example because the window was resized.
    // In that case the swap chain actually needs to be recreated from scratch
    // and a reference the old one must be specified in this field.
    //    createInfo.oldSwapchain = {};

    m_swapChain = m_device.createSwapchainKHR(createInfo);

    // Get the swap chain images
    m_swapChainImages = m_device.getSwapchainImagesKHR(m_swapChain);

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void HelloTriangleApp::createImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.image = m_swapChainImages[i];
        // Specify how the image data should be interpreted
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = m_swapChainImageFormat;

        // The components field allows you to swizzle the color channels around.
        // Eg. You can map all the channels to the red channel for a monochrome
        // texture.
        //     You can also map constant values of 0 and 1 to a channel.
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;

        // The subresourceRange field describes what the image's purpose is and
        // which part of the image should be accessed
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        m_swapChainImageViews[i] = m_device.createImageView(createInfo);
    }
}

#if 0
void HelloTriangleApp::createRenderPass()
{
    /* Attachment Description */

    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;  // Should match the format
    // of the swap chain images
    colorAttachment.samples = vk::SampleCountFlagBits::e1;

    // What to do with the color/depth data in the attachment before/after
    // rendering
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    // What to do with the stencil data in the attachment before/after rendering
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;  // What layout to have before the
    // render pass begins
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;  // What layout to automatically
    // transition to once the render
    // pass finishes

    /* Subpasses & Attachment References */

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;  // Which attachment to reference by its index in the
    // attachment descriptions array
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    /* Render Pass */
    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};

    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    m_renderPass = m_device.createRenderPass(renderPassInfo);
}
#endif  // 0

void HelloTriangleApp::createGraphicsPipeline()
{
    /* Programmable Pipeline Stages */

    auto vertShaderCode = readFile("shaders/shader.vert.spv");
    auto fragShaderCode = readFile("shaders/shader.frag.spv");

    vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;  // Which pipeline stage the
    // shader will belong to

    // Specify the shader module containing the code, and the function to
    // invoke, known as the entrypoint. This means that its possible to combine
    // multiple fragment shaders into a single shader module and use different
    // entry points to differentiate between their behaviours.
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
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

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
    rasterizer.depthClampEnable = VK_FALSE;           // Should clamp fragments beyond near/far planes?
    rasterizer.rasterizerDiscardEnable = VK_FALSE;    // Should disable any output to framebuffer?
    rasterizer.polygonMode = vk::PolygonMode::eFill;  // How are fragments generated? (Fill, Line,
    // Point)
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;  // Type of face culling to use
    rasterizer.frontFace = vk::FrontFace::eClockwise;   // Specify the vertex order for faces to be
    // considered front-facing
    rasterizer.depthBiasEnable = VK_FALSE;  // The rasterizer can alter depth values by adding a constant
    // value or biasing them based on the fragments slope.

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

    // pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;  // Index of the subpass where this graphics pipeline will be used

    // Vulkan allows you to create a new graphics pipeline by deriving from an
    // existing pipeline
    //    pipelineInfo.basePipelineHandle = {};
    //    pipelineInfo.basePipelineIndex = -1;

    m_graphicsPipeline = m_device.createGraphicsPipeline({}, pipelineInfo).value;

    m_device.destroy(vertShaderModule);
    m_device.destroy(fragShaderModule);
}

#if 0
void HelloTriangleApp::createFramebuffers()
{
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
    {
        std::vector<vk::ImageView> attachments = { m_swapChainImageViews[i] };

        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        m_swapChainFramebuffers[i] = m_device.createFramebuffer(framebufferInfo);
    }
}
#endif  // 0

void HelloTriangleApp::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    m_commandPool = m_device.createCommandPool(poolInfo);
}

void HelloTriangleApp::createCommandBuffers()
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

#if 0
		vk::RenderPassBeginInfo renderPassInfo{};
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[i];

        renderPassInfo.renderArea.setOffset({ 0, 0 });
        renderPassInfo.renderArea.extent = m_swapChainExtent;

        vk::ClearValue clearValue;
        clearValue.setColor({ CLEAR_COLOR });

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        m_commandBuffers[i].beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
#endif  // 0

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

        m_commandBuffers[i].draw(3, 1, 0, 0);

#if 0
        m_commandBuffers[i].endRenderPass();
#endif

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

void HelloTriangleApp::createSyncObjects()
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

void HelloTriangleApp::initVulkan()
{
    createVulkanInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    // createRenderPass();
    createGraphicsPipeline();
    // createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApp::drawFrame()
{
    m_device.waitForFences(m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    vk::Result result = m_device.acquireNextImageKHR(m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], {}, &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        // The swap chain has become incompatible with the surface
        // and can no longer be used for rendering.
        // Usually happens after window resize.
        recreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    // Check if a previous frame is using this image (ie. there is its fence to
    // wait on)
    if (m_imagesInFlight[imageIndex])
    {
        m_device.waitForFences(1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    vk::SubmitInfo submitInfo{};

    vk::Semaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    m_device.resetFences(1, &m_inFlightFences[m_currentFrame]);

    m_graphicsQueue.submit(submitInfo, m_inFlightFences[m_currentFrame]);

    vk::PresentInfoKHR presentInfo{};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;  // Which semaphore to wait on before
    // presentation can happen

    vk::SwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = m_presentQueue.presentKHR(&presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_frameBufferResized)
    {
        m_frameBufferResized = false;
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApp::mainLoop()
{
    // Check if user tried to close the window
    while (!glfwWindowShouldClose(m_window))
    {
        // Poll for window events
        glfwPollEvents();

        drawFrame();
    }

    m_device.waitIdle();
}

void HelloTriangleApp::cleanupSwapChain() const
{
#if 0
	for (auto framebuffer : m_swapChainFramebuffers)
    {
        m_device.destroy(framebuffer, nullptr);
    }
#endif  // 0

    m_device.freeCommandBuffers(m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

    m_device.destroy(m_graphicsPipeline, nullptr);
    m_device.destroy(m_pipelineLayout, nullptr);
    // m_device.destroy(m_renderPass, nullptr);

    // Destroy ImageViews
    for (auto imageView : m_swapChainImageViews)
    {
        m_device.destroy(imageView, nullptr);
    }

    // Destroy Swap Chain
    m_device.destroy(m_swapChain, nullptr);
}

void HelloTriangleApp::cleanup() const
{
    cleanupSwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_device.destroy(m_imageAvailableSemaphores[i], nullptr);
        m_device.destroy(m_renderFinishedSemaphores[i], nullptr);
        m_device.destroy(m_inFlightFences[i], nullptr);
    }

    m_device.destroy(m_commandPool, nullptr);

    // Destroy logical device
    m_device.destroy(nullptr);

    // Destroy the window surface
    m_instance.destroy(m_surface, nullptr);

    // Destroy Vulkan instance
    m_instance.destroy(nullptr);

    // Destroy our GLFW window
    glfwDestroyWindow(m_window);

    // Terminate GLFW itself
    glfwTerminate();
}

void HelloTriangleApp::recreateSwapChain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    // We don't want to touch resources still in use
    m_device.waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();  // Based on swap chain
    // createRenderPass();  // Depends on swap chain image formats
    //  Pipeline must be rebuilt because Viewport and Scissor rectangle
    //  size are specified during graphics pipeline creation.
    //  Note: Could be avoided by using dynamic state for the viewports and
    //  scissor rectangles.
    createGraphicsPipeline();
    // createFramebuffers();    // Depends on swap chain images
    createCommandBuffers();  // Depends on swap chain images
}

bool HelloTriangleApp::checkValidationLayerSupport()
{
    // Query validation layers
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    // Now check if all the layers in VALIDATION_LAYERS exists in the
    // availableLayers list.
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
    // Query Vulkan for supported extensions
    std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

    std::cout << "Vulkan supported extensions:\n";

    for (const auto& extension : extensions)
    {
        std::cout << '\t' << extension.extensionName << std::endl;
    }
}

QueueFamilyIndices HelloTriangleApp::findQueueFamilies(const vk::PhysicalDevice device) const
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

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

bool HelloTriangleApp::checkDeviceExtensionSupport(const vk::PhysicalDevice device)
{
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails HelloTriangleApp::querySwapChainSupport(const vk::PhysicalDevice device) const
{
    SwapChainSupportDetails details;

    // Query supported capabilities
    details.capabilities = device.getSurfaceCapabilitiesKHR(m_surface);

    // Query supported surface formats
    details.formats = device.getSurfaceFormatsKHR(m_surface);

    // Query supported presentation modes
    details.presentModes = device.getSurfacePresentModesKHR(m_surface);

    return details;
}

bool HelloTriangleApp::isDeviceSuitable(const vk::PhysicalDevice device) const
{
    // To evaluate the suitability of a device we start by querying for some
    // details.

    // Basic device properties like the name, type and supported Vulkan version
    // can be queried using PhysicalDevice.getProperties(deviceProperties)
    //    vk::PhysicalDeviceProperties deviceProperties;
    //    device.getProperties(deviceProperties);

    // The support for optional features like texture compression, 64bit floats
    // and multi-viewport rendering (useful for VR) can be queried using
    // PhysicalDevice.getFeatures()
    //    vk::PhysicalDeviceFeatures deviceFeatures;
    //    device.getFeatures(deviceFeatures);

    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

vk::SurfaceFormatKHR HelloTriangleApp::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
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

vk::PresentModeKHR HelloTriangleApp::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
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

vk::Extent2D HelloTriangleApp::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    int width{};
    int height{};
    glfwGetFramebufferSize(m_window, &width, &height);

    vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    // Use Min/Max to clamp the values between the allowed minimum and
    // maximum extents that are supported

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

vk::ShaderModule HelloTriangleApp::createShaderModule(const std::vector<char>& code) const
{
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    return m_device.createShaderModule(createInfo);
}

int main(int argc, char** argv)
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
