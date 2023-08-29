#include "Window.h"
#include <algorithm>
#include <limits>
#include <cstdint>
#include <fstream>

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
	createSurface();	
	PhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageVeiw();
	createRenderPass();
	createGraphicsPipeline();

}
void WindowApp::WindowLoop()
{
	while (!glfwWindowShouldClose(mWindow))
	{
		glfwPollEvents();
		//if (glfwGetKey(mWindow,GLFW_KEY_ESCAPE)) {
		//	return;
		//}
	}
}
void WindowApp::CleanUp()
{
	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(mDevice, imageView, nullptr);
	}

	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, nullptr);
	vkDestroySwapchainKHR(mDevice,mSwapChain,nullptr);
	vkDestroyPipeline(mDevice, mPipeline, nullptr);
	vkDestroyPipelineLayout(mDevice, mPipelinelayout, nullptr);
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
	auto vertexShad = readShaderFile("C:/Prosjekter/Vulkan/VulkanGameEngine/GameEngine/Shader/vert.spv");
	auto fragmentShad = readShaderFile("C:/Prosjekter/Vulkan/VulkanGameEngine/GameEngine/Shader/frag.spv");

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

	vkDestroyShaderModule(mDevice,fragShaderMod,nullptr);
	vkDestroyShaderModule(mDevice, vertShaderMod, nullptr);
	
	VkPipelineVertexInputStateCreateInfo vertexcreateInfo{};

	vertexcreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexcreateInfo.vertexBindingDescriptionCount = 0;
	vertexcreateInfo.pVertexBindingDescriptions = nullptr;
	vertexcreateInfo.vertexAttributeDescriptionCount = 0;
	vertexcreateInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inAsmCreateinfo{};

	inAsmCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inAsmCreateinfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inAsmCreateinfo.primitiveRestartEnable = VK_FALSE;

	VkViewport Viewport{};
	Viewport.x = 0.f;
	Viewport.y = 0.f;
	Viewport.width = static_cast<float>(mSwapchainExtent.width);
	Viewport.height = static_cast<float>(mSwapchainExtent.width);
	Viewport.minDepth = 0.f;
	Viewport.maxDepth = 1.f;

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = mSwapchainExtent;

	std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	createInfo.dynamicStateCount = dynamicStates.size();
	createInfo.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewCreateinfo{};
	viewCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewCreateinfo.scissorCount = 1;
	viewCreateinfo.pScissors = &scissor;
	viewCreateinfo.viewportCount = 1;
	viewCreateinfo.pViewports = &Viewport;

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

	VkPipelineLayoutCreateInfo pipelineLayCreateInfo{};
	pipelineLayCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayCreateInfo.setLayoutCount = 0;
	pipelineLayCreateInfo.pSetLayouts = nullptr;
	pipelineLayCreateInfo.pushConstantRangeCount = 0;
	pipelineLayCreateInfo.pPushConstantRanges = nullptr;

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
	pipelineInfo.pDynamicState = &createInfo;
	
	pipelineInfo.layout = mPipelinelayout;
	
	pipelineInfo.renderPass = mRenderpass;
	pipelineInfo.subpass = 0;
	
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(mDevice,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&mPipeline)!=VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create graphics pipeline");
	}
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

	VkRenderPassCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(mDevice,&createInfo,nullptr,&mRenderpass))
	{
		throw std::runtime_error("ERROR: failed to create render pass");
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
		size_t fileSize = file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
#ifdef DEBUG
		std::cout << std::endl << "LOG: read file from: " << fileName;
#endif // DEBUG
		return buffer;
	
}
