#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "vulkan/vulkan.h"
#include "Vertex.h"


#include <iostream>
#include <vector>
#include <optional>
#include <set>

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};
constexpr int WIDTH =800;
constexpr int HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

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
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentMode;

};

class WindowApp 
{
private:


	GLFWwindow* mWindow = nullptr;
	VkInstance mInstance;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mDevice;
	VkQueue mGraphicsQueue;
	VkQueue mPresentQueue;
	VkSurfaceKHR mSurface;
	VkSwapchainKHR mSwapChain;
	VkPipelineLayout mPipelinelayout;
	VkRenderPass mRenderpass;
	VkPipeline mPipeline;
	VkCommandPool mCommandPool;
	VkCommandBuffer mCommandBuffer;
	std::vector<VkCommandBuffer> commandBuffers;

	VkDescriptorSetLayout mDescriptorSetLayout;
	VkPipelineLayout mPipelineLayout;

	VkBuffer mVertexBuffer;
	VkDeviceMemory mVertexBufferMemory;
	VkBuffer mIndexBuffer;
	VkDeviceMemory mIndexBufferMemory;

	std::vector<VkBuffer> mUniformBuffers;
	std::vector<VkDeviceMemory> mUniformBuffersMemory;
	std::vector<void*> mUniformBuffersMapped;

	VkDescriptorPool mDescriptorPool;
	std::vector<VkDescriptorSet> mDescriptorSets;

	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapchainExtent;

	VkSemaphore imageAvalibleSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence infligthFence;

	std::vector<VkSemaphore> imageAvalibleSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	uint32_t currentFrame = 0;

	VkDebugUtilsMessengerEXT mDebugMessenger;


public:
	void run();
private:
	//setting up vulkan
	void initWindow();
	void initVulkan();
	void WindowLoop();
	void drawFrame();
	void CleanUp();

	//creating a vkinstance
	void CreateInstance();

	//setting up physical device
	void PhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice Device);
	int rateDevice(VkPhysicalDevice device);


	//setting up queue families
	QueueFamilyIndices findQueueFamily(VkPhysicalDevice device);

	//
	bool checkExtensionSupport(VkPhysicalDevice device);

	//setting up validationLayers
	bool checkForValidationLayerSupport();
	std::vector<const char*> getRequiredextensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void setupDebugMessenger();
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	//setting up logical device
	void createLogicalDevice();

	//creating surface
	void createSurface();

	//swapChain
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createSwapChain();
	void recreateSwapChain();
	void cleanUpSwapChain();

	//image view
	void createImageVeiw();

	void createGraphicsPipeline();

	void createRenderPass();

	void createFramebuffers();

	void createCommandPool();

	void createCommandBuffers();

	void createSyncObj();

	void recordCommandBuffer(VkCommandBuffer buffer, uint32_t index);

	VkShaderModule createShaderModule(const std::vector<char>& code);

	static std::vector<char> readShaderFile(const std::string& fileName);

	void createVertexBuffer();

	void createIndexBuffer();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createDescriptorSetLayout();

	void createUniformBuffers();

	void updateUniformBuffer(uint32_t currentImage);

	void createDescriptorPool();
	void createDescriptorSets();

	const std::vector<const char*> mValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	std::vector<VkImage> swapChainImage;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFrambuffers;

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif // DEBUG



};