#include "VulkanDevice.h"

#include <cstring>
#include <set>

VulkanEngine::VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
    : mInstance(instance), mSurface(surface) {
#ifdef DEBUG_BUILD
    mEnableValidationLayers = true;
#endif
#if RELEASE_BUILD
    mEnableValidationLayers = false;
#endif
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
}


VulkanEngine::VulkanDevice::~VulkanDevice() {

    if (mCommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
    }

    if (mDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(mDevice, nullptr);
    }

    if (mEnableValidationLayers && mDebugMessenger != VK_NULL_HANDLE) {
        DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
    }
}


uint32_t
VulkanEngine::VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice,&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if(typeFilter & (1 << i)&&(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("ERROR: failed to find suitable memory type");
}


VulkanEngine::SwapChainSupportDetails
VulkanEngine::VulkanDevice::querySwapChainSupport(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    uint32_t formatCount;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device,mSurface,&formatCount,details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentMode.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface,&presentModeCount, details.presentMode.data());
    }

    return details;
}


void
VulkanEngine::VulkanDevice::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilyIndices(mPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateinfos;
    std::set<uint32_t> uniqueQueueFamily = { indices.graphicsFamily.value(), indices.presentFamily.value()};


    if (indices.computeFamily.has_value()) {
        uniqueQueueFamily.insert(indices.computeFamily.value());
    }
    if (indices.transfereFamily.has_value()) {
        uniqueQueueFamily.insert(indices.transfereFamily.value());
    }
    float queuePriority = 1.f;
    for (uint32_t queueFamily : uniqueQueueFamily)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateinfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures DeviceFeatures{};

    VkDeviceCreateInfo createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateinfos.size());
    createInfo.pQueueCreateInfos = queueCreateinfos.data();

    createInfo.pEnabledFeatures = &DeviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(mDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = mDeviceExtensions.data();

    if (mEnableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
        createInfo.ppEnabledLayerNames = mValidationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }


    if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice))
    {
        throw std::runtime_error("ERROR: failed to create logical device");
    }


    vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue);

    if (indices.computeFamily.has_value()) {
        vkGetDeviceQueue(mDevice, indices.computeFamily.value(), 0, &mComputeQueue);
    }
    if (indices.transfereFamily.has_value()) {
        vkGetDeviceQueue(mDevice, indices.transfereFamily.value(), 0, &mTransferQueue);
    }

}


void
VulkanEngine::VulkanDevice::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilyIndices(mPhysicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}


bool
VulkanEngine::VulkanDevice::isDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Check queue families
    QueueFamilyIndices indices = findQueueFamilyIndices(device);

    // Check extension support
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // Check swap chain support
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentMode.empty();
    }

    // Device is suitable if:
    // 1. It's a discrete GPU (or whatever criteria you prefer)
    // 2. It supports geometry shaders
    // 3. It has the required queue families
    // 4. It supports the required extensions
    // 5. It has adequate swap chain support
    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           deviceFeatures.geometryShader &&
           indices.isComplete() &&
           extensionsSupported &&
           swapChainAdequate;
}


VulkanEngine::QueueFamilyIndices
VulkanEngine::VulkanDevice::findQueueFamilyIndices(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;
    uint32_t queueFamCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFam(queueFamCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, queueFam.data());
    int i = 0;
    for (const auto& queueFamily : queueFam)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;

        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}


bool
VulkanEngine::VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);

    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(mDeviceExtensions.begin(), mDeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}


void
VulkanEngine::VulkanDevice::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            mPhysicalDevice = device;
            break;
        }
    }

    if (mPhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    mQueueFamilyIndices = findQueueFamilyIndices(mPhysicalDevice);
}


bool
VulkanEngine::VulkanDevice::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    for (const char* layerName : mValidationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties : layers)
        {
            if (strcmp(layerName, layerProperties.layerName))
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


void
VulkanEngine::VulkanDevice::setupDebugMessenger()
{
    if (!mEnableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(mInstance,&createInfo,nullptr,&mDebugMessenger))
    {
        throw std::runtime_error("ERROR: Failed to create debug utils");
    }
}


void
VulkanEngine::VulkanDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}


VkBool32
VulkanEngine::VulkanDevice::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}


VkResult
VulkanEngine::VulkanDevice::CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


void
VulkanEngine::VulkanDevice::DestroyDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}