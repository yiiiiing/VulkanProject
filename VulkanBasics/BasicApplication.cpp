//
// Created by Ruiying on 17.9.2021.
//
#include <iostream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include "BasicApplication.h"
#include "VulkanHelperFunctions.h"

void BasicApplication::InitialApplication(int windowWidth, int windowHeight, const char *windowName) {
    InitWindow(windowWidth, windowHeight, windowName);
    InitVulkan();
}


void BasicApplication::RunApplication() {
    // create command buffers
    CreateCommandBuffers();
    MainLoop();
    CleanUp();
}

void BasicApplication::CleanUp() {
    // destroy the object
    DestroyObjects();
    DestroyTextures();

    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
    // destroy frame buffers
    for (auto framebuffer : m_swapChainFrameBuffers) {
        vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
    }

    // destroy image views
    for (VkImageView imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_logicalDevice, imageView, nullptr);
    }

    // destroy the swap chain before the device
    vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
    vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

    // destroy the logical device
    vkDestroyDevice(m_logicalDevice, nullptr);
    if (m_enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(m_Instance, m_debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(m_Instance, m_windowSurface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void BasicApplication::InitWindow(int windowWidth, int windowHeight, const char* windowName) {
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, windowName, nullptr, nullptr);
}

void  BasicApplication::InitVulkan() {
    // first, create m_instance because debugMessenger and physical device requiring valid vulkan instance
    CreateVulkanInstance();
    // add debug messenger to vulkan instance
    SetupDebugMessenger();

    // create window surface (must before physical device pickup)
    CreateWindowSurface();

    // select physical device to use
    PickPhysicalDevice();

    CreateLogicalDevice();

    // create swap chain
    CreateSwapChain();

    // create image views
    CreateImageViewsForSwapChain();

    // create render pass, must before creating graphics pipeline
    CreateRenderPass();

    // create frame buffers
    CreateFrameBuffers();

    // command pool
    CreateCommandPool();

    CreateSemaphores();
}

void BasicApplication::MainLoop() {
    while (!glfwWindowShouldClose(m_window)){
        // Update events from user
        glfwPollEvents();
        DrawFrame();
    }
    vkDeviceWaitIdle(m_logicalDevice);
}


void BasicApplication::CreateVulkanInstance() {
    // fill in a struct with some information about our application
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Basic Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // tell the vulkan driver which global extensions and validation layers we want to use
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    /* For Extensions*/
    // get the GLFW required extensions
    const std::vector<const char*> glfwExtensionNames = GetGlfwRequiredExtensionNames();
    // check if required extensions are in the vulkan support extensions
    if (!CheckGlfwRequiredExtensions(glfwExtensionNames))
    {
        throw std::runtime_error("required extensions are not supported by vulkan");
    }
    // add GLFW required extensions to createInfo of the vulkan instance
    createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensionNames.size());
    createInfo.ppEnabledExtensionNames = glfwExtensionNames.data();

    /* For Validation layers*/
    // check if validation layers are supported
    if(m_enableValidationLayers && !CheckValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not supported");
    }
    // add validation layers to createInfo of the vulkan instance
    if (m_enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }else{ createInfo.enabledLayerCount = 0;}


    // create vulkan instance (m_instance based on the VkInstanceCreateInfo)
    if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create instance");
    }
}


const std::vector<const char*> BasicApplication::GetVulkanSupportedExtensionNames() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    std::vector<const char*> extensionNames(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    for (int i = 0; i < extensionCount; ++i)
    {
        extensionNames[i] = extensions[i].extensionName;
    }
    // non-const can be converted to const
    return extensionNames;
}

const std::vector<const char *> BasicApplication::GetGlfwRequiredExtensionNames() {
    uint32_t glfwExtensionCount = 0;
    // extension to make vulkan interface with the window system
    // GLFW returns the extensions it needs to interface
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> glfwExtensionNames(glfwExtensions, glfwExtensions+glfwExtensionCount);
    if (m_enableValidationLayers)
    {
        glfwExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return glfwExtensionNames;
}

bool BasicApplication::CheckGlfwRequiredExtensions(const std::vector<const char *> &requiredExtensions) {
    // Get names of extensions that vulkan support
    const std::vector<const char*>& vulkanSupportExtensionNames = GetVulkanSupportedExtensionNames();
    bool foundExtension;
    for (const char* glfwExtension : requiredExtensions)
    {
        foundExtension = false;
        for (const char* supportExtension : vulkanSupportExtensionNames)
        {
            if (strcmp(glfwExtension, supportExtension))
            {
                foundExtension = true;
                break;
            }
            if (!foundExtension)
            {
                return false;
            }
        }
    }
    return true;
}

bool BasicApplication::CheckDeviceExtensions(VkPhysicalDevice const &device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // get all the device extensions that are available by vulkan
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

    // check if all the required extensions are in the available extensions
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

bool BasicApplication::CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    // available validation layers in the vulkan
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    // check if all the layers in m_validationLayers exist in the availableLayers list
    bool layerFound;
    for (const char* layerName : m_validationLayers)
    {
        layerFound = false;
        for (const VkLayerProperties& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName))
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound){
            return false;
        }
    }
    return true;
}


void BasicApplication::SetupDebugMessenger() {
    if (!m_enableValidationLayers) {return;}
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = DebugCallBack;
    debugCreateInfo.pUserData = nullptr;
    if (CreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }

}

VkResult BasicApplication::CreateDebugUtilsMessengerEXT(VkInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator,
                                           VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto CreateDebugMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!CreateDebugMessengerFunc)
    {
        std::cerr << "Not found the function for creating debug utils messenger" << std::endl;
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return CreateDebugMessengerFunc(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void BasicApplication::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                 const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VkBool32 BasicApplication::DebugCallBack(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                     const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData){
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    }
    // VK_TRUE will terminate applications and the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error
    return VK_FALSE;
}

void BasicApplication::PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    if (!deviceCount)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    // Get all the GPU that vulkan supports
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, physicalDevices.data());
    // check if GPU is suitable for the application
    for (VkPhysicalDevice device : physicalDevices)
    {
        if(IsPhysicalDeviceSuitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
}

bool BasicApplication::IsPhysicalDeviceSuitable(VkPhysicalDevice const &device) {
    // Get GPU features and properties
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // if GPU support geometry shader
    //bool isDeviceSupportGeometryShader = deviceFeatures.geometryShader;
    // if GPU supports anisotropy
    bool isDeviceSupportAnisotropy = deviceFeatures.samplerAnisotropy;

    // if the queue families supported by this device, support VK_QUEUE_GRAPHICS_BIT
    const QueueFamilyIndices& indices = FindQueueFamilies(device);
    // check if device extensions are supported
    bool isExtensionSupported = CheckDeviceExtensions(device);
    // check if the swap chain support is sufficient for the application
    bool isSwapChainAdequate = false;
    if (isExtensionSupported)
    {
        SwapChainSupportDetails swapChainSupport = GetSwapChainSupportDetails(device);
        isSwapChainAdequate = !swapChainSupport.surfaceFormats.empty() && !swapChainSupport.presentModes.empty();
    }
    return indices.IsComplete() && isExtensionSupported && isSwapChainAdequate && isDeviceSupportAnisotropy;
}

QueueFamilyIndices BasicApplication::FindQueueFamilies(const VkPhysicalDevice& physicalDevice) {
   QueueFamilyIndices familyIndices;
   //Get list of queue families that the physicalDevice support
   uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    int queueFamilyIndex = 0;
    // find a queue family that support VK_QUEUE_GRAPHICS_BIT
    // also find a queue family that support presentation
    VkBool32 isFamilySupportPresentation;
    for(VkQueueFamilyProperties family : queueFamilies)
    {
        isFamilySupportPresentation = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, m_windowSurface, &isFamilySupportPresentation);
        if (isFamilySupportPresentation)
        {
            familyIndices.queueFamilyIndexForPresenting = queueFamilyIndex;
        }
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            familyIndices.queueFamilyIndexForDrawing = queueFamilyIndex;
        }
        if (familyIndices.IsComplete()) {break;}
        ++queueFamilyIndex;
    }
   return familyIndices;
}

void BasicApplication::CreateLogicalDevice() {
    const QueueFamilyIndices& indices = FindQueueFamilies(m_physicalDevice);

    // create Infos of device queue
    // different queueCreateInfo for different queue family
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.queueFamilyIndexForDrawing.value(), indices.queueFamilyIndexForPresenting.value()};
    float queuePriority = 1.0f;

    for (uint32_t familyIndex : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // the queue family index that supporting Queue Graphics
        queueCreateInfo.queueFamilyIndex = familyIndex;
        // the number of queues we want for a single queue family
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }


    // physical device features
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    // Info of logical device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    deviceCreateInfo.enabledExtensionCount =static_cast<uint32_t> (m_deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_logicalDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device");
    }

    //Retrieve the graphics queue for drawing operations
    // we only has single queue
    vkGetDeviceQueue(m_logicalDevice, indices.queueFamilyIndexForDrawing.value(), 0, &m_graphicsQueue);

    //Retrieve the present queue for present operations
    vkGetDeviceQueue(m_logicalDevice, indices.queueFamilyIndexForPresenting.value(), 0, &m_presentQueue);
}

void BasicApplication::CreateWindowSurface() {
    if (glfwCreateWindowSurface(m_Instance, m_window, nullptr, &m_windowSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface");
    }
}

SwapChainSupportDetails BasicApplication::GetSwapChainSupportDetails(VkPhysicalDevice const &device) {
    SwapChainSupportDetails details;
    // capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_windowSurface, &details.surfaceCapabilities);

    //formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_windowSurface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_windowSurface, &formatCount, details.surfaceFormats.data());
    }
    // present modes
    uint32_t modeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_windowSurface, &modeCount, nullptr);
    if (modeCount != 0)
    {
        details.presentModes.resize(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_windowSurface, &modeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR BasicApplication::PickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    // the best one is BGR format (8-bit unsigned integer for a total of 32 bits per pixel)
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    // the best is not found, then just return the first one
    return availableFormats[0];
}

VkPresentModeKHR BasicApplication::PickSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    // the best one is VK_PRESENT_MODE_MAILBOX_KHR, the next is VK_PRESENT_MODE_FIFO_KHR
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
        else if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR)
        {
            return availablePresentMode;
        }
    }
    // if not found the best one, then
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkExtent2D BasicApplication::PickSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    // the image resolution should be same as window size
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {m_windowWidth, m_windowHeight};

        // clamp the window size within the capabilities.minImageExtent and capabilities.maxImageExtent
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

void BasicApplication::CreateSwapChain() {
    SwapChainSupportDetails swapChainSupport = GetSwapChainSupportDetails(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = PickSwapSurfaceFormat(swapChainSupport.surfaceFormats);
    VkPresentModeKHR presentMode = PickSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = PickSwapExtent(swapChainSupport.surfaceCapabilities);
    // set image count
    uint32_t imageCount = swapChainSupport.surfaceCapabilities.minImageCount + 1;
    // make sure that image count not exceed the maxImageCount in surfaceCapabilities
    if (swapChainSupport.surfaceCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.surfaceCapabilities.maxImageCount) {
        imageCount = swapChainSupport.surfaceCapabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // determine how to handle images in swap chain cross the queue families
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.queueFamilyIndexForDrawing.value(), indices.queueFamilyIndexForPresenting.value()};

    if (indices.queueFamilyIndexForDrawing != indices.queueFamilyIndexForPresenting) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = swapChainSupport.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create!
    if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    // retrieve the images
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

    // store the image format and extent
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void BasicApplication::CreateImageViewsForSwapChain() {
    m_swapChainImageViews.resize(m_swapChainImages.size());
    for (size_t i = 0; i < m_swapChainImages.size(); ++i) {
            VulkanHelperFunctions::CreateImageView(m_logicalDevice, m_swapChainImages[i], m_swapChainImageFormat, m_swapChainImageViews[i]);
    }

}

void BasicApplication::CreateRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // subpass, post-processing after render process
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    // subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_logicalDevice, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void BasicApplication::CreateFrameBuffers() {
    m_swapChainFrameBuffers.resize(m_swapChainImageViews.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
                m_swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_swapChainFrameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void BasicApplication::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.queueFamilyIndexForDrawing.value();
    poolInfo.flags = 0;
    if (vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void BasicApplication::CreateCommandBuffers() {
    m_commandBuffers.resize(m_swapChainFrameBuffers.size());
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
    // record the commands in command buffers
    for (size_t i = 0; i < m_commandBuffers.size(); i++) {
        // begin recording
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr; // Optional
        if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        // record render pass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFrameBuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;
        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // loop each object
        for(BaseObject* object : m_objects)
        {
            // bind the graphics pipeline
            vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, object->m_graphicsPipeline);
            // bind the vertex buffer
            VkBuffer vertexBuffers_rec[] = {object->m_vertexBuffer};
            VkDeviceSize offsets_rec[] = {0};
            vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers_rec, offsets_rec);
            // bind the index buffer
            vkCmdBindIndexBuffer(m_commandBuffers[i], object->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            // bind the descriptor set for each swap chain image to the descriptors in the shader with vkCmdBindDescriptorSets (before the vkCmdDrawIndexed)
            vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, object->m_pipelineLayout, 0, 1, &object->m_descriptorSets[i], 0, nullptr);
            // draw the object
            vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(object->m_indices.size()), 1, 0, 0, 0);
        }

        // end render pass
        vkCmdEndRenderPass(m_commandBuffers[i]);

        // end recording
        if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

void BasicApplication::CreateSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
    vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create semaphores!");
    }

}

void BasicApplication::DrawFrame() {
    // acquire available image in the swap chain
    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // update the uniform buffer
    UpdateUniformBuffersForObjects(imageIndex);

    // submit commands
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    // present
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    vkQueuePresentKHR(m_presentQueue, &presentInfo);

    // if CPU is submitting commands faster than the GPU can keep ip with, then the queue will fill up
    // also we are reusing the two semaphores
    // so we should wait for the queue is idle and then draw next frame
    vkQueueWaitIdle(m_presentQueue);

    // TODO optimal way to use graphics pipeline for multiple frames at a time
}

void BasicApplication::UpdateUniformBuffersForObjects(uint32_t currentImage) {
    // the time when app starting
    static auto moveTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    // get the duration between start time and current time (seconds)
    float duration = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - moveTime).count();

    for (BaseObject* object : m_objects)
    {
        object->UpdateUniformBuffer(m_logicalDevice, duration, currentImage);
    }
}



void BasicApplication::CreateTexture(const char *textureFile) {
    BaseTexture* texture = new BaseTexture(textureFile);
    texture->CreateTexture(m_logicalDevice, m_physicalDevice, m_commandPool, m_graphicsQueue);
    m_textures[textureFile] = texture;
    // make ensure creating object after creating textures
    vkDeviceWaitIdle(m_logicalDevice);
}

void BasicApplication::DestroyTextures() {
    for (auto texture : m_textures)
    {
        texture.second->DestroyTexture(m_logicalDevice);
        delete texture.second;
        texture.second = nullptr;
    }
}

void BasicApplication::AddObjectToApplication(const char *objectName, ObjectType objectType, const char *objectFile,
                                              const char *objectTexture) {
    BaseObject* newObject = new BaseObject(objectType, objectFile);
    m_objects.push_back(newObject);
    // create texture first, because descriptor creation requires texture sampler when creating objects
    if (objectTexture)
    {
        // map insert, return pair<iterator, bool>
        // if m_textures not contain objectTextureFile, then create
        if (m_textures.insert({objectTexture, nullptr}).second)
        {
            CreateTexture(objectTexture);
            std::cout <<"Create texture: " << objectTexture << " successfully" << std::endl;
        }
        // set texture to object
        newObject->SetTexture(m_textures[objectTexture]);
    }

    // create object
    newObject->CreateObject(m_logicalDevice, m_physicalDevice, m_commandPool, m_graphicsQueue, static_cast<uint32_t>(m_swapChainImages.size()), m_renderPass, m_swapChainExtent);

    if (objectName)
    {
        std::cout <<"Create object: " << objectName << " successfully" << std::endl;
    }
    else{std::cout <<"Create object: UNKNOWN NAME successfully" << std::endl;}

}

void BasicApplication::DestroyObjects() {
    for (BaseObject* object : m_objects)
    {
        object->DestroyObject(m_logicalDevice);
        delete object;
        object = nullptr;
    }
}



















