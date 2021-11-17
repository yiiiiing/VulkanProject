//
// Created by Ruiying on 17.9.2021.
//
#ifndef VULKANBASICS_BASICAPPLICATION_H
#define VULKANBASICS_BASICAPPLICATION_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include "BaseObject.h"

struct QueueFamilyIndices{
    std::optional<uint32_t> queueFamilyIndexForDrawing;
    std::optional<uint32_t> queueFamilyIndexForPresenting;

    // check if this queueFamily can be used
    bool IsComplete() const {
        return queueFamilyIndexForDrawing.has_value() &&
        queueFamilyIndexForPresenting.has_value();
    }
};

struct SwapChainSupportDetails {
    // min/max number of images in swap chain, min/max width and height of images
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    // pixel format, color space
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    // available presentation modes
    std::vector<VkPresentModeKHR> presentModes;
};


class BasicApplication {

public:
    // initial window, device, swap chain, render pass and command pool
    void InitialApplication(int windowWidth, int windowHeight, const char* windowName);

    void AddObjectToApplication(const char *objectName, ObjectType objectType, const char *objectFile,
                                const char *objectTexture);

    void RunApplication();

    // private functions
private:
    void InitWindow(int windowWidth, int windowHeight, const char* windowName);
    void InitVulkan();
    void CreateVulkanInstance();
    void MainLoop();
    void CleanUp();

    // Get the names of extension that vulkan supports
    const std::vector<const char*> GetVulkanSupportedExtensionNames();
    // Get the names of glfw required extensions
    const std::vector<const char*> GetGlfwRequiredExtensionNames();
    // Querying details of swap chain support
    SwapChainSupportDetails GetSwapChainSupportDetails(const VkPhysicalDevice& device);

    // check if glfw required extensions are all vulkan supported
    bool CheckGlfwRequiredExtensions(const std::vector<const char *> &requiredExtensions);

    // check if all the validation layers are supported
    bool CheckValidationLayerSupport();

    // check if required device extensions are supported
    bool CheckDeviceExtensions(VkPhysicalDevice const &device);

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallBack(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                 const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                 void *pUserData);
    // setup debug messenger
    void SetupDebugMessenger();

    //create VkDebugUtilsMessengerEXT object
    VkResult CreateDebugUtilsMessengerEXT(VkInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    // destroy VkDebugUtilsMessengerEXT
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    // create window surface
    void CreateWindowSurface();

    /* Select settings for the swap chain*/
    // surface format
    VkSurfaceFormatKHR PickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    // present mode
    VkPresentModeKHR PickSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    // image resolutions from surface capabilities
    VkExtent2D PickSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);


    // Select a physical device to use
    void PickPhysicalDevice();
    // check if a physical device is suitable for the application
    bool IsPhysicalDeviceSuitable(const VkPhysicalDevice& device);
    // find suitable queue families
    QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& physicalDevice);

    // create logical device
    void CreateLogicalDevice();

    // create the swap chain
    void CreateSwapChain();

    // create the image views for frame in swap chain
    void CreateImageViewsForSwapChain();

    // create render pass
    void CreateRenderPass();

    // Create frame buffers
    void CreateFrameBuffers();

    // Create command pool
    void CreateCommandPool();

    // create command buffers
    void CreateCommandBuffers();

    // Create semaphores
    void CreateSemaphores();

    // draw frame, run in main loop
    void DrawFrame();

    // create objects
    void CreateObjects();
    // destroy objects
    void DestroyObjects();
    // update uniform buffers for objects
    void UpdateUniformBuffersForObjects(uint32_t currentImage);

    void CreateTexture(const char *textureFile);

    void DestroyTextures();


// private variables
private:
    // GLFW window
    GLFWwindow* m_window = nullptr;
    // vulkan instance (specify the details about your applications to the driver)
    VkInstance m_Instance;

    // Window width and height
    uint32_t m_windowWidth;
    uint32_t m_windowHeight;

    // if enable validation layer to debug
#ifdef NO_VALIDATION_DEBUG
    const bool m_enableValidationLayers = false;
#else
    const bool m_enableValidationLayers = true;
#endif

    // validation layers
    //VK_LAYER_KHRONOS_validation: a layer containing all useful standard layers
    const std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};

    // Required physical device extensions
    const std::vector<const char*> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // debug messenger
    VkDebugUtilsMessengerEXT m_debugMessenger;

    // physical device
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    // logical device
    VkDevice m_logicalDevice;

    // Graphics queue for drawing
    VkQueue m_graphicsQueue;
    // present queue for presentation
    VkQueue m_presentQueue;

    // Window surface
    VkSurfaceKHR m_windowSurface;

    // swap chain
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;

    // image views for each VkImage (describe how to access image and which part of an image)
    std::vector<VkImageView> m_swapChainImageViews;

    // render pass
    VkRenderPass m_renderPass;

    // frame buffers
    std::vector<VkFramebuffer> m_swapChainFrameBuffers;

    // command pool to store the command buffers
    VkCommandPool m_commandPool;
    // don't need to destroy because these can be freed when command pool is destroyed
    // each command buffers for each frame buffer
    std::vector<VkCommandBuffer> m_commandBuffers;

    // semaphores (used in drawing frame)
    VkSemaphore m_imageAvailableSemaphore;
    VkSemaphore m_renderFinishedSemaphore;

    // objects in the scene
    std::vector<BaseObject*> m_objects;
    std::unordered_map<const char*, BaseTexture*> m_textures;

};


#endif //VULKANBASICS_BASICAPPLICATION_H
