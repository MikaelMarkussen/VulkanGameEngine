#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "vulkan/vulkan.h"


#include <iostream>
#include <vector>
#include <optional>



struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> computeFamily;
	std::optional<uint32_t> transfereFamily;
	std::optional<uint32_t> presentFamily;
	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();// && computeFamily.has_value() && transfereFamily.has_value();
	}
	bool isComputeFamily() { return computeFamily.has_value(); }
	bool isGraphicsFamily() { return graphicsFamily.has_value(); }
	bool isTransfereFamily() { return transfereFamily.has_value(); }

};


class WindowApp 
{
private:
	const int WIDTH = 900;
	const int HEIGHT = 600;

	GLFWwindow* mWindow = nullptr;
	VkInstance mInstance;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mDevice;
	VkQueue mGraphicsQueue;
	VkQueue mPresentQueue;
	VkSurfaceKHR mSurface;

public:
	void run();
private:
	//setting up vulkan
	void initWindow();
	void initVulkan();
	void WindowLoop();
	void CleanUp();

	//creating a vkinstance
	void CreateInstance();

	//setting up physical device
	void PhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice Device);
	int rateDevice(VkPhysicalDevice device);


	//setting up queue families
	QueueFamilyIndices findQueueFamily(VkPhysicalDevice device);



	//setting up validationLayers
	bool checkForValidationLayerSupport();

	//setting up logical device
	void createLogicalDevice();

	//creating surface
	void createSurface();

	//
	const std::vector<const char*> mValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

#ifdef DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif // DEBUG



};