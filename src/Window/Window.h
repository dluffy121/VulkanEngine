#pragma once

#include <Common.h>

namespace VulkanEngine
{
	class Window
	{
	private:
		GLFWwindow* window;

	public:
		Window(int width, int height, const std::string& title, GLFWwindow* sharedWindow = nullptr, GLFWmonitor* monitor = nullptr, bool isHidden = false, bool isDecorated = true);
		~Window();

		void Use();

		inline GLFWwindow* GetGLFWWindow() const { return window; }

	public:
		Window(const VulkanEngine::Window&) = delete;
		VulkanEngine::Window& operator=(const VulkanEngine::Window&) = delete;
	};

	class WindowFactory
	{
	public:
#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma push_macro("CreateWindow")
#undef CreateWindow
#endif // VK_USE_PLATFORM_WIN32_KHR
		static UPTR<VulkanEngine::Window> CreateWindow(int width, int height, const std::string& title, GLFWwindow* sharedWindow = nullptr, GLFWmonitor* monitor = nullptr, bool isHidden = false, bool isDecorated = true);
#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma pop_macro("CreateWindow")
#endif // VK_USE_PLATFORM_WIN32_KHR
	};
};