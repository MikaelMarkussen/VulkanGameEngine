// GameEngine.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

int main()
{
	glfwInit();

	if (glfwVulkanSupported()) 
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		GLFWwindow* window = glfwCreateWindow(800, 600, "Mikael's GameEngine", nullptr, nullptr);
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::cout << extensionCount << " Extension supportet\n";
		
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	else
		return 0;
}
