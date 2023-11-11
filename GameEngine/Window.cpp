#include "Window.h"
#include <algorithm>
#include <limits>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <filesystem>
#include <chrono>
#include <Windows.h>

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
	mWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Engine", nullptr, nullptr);
}
void WindowApp::initVulkan()
{
	CreateInstance();
	setupDebugMessenger();
	createSurface();	
	PhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageVeiw();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createIndexBuffer();
	createCommandBuffers();
	createSyncObj();
}

void WindowApp::CleanUp()
{
	cleanUpSwapChain();
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(mDevice, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(mDevice, imageAvalibleSemaphores[i], nullptr);
		vkDestroyFence(mDevice, inFlightFences[i], nullptr);
	}

	vkDestroyBuffer(mDevice, mIndexBuffer, nullptr);
	vkFreeMemory(mDevice, mIndexBufferMemory, nullptr);
	
	vkDestroyBuffer(mDevice, mVertexBuffer, nullptr);
	vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

	vkDestroyBuffer(mDevice,mVertexBuffer,nullptr);
	vkFreeMemory(mDevice,mVertexBufferMemory,nullptr);
	for (auto frameBuffer : swapChainFrambuffers)
	{
		vkDestroyFramebuffer(mDevice, frameBuffer, nullptr);
	}

	vkDestroyPipelineLayout(mDevice, mPipelinelayout, nullptr);
	vkDestroyPipeline(mDevice, mPipeline, nullptr);
	vkDestroyRenderPass(mDevice, mRenderpass,nullptr);


	for (auto imageView : swapChainImageViews) 
	{
		vkDestroyImageView(mDevice, imageView, nullptr);
	}
	vkDestroyDevice(mDevice, nullptr);
	
	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, nullptr);
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void WindowApp::WindowLoop()
{
	while (!glfwWindowShouldClose(mWindow))
	{
		//system("cls");

		glfwPollEvents();
		auto frameStart = std::chrono::high_resolution_clock::now();
		drawFrame();
		auto frameEnd = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::milli> elapsed = frameEnd - frameStart;
		double fps = 1000.0 / elapsed.count();
		
		//std::cout << "Frame time: " << elapsed.count() << "ms" << std::endl;
		//std::cout << "FPS: " << fps << std::endl;
		
	}
	vkDeviceWaitIdle(mDevice);
}
void WindowApp::drawFrame()
{
	vkWaitForFences(mDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(mDevice, 1, &inFlightFences[currentFrame]);

	uint32_t imageIndex;
	VkResult swapchainResult = vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, imageAvalibleSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (swapchainResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if(swapchainResult != VK_SUCCESS && swapchainResult != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("ERROR: FAILED TO AQUIRE SWAPCHAIN IMAGE");
	}

	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvalibleSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	VkSemaphore signalSemaphore[] = {renderFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphore;

	if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: failed to submit to queue");
	}


	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphore;

	VkSwapchainKHR swapchains[] = {mSwapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(mPresentQueue,&presentInfo);
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}


void WindowApp::CreateInstance()
{
	if (enableValidationLayers && !checkForValidationLayerSupport())
	{
		throw std::runtime_error("ERROR: validation layer not avalible");
	}

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
	
	auto extesions = getRequiredextensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extesions.size());
	createInfo.ppEnabledExtensionNames = extesions.data();
	
	VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
	if(enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
		createInfo.ppEnabledLayerNames = mValidationLayers.data();

		populateDebugMessengerCreateInfo(debugInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}


	
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

std::vector<const char*> WindowApp::getRequiredextensions()
{
	uint32_t extentioncount = 0;
	const char** glfwExtension;
	glfwExtension = glfwGetRequiredInstanceExtensions(&extentioncount);

	std::vector<const char*> extensions(glfwExtension, glfwExtension + extentioncount);

	if(enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL WindowApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void WindowApp::setupDebugMessenger()
{
	if (!enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(mInstance,&createInfo,nullptr,&mDebugMessenger))
	{
		throw std::runtime_error("ERROR: Failed to create debug utils");
	}
}

VkResult WindowApp::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void WindowApp::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void WindowApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
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
	//VkWin32SurfaceCreateInfoKHR createInfo {};

	//createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	//createInfo.hwnd = glfwGetWin32Window(mWindow);
	//createInfo.hinstance = GetModuleHandle(nullptr);

	//if (vkCreateWin32SurfaceKHR(mInstance, &createInfo, nullptr, &mSurface) != VK_SUCCESS) 
	//{
	//	throw std::runtime_error("ERROR: Could not create window surface");
	//}
	if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
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
		if (avalibleFormat.format == VK_FORMAT_B8G8R8A8_SRGB && avalibleFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return avalibleFormat;
		}
	}
	return availableFormats[0];
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

	return VK_PRESENT_MODE_FIFO_KHR;
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
	createInfo.imageFormat = surfaceformat.format;
	createInfo.imageColorSpace = surfaceformat.colorSpace;
	createInfo.minImageCount = imageCount;
	createInfo.imageExtent = extent;
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
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS) 
	{
		throw std::runtime_error("ERROR: failed to create swapchain");
	}
	

	vkGetSwapchainImagesKHR(mDevice,mSwapChain,&imageCount,nullptr);
	swapChainImage.resize(imageCount);
	vkGetSwapchainImagesKHR(mDevice,mSwapChain,&imageCount,swapChainImage.data());
	mSwapChainImageFormat = surfaceformat.format;
	mSwapchainExtent = extent;
}

void WindowApp::recreateSwapChain()
{
	vkDeviceWaitIdle(mDevice);
	cleanUpSwapChain();

	createSwapChain();
	createImageVeiw();
	createFramebuffers();

}

void WindowApp::cleanUpSwapChain()
{
	for (auto i = 0; i < swapChainImageViews.size(); i++)
	{
		vkDestroyFramebuffer(mDevice,swapChainFrambuffers[i],nullptr);
	}
	for(auto i = 0; i < swapChainImageViews.size();i++)
	{
		vkDestroyImageView(mDevice, swapChainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(mDevice,mSwapChain,nullptr);
}

void WindowApp::createImageVeiw()
{
	swapChainImageViews.resize(swapChainImage.size());

	for(auto i = 0; i < swapChainImage.size();i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImage[i];

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = mSwapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(mDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("ERROR: failed to create image views");
		}

	}
}

void WindowApp::createGraphicsPipeline()
{
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescription = Vertex::getAttributeDescriptions();

	auto vertexShad = readShaderFile("Shader/vert.spv");
	auto fragmentShad = readShaderFile("Shader/frag.spv");

	VkShaderModule vertShaderMod = createShaderModule(vertexShad);
	VkShaderModule fragShaderMod = createShaderModule(fragmentShad);

	VkPipelineShaderStageCreateInfo VertCreateInfo{};
	VertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VertCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertCreateInfo.module = vertShaderMod;
	VertCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo FragCreateInfo{};
	FragCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	FragCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragCreateInfo.module = fragShaderMod;
	FragCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { VertCreateInfo, FragCreateInfo };

	
	VkPipelineVertexInputStateCreateInfo vertexcreateInfo{};
	vertexcreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexcreateInfo.vertexBindingDescriptionCount = 1;
	vertexcreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexcreateInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexcreateInfo.pVertexAttributeDescriptions = attributeDescription.data();

	VkPipelineInputAssemblyStateCreateInfo inAsmCreateinfo{};
	inAsmCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inAsmCreateinfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inAsmCreateinfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewCreateinfo{};
	viewCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewCreateinfo.scissorCount = 1;
	viewCreateinfo.viewportCount = 1;

	VkPipelineRasterizationStateCreateInfo raztCreateinfo{};
	raztCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raztCreateinfo.depthClampEnable = VK_FALSE;
	raztCreateinfo.rasterizerDiscardEnable = VK_FALSE;
	raztCreateinfo.polygonMode = VK_POLYGON_MODE_FILL;
	raztCreateinfo.lineWidth = 1.0f;
	raztCreateinfo.cullMode = VK_CULL_MODE_BACK_BIT;
	raztCreateinfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raztCreateinfo.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multiCreateinfo{};
	multiCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiCreateinfo.sampleShadingEnable = VK_FALSE;
	multiCreateinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multiCreateinfo.minSampleShading = 1.0f; // Optional
	multiCreateinfo.pSampleMask = nullptr; // Optional
	multiCreateinfo.alphaToCoverageEnable = VK_FALSE; // Optional
	multiCreateinfo.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAtt{};
	colorBlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAtt.blendEnable = VK_FALSE;
	colorBlendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAtt.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAtt.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAtt;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo DynamicCreateInfo{};
	DynamicCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	DynamicCreateInfo.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayCreateInfo{};
	pipelineLayCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayCreateInfo.setLayoutCount = 0;
	pipelineLayCreateInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(mDevice,&pipelineLayCreateInfo,nullptr,&mPipelinelayout) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create PipelineLayout");
	}
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexcreateInfo;
	pipelineInfo.pInputAssemblyState = &inAsmCreateinfo;
	pipelineInfo.pViewportState = &viewCreateinfo;
	pipelineInfo.pRasterizationState = &raztCreateinfo;
	pipelineInfo.pMultisampleState = &multiCreateinfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &DynamicCreateInfo;
	pipelineInfo.layout = mPipelinelayout;
	pipelineInfo.renderPass = mRenderpass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(mDevice,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&mPipeline)!=VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(mDevice, fragShaderMod, nullptr);
	vkDestroyShaderModule(mDevice, vertShaderMod, nullptr);
}

void WindowApp::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = mSwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorRef{};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	VkSubpassDependency subpassdep{};
	subpassdep.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassdep.dstSubpass = 0;
	subpassdep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassdep.srcAccessMask = 0;
	subpassdep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassdep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &subpassdep;	


	if (vkCreateRenderPass(mDevice,&createInfo,nullptr,&mRenderpass))
	{
		throw std::runtime_error("ERROR: failed to create render pass");
	}
}

void WindowApp::createFramebuffers()
{
	swapChainFrambuffers.resize(swapChainImageViews.size());
	
	for (auto i = 0; i < swapChainImageViews.size(); i++)
	{
		VkImageView attachment[] = { swapChainImageViews[i] };
		
		VkFramebufferCreateInfo frambufferinfo{};
		frambufferinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frambufferinfo.renderPass = mRenderpass;
		frambufferinfo.attachmentCount = 1;
		frambufferinfo.pAttachments = attachment;
		frambufferinfo.width = mSwapchainExtent.width;
		frambufferinfo.height = mSwapchainExtent.height;
		frambufferinfo.layers = 1;

		if (vkCreateFramebuffer(mDevice, &frambufferinfo, nullptr, &swapChainFrambuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("ERROR: failed to make frambuffer");
		}
	}

}

void WindowApp::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamily(mPhysicalDevice);

	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(mDevice,&createInfo,nullptr,&mCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: failed to create command pool");
	}
}

void WindowApp::createCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(mDevice,&allocInfo,commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("ERORR: failed to allocate command buffers");
	}
}

void WindowApp::createSyncObj()
{
	imageAvalibleSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &imageAvalibleSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("ERROR: failed to create semaphore or fence");
		}
	}

}

void WindowApp::recordCommandBuffer(VkCommandBuffer buffer, uint32_t index)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: failed to record command buffer");
	}

	VkRenderPassBeginInfo renderpassBegininfo{};

	renderpassBegininfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderpassBegininfo.renderPass = mRenderpass;
	renderpassBegininfo.framebuffer = swapChainFrambuffers[index];
	renderpassBegininfo.renderArea.offset = { 0,0 };
	renderpassBegininfo.renderArea.extent = mSwapchainExtent;

	VkClearValue clearColor = { {{0.0f,0.0f,0.0f,1.0f}} };
	renderpassBegininfo.clearValueCount = 1;
	renderpassBegininfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(buffer, &renderpassBegininfo, VK_SUBPASS_CONTENTS_INLINE);
	VkBuffer vertexBuffers[] = { mVertexBuffer };
	VkDeviceSize offsets[] = { 0 };


	vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(buffer, mIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(mSwapchainExtent.width);
	viewport.height = static_cast<float>(mSwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(buffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = mSwapchainExtent;


	vkCmdSetScissor(buffer,0,1,&scissor);

	vkCmdDrawIndexed(buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);


	vkCmdEndRenderPass(buffer);

	if (vkEndCommandBuffer(buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: failed to record command buffer");
	}

}

VkShaderModule WindowApp::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	

	VkShaderModule shaderMod;

	if(vkCreateShaderModule(mDevice,&createInfo,nullptr,&shaderMod) != VK_SUCCESS)
	{
		throw std::runtime_error("ERORR: Could not create shadermodule");
	}
	return shaderMod;
}

std::vector<char> WindowApp::readShaderFile(const std::string& fileName)
{
	
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("ERROR: Could not open shader file");
		
	}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
}

void WindowApp::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,stagingBuffer,stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice,stagingBufferMemory,0,bufferSize,0,&data);
	memcpy(data,vertices.data(),(size_t)bufferSize);
	vkUnmapMemory(mDevice,stagingBufferMemory);

	createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,mVertexBuffer,mVertexBufferMemory);
	
	copyBuffer(stagingBuffer,mVertexBuffer,bufferSize);

	vkDestroyBuffer(mDevice,stagingBuffer,nullptr);
	vkFreeMemory(mDevice,stagingBufferMemory,nullptr);
}

void WindowApp::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice,stagingBufferMemory,0,bufferSize,0,&data);
	memcpy(data,indices.data(),(size_t)bufferSize);
	vkUnmapMemory(mDevice,stagingBufferMemory);

	createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,mIndexBuffer,mIndexBufferMemory);

	copyBuffer(stagingBuffer,mIndexBuffer,bufferSize);

	vkDestroyBuffer(mDevice,stagingBuffer,nullptr);
	vkFreeMemory(mDevice,stagingBufferMemory,nullptr);
}

void WindowApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create vertex buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;

	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{ 
		throw std::runtime_error("ERROR: Unable to allocate memory");
	}
	vkBindBufferMemory(mDevice, buffer, bufferMemory, 0);
}

void WindowApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandPool = mCommandPool;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer,&beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	
	vkCmdCopyBuffer(commandBuffer,srcBuffer,dstBuffer,1,&copyRegion);

	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	
	vkQueueSubmit(mGraphicsQueue,1,&submitInfo,VK_NULL_HANDLE);
	vkQueueWaitIdle(mGraphicsQueue);

	vkFreeCommandBuffers(mDevice,mCommandPool,1,&commandBuffer);
}

uint32_t WindowApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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
