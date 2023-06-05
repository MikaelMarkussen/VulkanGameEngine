#pragma once


#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "vulkan/vulkan.h"


#include <iostream>
#include <vector>
#include <optional>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN


struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> computeFamily;
	std::optional<uint32_t> transfereFamily;
	bool isComplete()
	{
		return graphicsFamily.has_value() && computeFamily.has_value() && transfereFamily.has_value();
	}
};


class WindowApp 
{
private:
	const int WIDTH = 900;
	const int HEIGHT = 600;

	GLFWwindow* window = nullptr;
	VkInstance mInstance;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice device;

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