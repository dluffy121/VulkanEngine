#pragma once

#include <Common.h>

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
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
};

class WindowFactory
{
public:
	static UPTR<Window> CreateWindow(int width, int height, const std::string& title, GLFWwindow* sharedWindow = nullptr, GLFWmonitor* monitor = nullptr, bool isHidden = false, bool isDecorated = true);
};