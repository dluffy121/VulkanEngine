#include "Window.h"

UPTR<Window> WindowFactory::CreateWindow(int width, int height, const std::string& title, GLFWwindow* sharedWindow, GLFWmonitor* monitor, bool isHidden, bool isDecorated)
{
	return UPTR<Window>{ new Window(width, height, title, sharedWindow, monitor, isHidden, isDecorated) };
}

Window::Window(int width, int height, const std::string& title, GLFWwindow* sharedWindow, GLFWmonitor* monitor, bool isHidden, bool isDecorated)
{
	glfwSwapInterval(1);			// sets swap interval current gl context window i.e. wait for screen updates https://www.glfw.org/docs/3.3/group__context.html#ga6d4e0cdf151b5e579bd67f13202994ed

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	glfwWindowHint(GLFW_VISIBLE, isHidden ? GLFW_FALSE : GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, isDecorated ? GLFW_TRUE : GLFW_FALSE);

	window = glfwCreateWindow(width, height, title.c_str(), monitor, sharedWindow);
}

Window::~Window()
{
	glfwDestroyWindow(window);
}

void Window::Use()
{
	glfwMakeContextCurrent(window);
}