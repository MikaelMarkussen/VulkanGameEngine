#include "Window.h"


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

	std::cout <<"GPU: " << deviceProp.deviceName << std::endl;
	std::cout << "Driver: " << deviceProp.driverVersion << std::endl;
	std::cout  << "Graphics queue family : " << indices.graphicsFamily.has_value() << std::endl;
	std::cout << "Transfere queue family : " << indices.transfereFamily.has_value() << std::endl;
	std::cout << "Compute queue family : " << indices.computeFamily.has_value() << std::endl;
	std::cout << "Present queue family : " << indices.presentFamily.has_value() << std::endl;

	return deviceProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && Devicefeat.geometryShader && indices.isComplete();

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

	createInfo.enabledExtensionCount = 0;
	
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
