//
// Created by stuart on 17/08/2020.
//

#include "HelloTriangleApp.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>

#include <glm/glm.hpp>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char *> VALIDATION_LAYERS =
        {
                "VK_LAYER_KHRONOS_validation"
        };

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

const std::vector<const char *> DEVICE_EXTENSIONS =
        {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    
    bool is_complete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats; // Color Depth
    std::vector<VkPresentModeKHR> presentModes; // Conditions for "swapping" images to the screen
};

const int MAX_FRAMES_IN_FLIGHT = 2;

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
    
    static VkVertexInputBindingDescription get_binding_description()
    {
        VkVertexInputBindingDescription bindingDesc { };
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex); // Bytes from one entry to the next
        
        // VK_VERTEX_INPUT_RATE_VERTEX = Move to the next data entry after each vertex
        // VK_VERTEX_INPUT_RATE_INSTANCE = Move to the next data entry after each instance
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return bindingDesc;
    }
    
    static std::array<VkVertexInputAttributeDescription, 2> get_attrib_descriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attribDescriptions { };
        
        attribDescriptions[0].binding = 0;
        attribDescriptions[0].location = 0;
        attribDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attribDescriptions[0].offset = offsetof(Vertex, pos);
        
        attribDescriptions[1].binding = 0;
        attribDescriptions[1].location = 1;
        attribDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribDescriptions[1].offset = offsetof(Vertex, color);
        
        return attribDescriptions;
    }
};

const std::vector<Vertex> vertices =
        {
                {{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
                {{ 0.5f,  -0.5f }, { 0.0f, 1.0f, 0.0f }},
                {{ 0.5f,  0.5f },  { 0.0f, 0.0f, 1.0f }},
                {{ -0.5f, 0.5f },  { 1.0f, 1.0f, 1.0f }}
        };

const std::vector<uint16_t> indices =
        {
            0, 1, 2, 2, 3, 0
        };

static std::vector<char> readfile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file!");
    }
    
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    
    file.close();
    
    return buffer;
}

void HelloTriangleApp::framebuffer_resize_callback(GLFWwindow *window, int width, int height)
{
    auto app = reinterpret_cast<HelloTriangleApp *>(glfwGetWindowUserPointer(window));
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
    
    //list_supported_extensions();
    
    // Fill a struct with some info about our application. This is technically optional,
    // but may provide some useful information to the driver in order to optimise our
    // specific application.
    VkApplicationInfo appInfo { };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan - Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    // This struct tells the Vulkan driver which global (entire program) extensions and
    // validation layers we want to use. This is NOT optional.
    VkInstanceCreateInfo createInfo { };
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // Specify the desired global extensions. Vulkan is a platform agnostic API,
    // which means we need a an extension to interface with the window system.
    // GLFW provides a handy function that returns the extensions it needs.
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    
    // What global validation layers to enable
    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }
    
    // Create the Vulkan instance
    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void HelloTriangleApp::create_surface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void HelloTriangleApp::pick_physical_device()
{
    // After initialising Vulkan, we need to look for and select
    // a graphics card in the system that supports the features we need.
    // In fact we can select any number of graphics cards and use them simultaneously,
    // but here we will stick with the first graphics card.
    
    // The graphics card we select will be stored in a 'VkPhysicalDevice' handle.
    // This object will be implicitly destroyed when the 'VkInstance' is destroyed,
    // so we don't need to do it ourselves in cleanup().
    m_physicalDevice = VK_NULL_HANDLE;
    
    // Listing the graphics card is very similar to listing extensions and
    // starts with querying just the number
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    
    // If there are 0 devices with Vulkan support then there is no point in continuing
    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    
    // Otherwise we can now allocate an array to hold all the 'VkPhysicalDevice' handles
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    
    // Now we evaluate each of them and check if they are suitable for the operations
    // we want to perform, because not all graphics cards are created equal.
    for (const auto &device : devices
            )
    {
        if (is_device_suitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }
    
    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
    
}

void HelloTriangleApp::create_logical_device()
{
    QueueFamilyIndices indices = find_queue_families(m_physicalDevice);
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies
            )
    {
        // This structure describes the number of queues we
        // want for a single queue family
        VkDeviceQueueCreateInfo queueCreateInfo { };
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // Next we need to specify the set of device features we'll be using.
    // These were queries for earlier with vkGetPhysicalDeviceFeatures()
    VkPhysicalDeviceFeatures deviceFeatures { };
    
    VkDeviceCreateInfo createInfo { };
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // First add pointers to the queue creation info and device features structs
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    // The remainder of the information bears resemblance to VkInstanceCreateInfo
    // struct and requires you to specify extensions and validation layers.
    // The difference is that these are device specific this time.
    // NOTE: Newer versions of Vulkan removed the distinction between Instance and Device specific validation layers.
    //       This means that the enabledLayerCount and ppEnabledLayerNames fields of VkCreateInfo are ignored by
    //       up-to-date implementations. However, it is still a good idea to set them anyway to be compatible with
    //       older implementations.
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
    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Logical Device!");
    }
    
    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

void HelloTriangleApp::create_swap_chain()
{
    SwapChainSupportDetails swapChainSupport = query_swap_chain_support(m_physicalDevice);
    
    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.presentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities);
    
    // Decide how many images to have in swap chain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    
    // We should also make sure not to exceed the maximum number of images
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo { };
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // Amount of layers each image consists of
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // The kind of operations we'll use the swap chain images for
    
    // Specify how to handle swap chain images that will be used across multiple queue families.
    QueueFamilyIndices indices = find_queue_families(m_physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    
    if (indices.graphicsFamily != indices.presentFamily)
    {
        // An image is owned by one family queue at a time and ownership must be explicitly transferred before using it in another queue family.
        // This option offers the best performance.
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        // Images can be used across multiple queue families without explicit ownership transfers
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    
    // We can specify that a certain transform should be applied to images in the swap chain if its supported,
    // like a 90 degree clockwise rotation or horizontal flip.
    // To specify that you don't want any transform, simply specify the current transformation.
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    
    // Specifies if the alpha channel should be used for blending space with other windows in the window system.
    // You'll almost always want to simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    
    createInfo.presentMode = presentMode;
    // If TRUE we don't care about the color of pixels that are obscured, for example
    // because another window is in front of them.
    createInfo.clipped = VK_TRUE;
    
    // Its possible that your swap chain becomes invalid or unoptimised while your application is running,
    // for example because the window was resized. In that case the swap chain actually needs to be recreated
    // from scratch and a reference the old one must be specified in this field.
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Swap Chain!");
    }
    
    // Get the swap chain images
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
    
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void HelloTriangleApp::create_image_views()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());
    
    for (size_t i = 0; i < m_swapChainImages.size(); i++
            )
    {
        VkImageViewCreateInfo createInfo { };
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        // Specify how the image data should be interpreted
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainImageFormat;
        
        // The components field allows you to swizzle the color channels around.
        // Eg. You can map all the channels to the red channel for a monochrome texture.
        //     You can also map constant values of 0 and 1 to a channel.
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        // The subresourceRange field describes what the image's purpose is and which part
        // of the image should be accessed
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create the image views!");
        }
    }
}

void HelloTriangleApp::create_render_pass()
{
    /* Attachment Description */
    
    VkAttachmentDescription colorAttachment { };
    colorAttachment.format = m_swapChainImageFormat; // Should match the format of the swap chain images
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    
    // What to do with the color/depth data in the attachment before/after rendering
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    
    // What to do with the stencil data in the attachment before/after rendering
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // WHat layout to have before the render pass begins
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // What layout to automatically transition to once the render pass finishes
    
    /* Subpasses & Attachment References */
    
    VkAttachmentReference colorAttachmentRef { };
    colorAttachmentRef.attachment = 0; // Which attachment to reference by its index in the attachment descriptions array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass { };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    /* Render Pass */
    VkRenderPassCreateInfo renderPassInfo { };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    VkSubpassDependency dependency { };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void HelloTriangleApp::create_graphics_pipeline()
{
    /* Programmable Pipeline Stages */
    
    auto vertShaderCode = readfile("shaders/vert.spv");
    auto fragShaderCode = readfile("shaders/frag.spv");
    
    VkShaderModule vertShaderModule = create_shader_module(vertShaderCode);
    VkShaderModule fragShaderModule = create_shader_module(fragShaderCode);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo { };
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Which pipeline stage the shader will belong to
    
    // Specify the shader module containing the code, and the function to invoke, known as the entrypoint.
    // This means that its possible to combine multiple fragment shaders into a single shader module and
    // use different entry points to differentiate between their behaviours.
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo { };
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    
    /* Fixed Function Pipeline Stages */
    
    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo { };
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    auto bindingDescription = Vertex::get_binding_description();
    auto attribDescriptions = Vertex::get_attrib_descriptions();
    
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attribDescriptions.data();
    
    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly { };
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // Viewport and Scissors
    VkViewport viewport { };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_swapChainExtent.width;
    viewport.height = (float) m_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor { };
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapChainExtent;
    
    VkPipelineViewportStateCreateInfo viewportState { };
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer { };
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; // Should clamp fragments beyond near/far planes?
    rasterizer.rasterizerDiscardEnable = VK_FALSE; // Should disable any output to framebuffer?
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // How are fragments generated? (Fill, Line, Point)
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Type of face culling to use
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Specify the vertex order for faces to be considered front-facing
    rasterizer.depthBiasEnable = VK_FALSE; // The rasterizer can alter depth values by adding a constant value or biasing them based on the fragments slope.
    
    //Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling { };
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // Depth and Stencil Testing
    
    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment { };
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending { };
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // Dynamic State
    
    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo { };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    
    /* Create Pipeline */
    
    VkGraphicsPipelineCreateInfo pipelineInfo { };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    
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
    pipelineInfo.subpass = 0; // Index of the subpass where this graphics pipeline will be used
    
    // Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline
//    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
//    pipelineInfo.basePipelineIndex = -1;
    
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
    
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
}

void HelloTriangleApp::create_framebuffers()
{
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++
            )
    {
        VkImageView attachments[] = {
                m_swapChainImageViews[i]
        };
        
        VkFramebufferCreateInfo framebufferInfo { };
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void HelloTriangleApp::create_command_pool()
{
    QueueFamilyIndices queueFamilyIndices = find_queue_families(m_physicalDevice);
    
    VkCommandPoolCreateInfo poolInfo { };
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0; // Optional
    
    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void HelloTriangleApp::create_vertex_buffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size(); // Buffer size in bytes
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer,
                  stagingBufferMemory
                 );
    
    void *data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    create_buffer(bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  m_vertexBuffer,
                  m_vertexBufferMemory
                 );
    
    copy_buffer(stagingBuffer, m_vertexBuffer, bufferSize);
    
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void HelloTriangleApp::create_index_buffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size(); // Buffer size in bytes
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer,
                  stagingBufferMemory
                 );
    
    void *data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    create_buffer(bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  m_indexBuffer,
                  m_indexBufferMemory
                 );
    
    copy_buffer(stagingBuffer, m_indexBuffer, bufferSize);
    
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void HelloTriangleApp::create_command_buffers()
{
    m_commandBuffers.resize(m_swapChainFramebuffers.size());
    
    VkCommandBufferAllocateInfo allocInfo { };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) m_commandBuffers.size();
    
    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
    
    for (size_t i = 0; i < m_commandBuffers.size(); i++
            )
    {
        VkCommandBufferBeginInfo beginInfo { };
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }
        
        VkRenderPassBeginInfo renderPassInfo { };
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
        
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapChainExtent;
        
        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // Basic drawing commands
        
        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
        
        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
        
        vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        
        vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        
        vkCmdEndRenderPass(m_commandBuffers[i]);
        
        if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

void HelloTriangleApp::create_sync_objects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);
    
    VkSemaphoreCreateInfo semaphoreInfo { };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo { };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++
            )
    {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS
            || vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
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
    create_graphics_pipeline();
    create_framebuffers();
    create_command_pool();
    create_vertex_buffer();
    create_index_buffer();
    create_command_buffers();
    create_sync_objects();
}


void HelloTriangleApp::draw_frame()
{
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // The swap chain has become incompatible with the surface
        // and can no longer be used for rendering.
        // Usually happens after window resize.
        recreate_swap_chain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    // Check if a previous frame is using this image (ie. there is its fence to wait on)
    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];
    
    
    VkSubmitInfo submitInfo { };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
    
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR presentInfo { };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Which semaphore to wait on before presentation can happen
    
    VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_frameBufferResized)
    {
        m_frameBufferResized = false;
        recreate_swap_chain();
    }
    else if (result != VK_SUCCESS)
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
    
    vkDeviceWaitIdle(m_device);
}

void HelloTriangleApp::cleanup_swap_chain()
{
    for (auto framebuffer : m_swapChainFramebuffers
            )
    {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    
    vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
    
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    
    // Destroy ImageViews
    for (auto imageView : m_swapChainImageViews
            )
    {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    
    // Destroy Swap Chain
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}

void HelloTriangleApp::cleanup()
{
    cleanup_swap_chain();
    
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    // Free buffer memory
    vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
    
    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++
            )
    {
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    }
    
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    
    // Destroy logical device
    vkDestroyDevice(m_device, nullptr);
    
    // Destroy the window surface
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    
    // Destroy Vulkan instance
    vkDestroyInstance(m_instance, nullptr);
    
    // Destroy our GLFW window
    glfwDestroyWindow(m_window);
    
    // Terminate GLFW itself
    glfwTerminate();
}


void HelloTriangleApp::recreate_swap_chain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }
    
    // We don't want to touch resources still in use
    vkDeviceWaitIdle(m_device);
    
    cleanup_swap_chain();
    
    create_swap_chain();
    create_image_views(); // Based on swap chain
    create_render_pass(); // Depends on swap chain image formats
    // Pipeline must be rebuilt because Viewport and Scissor rectangle
    // size are specified during graphics pipeline creation.
    // Note: Could be avoided by using dynamic state for the viewports and scissor rectangles.
    create_graphics_pipeline();
    create_framebuffers(); // Depends on swap chain images
    create_command_buffers(); // Depends on swap chain images
}

bool HelloTriangleApp::check_validation_layer_support()
{
    // Get layer count
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    // Allocate an array to hold all layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    
    // Query validation layers
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    // Now check if all the layers in VALIDATION_LAYERS exists in the availableLayers list.
    for (const char *layerName : VALIDATION_LAYERS
            )
    {
        bool layerFound = false;
        
        for (const auto &layerProperties : availableLayers
                )
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
    // First we need to know how many extensions there are
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    
    // Allocate an array to hold all extensions
    std::vector<VkExtensionProperties> extensions(extensionCount);
    
    // Finally we can query the extension details
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    
    std::cout << "Vulkan supported extensions:\n";
    
    for (const auto &extension : extensions
            )
    {
        std::cout << '\t' << extension.extensionName << std::endl;
    }
}

QueueFamilyIndices HelloTriangleApp::find_queue_families(VkPhysicalDevice device)
{
    QueueFamilyIndices indices { };
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto &queueFamily : queueFamilies
            )
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        
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

bool HelloTriangleApp::check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
    
    for (const auto &extension : availableExtensions
            )
    {
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

SwapChainSupportDetails HelloTriangleApp::query_swap_chain_support(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    
    // Query supported capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
    
    // Query supported surface formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }
    
    // Query supported presentation modes
    uint32_t presentModesCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModesCount, nullptr);
    
    if (presentModesCount != 0)
    {
        details.presentModes.resize(presentModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModesCount, details.presentModes.data());
    }
    
    return details;
}

bool HelloTriangleApp::is_device_suitable(VkPhysicalDevice device)
{
    // To evaluate the suitability of a device we start by querying for some details.
    
    // Basic device properties like the name, type and supported Vulkan version can
    // be queried using vkGetPhysicalDeviceProperties()
//    VkPhysicalDeviceProperties deviceProperties;
//    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    
    // The support for optional features like texture compression, 64bit floats and multi-viewport
    // rendering (useful for VR) can be queried using vkGetPhysicalDeviceFeatures()
//    VkPhysicalDeviceFeatures deviceFeatures;
//    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
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

VkSurfaceFormatKHR HelloTriangleApp::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats
            )
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApp::choose_swap_present_mode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes
            )
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
    
    // Default to double buffering (VSync)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApp::choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        
        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };
        
        // Use Min/Max to clamp the values between the allowed minimum and maximum extents that are supported
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        
        return actualExtent;
    }
}

VkShaderModule HelloTriangleApp::create_shader_module(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo { };
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }
    
    return shaderModule;
}

uint32_t HelloTriangleApp::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);
    
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++
            )
    {
        if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

void HelloTriangleApp::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo { };
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }
    
    VkMemoryRequirements memoryRequirements { };
    vkGetBufferMemoryRequirements(m_device, buffer, &memoryRequirements);
    
    VkMemoryAllocateInfo allocInfo { };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memoryRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }
    
    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void HelloTriangleApp::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo { };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo { };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkBufferCopy copyRegion { };
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo { };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);
    
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

int main(int argc, char **argv)
{
    HelloTriangleApp app { };
    
    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

