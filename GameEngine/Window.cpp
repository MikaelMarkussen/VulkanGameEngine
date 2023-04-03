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
	window = glfwCreateWindow(WIDTH, HEIGHT, "MikaelVulkanEngine", nullptr, nullptr);
}
void WindowApp::initVulkan()
{

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
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}

void WindowApp::CreateInstance()
{
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "MikaelEngine";

	VkInstanceCreateInfo* createInfo;
	createInfo->sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo->pApplicationInfo = &appInfo;
	
	uint32_t extentioncount = 0;
	const char** glfwExtension;
	glfwExtension = glfwGetRequiredInstanceExtensions(&extentioncount);

	createInfo->enabledExtensionCount = extentioncount;
	createInfo->ppEnabledExtensionNames = glfwExtension;
	createInfo->enabledLayerCount = 0;

	VkResult result =  vkCreateInstance(createInfo,nullptr,&instance);
	if (result != VK_SUCCESS) 
	{
		return;
	}

}
