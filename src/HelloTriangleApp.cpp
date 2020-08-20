//
// Created by stuart on 17/08/2020.
//

#include "HelloTriangleApp.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::array<float, 4> CLEAR_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static auto get_binding_description() -> vk::VertexInputBindingDescription
    {
        vk::VertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);  // Bytes from one entry to the next

        // vk::VertexInputRate::eVertex = Move to the next data entry after each vertex
        // vk::VertexInputRate::eInstance = Move to the next data entry after each instance
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

const std::vector<Vertex> VERTICES = { { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
                                       { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
                                       { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
                                       { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f } } };

const std::vector<uint16_t> INDICES = { 0, 1, 2, 2, 3, 0 };

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

static auto readfile(const std::string& filename) -> std::vector<char>
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

void HelloTriangleApp::framebuffer_resize_callback(GLFWwindow* window, int width, int height)
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
    // Initialise GLFW
    glfwInit();

    // Tell GLFW not to create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Handling resized windows requires special care, so disable it
    //    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create GLFW window
    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan - Hello Triangle", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
}

void HelloTriangleApp::create_vulkan_instance()
{
    if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support())
    {
        throw std::runtime_error("Vulkan validation layers requested, but not available!");
    }

    // list_supported_extensions();

    // Fill a struct with some info about our application. This is technically
    // optional, but may provide some useful information to the driver in order
    // to optimise our specific application.
    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Vulkan - Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

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

    // What global validation layers to enable
    if (ENABLE_VALIDATION_LAYERS)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }
    else
    {
        instanceCreateInfo.enabledLayerCount = 0;
    }

    // Create the Vulkan instance
    m_instance = vk::createInstance(instanceCreateInfo);
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
        if (is_device_suitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }
}

void HelloTriangleApp::create_logical_device()
{
    QueueFamilyIndices indices = find_queue_families(m_physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

    vk::DeviceCreateInfo a{};

    vk::DeviceCreateInfo createInfo{};
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    // Instantiate the Logical Device
    m_device = m_physicalDevice.createDevice(createInfo);

    m_graphicsQueue = m_device.getQueue(indices.graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(indices.presentFamily.value(), 0);
}

void HelloTriangleApp::create_swap_chain()
{
    SwapChainSupportDetails swapChainSupport = query_swap_chain_support(m_physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.presentModes);
    vk::Extent2D extent = choose_swap_extent(swapChainSupport.capabilities);

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
    QueueFamilyIndices indices = find_queue_families(m_physicalDevice);
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

void HelloTriangleApp::create_image_views()
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

void HelloTriangleApp::create_render_pass()
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

    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;    // What layout to have before the
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

void HelloTriangleApp::create_descriptor_set_layout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;  // Should be the same as defined in shader
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    // NOTE: It is possible to for the shader variable to represent an array of uniform buffer
    //       objects, and descriptorCount specifies the number of values in the array. This could
    //       be used to specify a transformation for each of the bones in a skeleton for skeletal
    //       animation, for example.

    // We also need to specify in which shader stages the descriptor is going to be referenced.
    // The stageFlags field can be a combination of vk::ShaderStageFlagBits
    // or vk::ShaderStageFlagBits::eAllGraphics
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    // The pImmutableSamplers field is only relevent for image sampling related descriptors
    uboLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    m_descriptorSetLayout = m_device.createDescriptorSetLayout(layoutInfo);
}

void HelloTriangleApp::create_graphics_pipeline()
{
    /* Programmable Pipeline Stages */

    auto vertShaderCode = readfile("shaders/vert.spv");
    auto fragShaderCode = readfile("shaders/frag.spv");

    vk::ShaderModule vertShaderModule = create_shader_module(vertShaderCode);
    vk::ShaderModule fragShaderModule = create_shader_module(fragShaderCode);

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
    rasterizer.depthClampEnable = VK_FALSE;           // Should clamp fragments beyond near/far planes?
    rasterizer.rasterizerDiscardEnable = VK_FALSE;    // Should disable any output to framebuffer?
    rasterizer.polygonMode = vk::PolygonMode::eFill;  // How are fragments generated? (Fill, Line,
                                                      // Point)
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;        // Type of face culling to use
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;  // Specify the vertex order for faces to be
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
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Dynamic State

    // Pipeline Layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

    m_pipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

    /* Create Pipeline */

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages.data();

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;

    pipelineInfo.layout = m_pipelineLayout;

    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;  // Index of the subpass where this graphics pipeline will be used

    // Vulkan allows you to create a new graphics pipeline by deriving from an
    // existing pipeline
    //    pipelineInfo.basePipelineHandle = {};
    //    pipelineInfo.basePipelineIndex = -1;

    m_graphicsPipeline = m_device.createGraphicsPipeline({}, pipelineInfo).value;

    m_device.destroy(vertShaderModule);
    m_device.destroy(fragShaderModule);
}

void HelloTriangleApp::create_framebuffers()
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

void HelloTriangleApp::create_command_pool()
{
    QueueFamilyIndices queueFamilyIndices = find_queue_families(m_physicalDevice);

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    m_commandPool = m_device.createCommandPool(poolInfo);
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
                 vk::ImageTiling::eOptimal,
                 vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 m_textureImage,
                 m_textureImageMemory);

    transition_image_layout(
        m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    copy_buffer_to_image(
        stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    transition_image_layout(m_textureImage,
                            vk::Format::eR8G8B8A8Srgb,
                            vk::ImageLayout::eTransferDstOptimal,
                            vk::ImageLayout::eShaderReadOnlyOptimal);
    
    m_device.destroy(stagingBuffer);
    m_device.free(stagingBufferMemory);
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
    vk::DeviceSize bufferSize = sizeof(INDICES[0]) * INDICES.size();  // Buffer size in bytes

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
    vk::DescriptorPoolSize poolSize{};
    // The type of the descriptor
    poolSize.type = vk::DescriptorType::eUniformBuffer;
    // The number of descriptors of a given type which can be allocated in total from
    // a given pool (across all sets)
    poolSize.descriptorCount = static_cast<uint32_t>(m_swapChainImages.size());

    vk::DescriptorPoolCreateInfo poolInfo{};
    // The number of elements in the pPoolSizes array
    poolInfo.poolSizeCount = 1;
    // Pointer to an array containing no less than poolSizeCount elements containing descriptor types and a total number
    // of descriptors of that type that can be allocated from the pool
    poolInfo.pPoolSizes = &poolSize;

    // The number of descriptor sets that can be allocated from the pool
    poolInfo.maxSets = static_cast<uint32_t>(m_swapChainImages.size());

    m_descriptorPool = m_device.createDescriptorPool(poolInfo);
}

void HelloTriangleApp::create_descriptor_sets()
{
    std::vector<vk::DescriptorSetLayout> layouts(m_swapChainImages.size(), m_descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets = m_device.allocateDescriptorSets(allocInfo);  // TODO: Error point

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        vk::WriteDescriptorSet descriptorWrite{};
        descriptorWrite.dstSet = m_descriptorSets[i];  // The descriptor set to update
        descriptorWrite.dstBinding = 0;                // The Uniform binding specified in the shaders
        descriptorWrite.dstArrayElement = 0;

        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;

        descriptorWrite.pBufferInfo = &bufferInfo;

        m_device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
    }
}

void HelloTriangleApp::create_command_buffers()
{
    m_commandBuffers.resize(m_swapChainFramebuffers.size());

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

        // Basic drawing commands

        m_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);

        std::vector<vk::Buffer> vertexBuffers = { m_vertexBuffer };
        std::vector<vk::DeviceSize> offsets = { 0 };
        m_commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers.data(), offsets.data());

        m_commandBuffers[i].bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint16);

        m_commandBuffers[i].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);

        m_commandBuffers[i].drawIndexed(static_cast<uint32_t>(INDICES.size()), 1, 0, 0, 0);

        m_commandBuffers[i].endRenderPass();

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
    create_vulkan_instance();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_pool();
    create_texture_image();
    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();
}

void HelloTriangleApp::update_uniform_buffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj =
        glm::perspective(glm::radians(45.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 10.0f);

    // GLM was designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
    ubo.proj[1][1] *= -1.0f;

    void* data = m_device.mapMemory(m_uniformBuffersMemory[currentImage], 0, sizeof(ubo));
    memcpy(data, &ubo, sizeof(ubo));
    m_device.unmapMemory(m_uniformBuffersMemory[currentImage]);
}

void HelloTriangleApp::draw_frame()
{
    m_device.waitForFences(m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    vk::Result result = m_device.acquireNextImageKHR(
        m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], {}, &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        // The swap chain has become incompatible with the surface
        // and can no longer be used for rendering.
        // Usually happens after window resize.
        recreate_swap_chain();
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

    update_uniform_buffer(imageIndex);

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

    vk::PresentInfoKHR presentInfo{};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores.data();  // Which semaphore to wait on before
                                                            // presentation can happen

    std::vector<vk::SwapchainKHR> swapChains = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains.data();
    presentInfo.pImageIndices = &imageIndex;

    result = m_presentQueue.presentKHR(&presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_frameBufferResized)
    {
        m_frameBufferResized = false;
        recreate_swap_chain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApp::main_loop()
{
    // Check if user tried to close the window
    while (!glfwWindowShouldClose(m_window))
    {
        // Poll for window events
        glfwPollEvents();

        draw_frame();
    }

    m_device.waitIdle();
}

void HelloTriangleApp::cleanup_swap_chain()
{
    for (auto framebuffer : m_swapChainFramebuffers)
    {
        m_device.destroy(framebuffer, nullptr);
    }

    m_device.freeCommandBuffers(m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

    m_device.destroy(m_graphicsPipeline, nullptr);
    m_device.destroy(m_pipelineLayout, nullptr);
    m_device.destroy(m_renderPass, nullptr);

    // Destroy ImageViews
    for (auto imageView : m_swapChainImageViews)
    {
        m_device.destroy(imageView, nullptr);
    }

    // Destroy Swap Chain
    m_device.destroy(m_swapChain, nullptr);

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_device.destroy(m_uniformBuffers[i]);
        m_device.free(m_uniformBuffersMemory[i]);
    }

    m_device.destroy(m_descriptorPool);
}

void HelloTriangleApp::cleanup()
{
    cleanup_swap_chain();

    m_device.destroy(m_textureImage);
    m_device.free(m_textureImageMemory);
    
    m_device.destroy(m_descriptorSetLayout);

    m_device.destroy(m_vertexBuffer);
    // Free buffer memory
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

    // Destroy logical device
    m_device.destroy();

    // Destroy the window surface
    m_instance.destroy(m_surface);

    // Destroy Vulkan instance
    m_instance.destroy();

    // Destroy our GLFW window
    glfwDestroyWindow(m_window);

    // Terminate GLFW itself
    glfwTerminate();
}

void HelloTriangleApp::recreate_swap_chain()
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

    cleanup_swap_chain();

    create_swap_chain();
    create_image_views();  // Based on swap chain
    create_render_pass();  // Depends on swap chain image formats
    // Pipeline must be rebuilt because Viewport and Scissor rectangle
    // size are specified during graphics pipeline creation.
    // Note: Could be avoided by using dynamic state for the viewports and
    // scissor rectangles.
    create_graphics_pipeline();
    create_framebuffers();     // Depends on swap chain images
    create_uniform_buffers();  // Depends on swap chain images
    create_descriptor_pool();  // Depends on swap chain images
    create_descriptor_sets();  // Depends on swap chain images
    create_command_buffers();  // Depends on swap chain images
}

auto HelloTriangleApp::check_validation_layer_support() -> bool
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

void HelloTriangleApp::list_supported_extensions()
{
    // Query Vulkan for supported extensions
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

        vk::Bool32 presentSupport = false;
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

    // Query supported capabilities
    details.capabilities = device.getSurfaceCapabilitiesKHR(m_surface);

    // Query supported surface formats
    details.formats = device.getSurfaceFormatsKHR(m_surface);

    // Query supported presentation modes
    details.presentModes = device.getSurfacePresentModesKHR(m_surface);

    return details;
}

auto HelloTriangleApp::is_device_suitable(vk::PhysicalDevice device) -> bool
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

auto HelloTriangleApp::choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    -> vk::SurfaceFormatKHR
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

auto HelloTriangleApp::choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    -> vk::PresentModeKHR
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

    // Use Min/Max to clamp the values between the allowed minimum and
    // maximum extents that are supported
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

auto HelloTriangleApp::create_shader_module(const std::vector<char>& code) -> vk::ShaderModule
{
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

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
    allocInfo.commandPool = m_commandPool;
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

    m_device.free(m_commandPool, 1, &commandBuffer);
}

void HelloTriangleApp::create_image(uint32_t width,
                                    uint32_t height,
                                    vk::Format format,
                                    vk::ImageTiling tiling,
                                    const vk::ImageUsageFlags& usage,
                                    const vk::MemoryPropertyFlags& properties,
                                    vk::Image& image,
                                    vk::DeviceMemory& imageMemory)
{
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    // vk::ImageUsageFlagBits::eSampled allows shaders to access the image
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;

    m_textureImage = m_device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = m_device.getImageMemoryRequirements(m_textureImage);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

    m_textureImageMemory = m_device.allocateMemory(allocInfo);

    m_device.bindImageMemory(m_textureImage, m_textureImageMemory, 0);
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

void HelloTriangleApp::transition_image_layout(vk::Image image,
                                               vk::Format format,
                                               vk::ImageLayout oldLayout,
                                               vk::ImageLayout newLayout)
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
