#include "Window.h"
#include <algorithm>
#include <limits>
#include <cstdint>

//to run std::numeric_limits<T>::max()
//#undef max

void WindowApp::run()
{
	initWindow();
	initVulkan();
	WindowLoop();
	CleanUp();
}

void WindowApp::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	mWindow = glfwCreateWindow(WIDTH, HEIGHT, "VulkanEngine", nullptr, nullptr);
}
void WindowApp::initVulkan()
{
	CreateInstance();
	createSurface();	
	PhysicalDevice();
	createLogicalDevice();
	createSwapChain();
}
void WindowApp::WindowLoop()
{
	while (!glfwWindowShouldClose(mWindow))
	{
		glfwPollEvents();
	}
}
void WindowApp::CleanUp()
{
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, nullptr);
	vkDestroyDevice(mDevice, nullptr);
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void WindowApp::CreateInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VulkanEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "MikaelEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	
	uint32_t extentioncount = 0;
	const char** glfwExtension;
	glfwExtension = glfwGetRequiredInstanceExtensions(&extentioncount);
	
	createInfo.enabledExtensionCount = extentioncount;
	createInfo.ppEnabledExtensionNames = glfwExtension;

	createInfo.enabledLayerCount = 0;

	
	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create a VkInstance");
	}
}

void WindowApp::PhysicalDevice()
{
	uint32_t numberOfGPU;
	vkEnumeratePhysicalDevices(mInstance, &numberOfGPU, nullptr);
	if (numberOfGPU == 0) 
	{
		throw std::runtime_error( "ERROR: Could not find suitable GPU");
	}
	std::vector<VkPhysicalDevice> physDev(numberOfGPU);
	vkEnumeratePhysicalDevices(mInstance, &numberOfGPU, physDev.data());
	for (const auto& device : physDev) 
	{
		if (isDeviceSuitable(device)) 
		{
			mPhysicalDevice = device;
			break;
		}
	}
	
	if (mPhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("ERROR:Could not find a suitable device");
	}       
}

bool WindowApp::isDeviceSuitable(VkPhysicalDevice Device)
{
	VkPhysicalDeviceProperties deviceProp;
	VkPhysicalDeviceFeatures Devicefeat;
	vkGetPhysicalDeviceFeatures(Device, &Devicefeat);
	vkGetPhysicalDeviceProperties(Device, &deviceProp);
	QueueFamilyIndices indices = findQueueFamily(Device);
	bool extensionSupport = checkExtensionSupport(Device);
	bool swapChainSupported = false;
	if (extensionSupport) 
	{
		SwapChainSupportDetails swapChainDet = querySwapChainSupport(Device);
		swapChainSupported = !swapChainDet.formats.empty() && !swapChainDet.presentMode.empty();
	}


	std::cout <<"GPU: " << deviceProp.deviceName << std::endl;
	std::cout << "Driver: " << deviceProp.driverVersion << std::endl;
	std::cout  << "Graphics queue family : " << indices.graphicsFamily.has_value() << std::endl;
	std::cout << "Transfere queue family : " << indices.transfereFamily.has_value() << std::endl;
	std::cout << "Compute queue family : " << indices.computeFamily.has_value() << std::endl;
	std::cout << "Present queue family : " << indices.presentFamily.has_value() << std::endl;

	return deviceProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && Devicefeat.geometryShader && indices.isComplete() && extensionSupport&&swapChainSupported;

}

QueueFamilyIndices WindowApp::findQueueFamily(VkPhysicalDevice device)
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
		}else if(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			indices.computeFamily = i;
		}
		else if(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			indices.transfereFamily = i;
		}
		VkBool32 presentSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);
		if (presentSupport)
		{
			indices.presentFamily = i;
		}
		i++;
	}

	return indices;
}

bool WindowApp::checkExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device,nullptr,&extensionCount,nullptr);
	std::vector<VkExtensionProperties> extensionProp(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensionProp.data());

	std::set<std::string> requiredExtension(deviceExtensions.begin(), deviceExtensions.end());
	for(auto &extension:extensionProp)
	{
		requiredExtension.erase(extension.extensionName);
	}
	return requiredExtension.empty();
}

bool WindowApp::checkForValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> layers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
	return false;
}

void WindowApp::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamily(mPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateinfos;
	std::set<uint32_t> uniqueQueueFamily = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.f;
	for (uint32_t queueFamily : uniqueQueueFamily)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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
	int i;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	
	if (enableValidationLayers) 
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
}

void WindowApp::createSurface()
{
	VkWin32SurfaceCreateInfoKHR createInfo {};

	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = glfwGetWin32Window(mWindow);
	createInfo.hinstance = GetModuleHandle(nullptr);

	if (vkCreateWin32SurfaceKHR(mInstance, &createInfo, nullptr, &mSurface) != VK_SUCCESS) 
	{
		throw std::runtime_error("ERROR: Could not create window surface");
	}
	else
	{
		std::cout << "LOG: Window surface created \n";
	}

}

SwapChainSupportDetails WindowApp::querySwapChainSupport(VkPhysicalDevice device)
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

VkSurfaceFormatKHR WindowApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for(const auto &avalibleFormat : availableFormats)
	{
		if (avalibleFormat.format == VK_FORMAT_B8G8R8A8_SRGB && avalibleFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			std::cout << avalibleFormat.format;
			return avalibleFormat;
		}
	}
	std::cout << availableFormats[1].format;
	return VkSurfaceFormatKHR();
}

VkPresentModeKHR WindowApp::choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& presentModes : availablePresentModes)
	{
		if(presentModes == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentModes;
		}
	}

	return VkPresentModeKHR();
}

VkExtent2D WindowApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(mWindow, &width, &height);
		VkExtent2D actualExtent = { static_cast<uint32_t>(width),static_cast<uint32_t>(height) };
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		
		return	actualExtent;

	}
}

void WindowApp::createSwapChain()
{
	SwapChainSupportDetails swapchainSup = querySwapChainSupport(mPhysicalDevice);

	VkSurfaceFormatKHR surfaceformat = chooseSwapSurfaceFormat(swapchainSup.formats);
	VkPresentModeKHR presentMode = choosePresentMode(swapchainSup.presentMode);
	VkExtent2D extent = chooseSwapExtent(swapchainSup.capabilities);
	
	uint32_t imageCount = swapchainSup.capabilities.minImageCount + 1;

	if (swapchainSup.capabilities.maxImageCount > 0 && imageCount > swapchainSup.capabilities.maxImageCount)
	{
		imageCount = swapchainSup.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;
	createInfo.imageExtent = extent;
	createInfo.imageFormat = surfaceformat.format;
	createInfo.imageColorSpace = surfaceformat.colorSpace;
	createInfo.minImageCount = imageCount;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamily(mPhysicalDevice);
	uint32_t queueFamily[] = {indices.graphicsFamily.value(),indices.presentFamily.value()};

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamily;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}
	createInfo.preTransform = swapchainSup.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT;

}