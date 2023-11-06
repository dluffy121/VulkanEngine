#include "Window.h"

void VulkanEngine::Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	reinterpret_cast<VulkanEngine::Window*>(glfwGetWindowUserPointer(window))->MarkDirty();
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma push_macro("CreateWindow")
#undef CreateWindow
#endif // VK_USE_PLATFORM_WIN32_KHR
UPTR<VulkanEngine::Window> VulkanEngine::WindowFactory::CreateWindow(int width, int height, const std::string& title, GLFWwindow* sharedWindow, GLFWmonitor* monitor, bool isHidden, bool isDecorated)
#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma pop_macro("CreateWindow")
#endif // VK_USE_PLATFORM_WIN32_KHR
{
	return UPTR<VulkanEngine::Window>{ new VulkanEngine::Window(width, height, title, sharedWindow, monitor, isHidden, isDecorated) };
}

VulkanEngine::Window::Window(int width, int height, const std::string& title, GLFWwindow* sharedWindow, GLFWmonitor* monitor, bool isHidden, bool isDecorated) :
	_framebufferResized(false)
{
	glfwSwapInterval(1);			// sets swap interval current gl context window i.e. wait for screen updates https://www.glfw.org/docs/3.3/group__context.html#ga6d4e0cdf151b5e579bd67f13202994ed

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	glfwWindowHint(GLFW_VISIBLE, isHidden ? GLFW_FALSE : GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, isDecorated ? GLFW_TRUE : GLFW_FALSE);

	_window = glfwCreateWindow(width, height, title.c_str(), monitor, sharedWindow);
	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, VulkanEngine::Window::FramebufferResizeCallback);
}

VulkanEngine::Window::~Window()
{
	glfwDestroyWindow(_window);
}

void VulkanEngine::Window::Use()
{
	glfwMakeContextCurrent(_window);
}

void VulkanEngine::Window::WaitForMaximization()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}
}