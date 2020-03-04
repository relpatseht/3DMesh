#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include <iostream>

namespace glfw
{
	static void Error(int /*error*/, const char* description)
	{
		std::cout << "Error: " << description << std::endl;
	}

	void Key(GLFWwindow* window, int key, int scancode, int action, int mode)
	{
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mode);
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	int ret = 0;

	glfwSetErrorCallback(&glfw::Error);

	if (!glfwInit())
	{
		std::cout << "Failed to initalize glfw" << std::endl;
		ret = -1;
	}
	else
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

		GLFWwindow* const window = glfwCreateWindow(1024, 768, "Mesh Processing", nullptr, nullptr);
		if (!window)
		{
			std::cout << "Failed to create window." << std::endl;
			ret = -2;
		}
		else
		{
			glfwSetKeyCallback(window, glfw::Key);
			glfwSetKeyCallback(window, glfw::Mouse);
		}
		
		glfwTerminate();
	}

	return ret;
}