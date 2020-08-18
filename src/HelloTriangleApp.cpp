//
// Created by stuart on 17/08/2020.
//

#include "HelloTriangleApp.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>


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

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    
    bool is_complete()
    {
        return graphicsFamily.has_value();
    }
};

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

QueueFamilyIndices HelloTriangleApp::find_queue_families(VkPhysicalDevice device)
{
    QueueFamilyIndices indices{};
    
    uint32_t  queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for(const auto& queueFamily : queueFamilies)
    {
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }
        
        if(indices.is_complete())
        {
            break;
        }
        
        i++;
    }
    
    return indices;
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
    
    return indices.is_complete();
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
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    
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
            physicalDevice = device;
            break;
        }
    }
    
    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
    
}

void HelloTriangleApp::init_vulkan()
{
    create_vulkan_instance();
    pick_physical_device();
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
    // Destroy Vulkan instance
    vkDestroyInstance(m_instance, nullptr);
    
    // Destroy our GLFW window
    glfwDestroyWindow(m_window);
    
    // Terminate GLFW itself
    glfwTerminate();
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

