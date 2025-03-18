// VulkanRenderer.h
#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include "VulkanDevice.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace VulkanEngine {

class VulkanRenderer {
public:
    VulkanRenderer(VulkanDevice& device, VkSurfaceKHR surface, VkExtent2D windowExtent);
    ~VulkanRenderer();

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    VkCommandBuffer beginFrame();
    void endFrame();
    void beginRenderPass(VkCommandBuffer commandBuffer);
    void endRenderPass(VkCommandBuffer commandBuffer);
    bool frameInProgress() const { return isFrameStarted; }
    VkCommandBuffer getCurrentCommandBuffer() const { return commandBuffers[currentFrame]; }
    int getFrameIndex() const { return currentFrame; }
    VkRenderPass getRenderPass() const { return renderPass; }
    VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
    float getAspectRatio() const { return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height); }

    void recreateSwapChain();

private:
    void createSwapChain();
    void cleanupSwapChain();
    void createCommandBuffers();
    void freeCommandBuffers();
    void createSyncObjects();
    void createRenderPass();
    void createFramebuffers();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VulkanDevice& device;
    VkSurfaceKHR surface;
    VkExtent2D windowExtent;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    std::vector<VkCommandBuffer> commandBuffers;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;
    uint32_t currentImageIndex = 0;
    bool isFrameStarted = false;
};

} // namespace VulkanEngine

#endif // VULKANRENDERER_H