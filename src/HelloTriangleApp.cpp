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
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    
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
}

void HelloTriangleApp::main_loop()
{
    // Check if user tried to close the window
    while (!glfwWindowShouldClose(m_window))
    {
        // Poll for window events
        glfwPollEvents();
    }
}

void HelloTriangleApp::cleanup()
{
    for (auto framebuffer : m_swapChainFramebuffers
            )
    {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    
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
        VkExtent2D actualExtent = { WIDTH, HEIGHT };
        
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

