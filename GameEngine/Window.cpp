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
	window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanEngine", nullptr, nullptr);
}
void WindowApp::initVulkan()
{
	CreateInstance();
	PhysicalDevice();
	createLogicalDevice();
}
void WindowApp::WindowLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}
void WindowApp::CleanUp()
{
	vkDestroyInstance(mInstance, nullptr);
	vkDestroyDevice(mDevice, nullptr);
	glfwDestroyWindow(window);
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

	std::cout << deviceProp.deviceName << std::endl;
	std::cout  << "Graphics queue family : " << indices.graphicsFamily.has_value() << std::endl;
	std::cout << "Transfere queue family : " << indices.transfereFamily.has_value() << std::endl;
	std::cout << "Comnpute queue family : " << indices.computeFamily.has_value() << std::endl;
	std::cout << "is complete : " << indices.isComplete();


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
	VkDeviceQueueCreateInfo queueInfo{};

	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueInfo.queueCount = 1;
	float queuePriority = 1.f;
	queueInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures DeviceFeatures{};
	
	VkDeviceCreateInfo createInfo{};
	
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueInfo;
	createInfo.queueCreateInfoCount = 1;

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
}
