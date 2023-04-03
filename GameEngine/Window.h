#pragma once


#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "vulkan/vulkan.h"
#include <iostream>


class WindowApp 
{
private:
	const int WIDTH = 900;
	const int HEIGHT = 600;

	GLFWwindow* window = nullptr;
	VkInstance instance;


public:
	void run();
private:

	void initWindow();
	void initVulkan();
	void WindowLoop();
	void CleanUp();
	void CreateInstance();

};