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

	void Mouse(GLFWwindow* window, int button, int action, int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	}

	void Scroll(GLFWwindow* window, double xoffset, double yoffset)
	{
		ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	}
	
	void Char(GLFWwindow* window, unsigned int c)
	{
		ImGui_ImplGlfw_CharCallback(window, c);
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
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		GLFWwindow* const window = glfwCreateWindow(1024, 768, "Mesh Processing", nullptr, nullptr);
		if (!window)
		{
			std::cout << "Failed to create window." << std::endl;
			ret = -2;
		}
		else
		{
			glfwSetKeyCallback(window, glfw::Key);
			glfwSetMouseButtonCallback(window, glfw::Mouse);
			glfwSetScrollCallback(window, glfw::Scroll);
			glfwSetCharCallback(window, glfw::Char);

			glfwMakeContextCurrent(window);
			glfwSwapInterval(1);

			if (!gladLoadGL())
			{
				std::cout << "Failed to load opengl." << std::endl;
				ret = -3;
			}
			else
			{
				ImGui::CreateContext();
				ImGui::StyleColorsDark();
				ImGui_ImplGlfw_InitForOpenGL(window, false);
				ImGui_ImplOpenGL3_Init("#version 150");

				while (!glfwWindowShouldClose(window))
				{
					glfwPollEvents();

					ImGui_ImplOpenGL3_NewFrame();
					ImGui_ImplGlfw_NewFrame();
					ImGui::NewFrame();

					// Stuff


					ImGui::Render();
					int display_w, display_h;
					glfwGetFramebufferSize(window, &display_w, &display_h);
					glViewport(0, 0, display_w, display_h);
					glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT);
					ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

					glfwSwapBuffers(window);
				}

				ImGui_ImplOpenGL3_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
			}

			glfwDestroyWindow(window);
		}
		
		glfwTerminate();
	}

	return ret;
}