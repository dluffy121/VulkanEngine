#include <VulkanApplication.h>
#include <Window/Window.h>

bool VulkanEngine::VulkanApplication::Init()
{
	if (!InitGLFW())
		return false;

#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma push_macro("CreateWindow")
#undef CreateWindow
#endif // VK_USE_PLATFORM_WIN32_KHR
	_window = VulkanEngine::WindowFactory::CreateWindow(800, 600, "Vulkan window");
#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma pop_macro("CreateWindow")
#endif // VK_USE_PLATFORM_WIN32_KHR

	if (!InitVulkan())
		return false;

	return true;
}

void VulkanEngine::VulkanApplication::Run()
{
	while (!glfwWindowShouldClose(_window->GetGLFWWindow()))
	{
		glfwPollEvents();

		DrawFrame();
	}

	// This waits for device to complete all async operations and thus making async objects releasable
	vkDeviceWaitIdle(_device);
}

void VulkanEngine::VulkanApplication::Shutdown()
{
	ShutdownVulkan();
	ShutdownGLFW();
}

void VulkanEngine::VulkanApplication::DrawFrame()
{
	vkWaitForFences(_device, 1, &_frameFences[_currentFrame], VK_TRUE, UINT64_MAX);

	UINT32 imageIndex;
	VkResult result = vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		ReCreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to acquire swap chain image!");

	vkResetFences(_device, 1, &_frameFences[_currentFrame]);

	vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);

	Draw(_commandBuffers[_currentFrame], imageIndex);

	VkSemaphore waitSemaphores[]
	{
		_imageAvailableSemaphores[_currentFrame]
	};
	VkPipelineStageFlags waitStages[]
	{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSemaphore signalSemaphores[]
	{
		_renderFinishSemaphores[_currentFrame]
	};

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(_presentationQueue, 1, &submitInfo, _frameFences[_currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command to queue");

	VkSwapchainKHR swapChains[]
	{
		_swapChain
	};

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &_swapChain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(_presentationQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _window.get()->IsDirty())
	{
		_window.get()->Clean();
		ReCreateSwapChain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain Image");

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

#pragma region GLFW

bool VulkanEngine::VulkanApplication::InitGLFW()
{
	if (GLFW_FALSE == glfwInit())
		return false;

	fprintf(stdout, "Initialized GLFW %s\n", glfwGetVersionString());

	return true;
}

void VulkanEngine::VulkanApplication::ShutdownGLFW()
{
	glfwTerminate();

	fprintf(stdout, "GLFW Terminated");
}

#pragma endregion

#pragma region Vulkan

bool VulkanEngine::VulkanApplication::InitVulkan()
{
#ifdef ENABLE_VK_VAL_LAYERS
	if (!CheckValidationLayers())
		return false;
#endif // ENABLE_VK_VAL_LAYERS

	if (!CreateVulkanInstance())
		return false;

	PrintAvailableExtensions();

#ifdef ENABLE_VK_VAL_LAYERS
	if (!SetupDebugMessenger())
		return false;
#endif // ENABLE_VK_VAL_LAYERS

	if (!CreateSurface())
		return false;

	if (!PickPhysicalDevice())
		return false;

	if (!CreateLogicalDevice())
		return false;

	if (!CreateSwapChain())
		return false;

	if (!CreateImageViews())
		return false;

	if (!CreateRenderPass())
		return false;

	if (!CreateGraphicsPipeline())
		return false;

	if (!CreateFrameBuffers())
		return false;

	if (!CreateCommandPool())
		return false;

	if (!CreateCommandBuffers())
		return false;

	if (!CreateSyncObjects())
		return false;

	return true;
}

void VulkanEngine::VulkanApplication::ShutdownVulkan()
{
	CleanupSwapChain();

	vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);

	vkDestroyRenderPass(_device, _renderPass, nullptr);

	DestroySyncObjects();

	vkDestroyCommandPool(_device, _commandPool, nullptr);

	vkDestroyDevice(_device, nullptr);

#ifdef ENABLE_VK_VAL_LAYERS
	DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
#endif // ENABLE_VK_VAL_LAYERS

	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkDestroyInstance(_instance, nullptr);

	fprintf(stdout, "Vulkan Instance destroyed\n");
}

bool VulkanEngine::VulkanApplication::CreateVulkanInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto extensions = GetRequiredExtensions();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	PopulateDebugCreateInfo(debugCreateInfo);

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef ENABLE_VK_VAL_LAYERS
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif // ENABLE_VK_VAL_LAYERS
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<UINT32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef ENABLE_VK_VAL_LAYERS
	createInfo.enabledLayerCount = static_cast<UINT32>(_validationLayers.size());
	createInfo.ppEnabledLayerNames = _validationLayers.data();
#endif // ENABLE_VK_VAL_LAYERS

	if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Vulkan Instance....");
		return false;
	}

	fprintf(stdout, "Vulkan Instance created\n");

	return true;
}

std::vector<const char*> VulkanEngine::VulkanApplication::GetRequiredExtensions()
{
	UINT32 glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef ENABLE_VK_VAL_LAYERS
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // ENABLE_VK_VAL_LAYERS

	return extensions;
}

void VulkanEngine::VulkanApplication::PrintAvailableExtensions()
{
	fprintf(stdout, "Checking supported extensions\n");

	UINT32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	fprintf(stdout, "%d extensions supported\n", extensionCount);
	for (const VkExtensionProperties& extension : availableExtensions)
		fprintf(stdout, "\t %s extensions supported\n", extension.extensionName);
}

#ifdef ENABLE_VK_VAL_LAYERS

bool VulkanEngine::VulkanApplication::CheckValidationLayers()
{
	fprintf(stdout, "Checking supported validation layers\n");

	UINT32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	bool success = true;

	for (const char* layerName : _validationLayers)
	{
		bool layerfound = false;

		for (const VkLayerProperties& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerfound = true;
				break;
			}
		}

		success &= layerfound;

		if (layerfound)
			fprintf(stdout, "\t %s layer found\n", layerName);
		else
			fprintf(stderr, "\t %s layer NOT found\n", layerName);
	}

	if (!success)
	{
		fprintf(stderr, "Vulkan Validation layers check failed....");
		return false;
	}

	fprintf(stdout, "Vulkan Validation layers check successful\n");

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::VulkanApplication::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	fprintf(stderr, "Debug Validation layer: \n%s\n\n", pCallbackData->pMessage);

	return VK_FALSE;
}

bool VulkanEngine::VulkanApplication::SetupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
	{
		fprintf(stderr, "Debug Messenger setup failed");
		return false;
	}

	fprintf(stdout, "Debug Messenger setup successful\n");

	return true;
}

void VulkanEngine::VulkanApplication::PopulateDebugCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
}

VkResult VulkanEngine::VulkanApplication::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanEngine::VulkanApplication::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

#endif // ENABLE_VK_VAL_LAYERS

bool VulkanEngine::VulkanApplication::PickPhysicalDevice()
{
	UINT32 deviceCount;
	vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		fprintf(stderr, "No Vulkan supported graphic device found");
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

	std::multimap<int, VkPhysicalDevice> candidates;

	UINT8 i = 0;
	for (const auto& device : devices)
	{
		fprintf(stdout, "Device %d : ", i++);
		UINT16 score = RateDevice(device);
		candidates.insert(std::make_pair(score, device));
	}

	_physicalDevice = candidates.rbegin()->second;

	if (candidates.rbegin()->first <= 0
		||
		_physicalDevice == VK_NULL_HANDLE)
	{
		fprintf(stderr, "No suitable device found to run application");
		return false;
	}

	_queueFamilyIndices = FindQueueFamilies(_physicalDevice);
	if (!_queueFamilyIndices.IsComplete())
	{
		fprintf(stderr, "No suitable device found to run application");
		return false;
	}
	fprintf(stdout, "Queue Families :\n\t%s\n", _queueFamilyIndices.Print().c_str());

	if (!CheckDeviceExtensionSupport(_physicalDevice))
	{
		fprintf(stderr, "No suitable device found to run application");
		return false;
	}

	SwapChainSupportDetails details = QuerySwapChainSupport(_physicalDevice, _surface);
	if (!details.IsAdequate())
	{
		fprintf(stderr, "No suitable device found to run application");
		return false;
	}

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);
	fprintf(stdout, "Device %d '%s' will be used to run application\n", i, deviceProperties.deviceName);

	return true;
}

UINT16 VulkanEngine::VulkanApplication::RateDevice(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	UINT16 score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader)
		score = 0;

	fprintf(stdout, "Score : %d		\
					\n\tName: %s	\
					\n\tId : %d		\
					\n\tVendor : %d \
					\n\tDriver: %d	\
					\n\tAPI: %d\n",
			score,
			deviceProperties.deviceName,
			deviceProperties.deviceID,
			deviceProperties.vendorID,
			deviceProperties.driverVersion,
			deviceProperties.apiVersion);

	return score;
}

VulkanEngine::VulkanApplication::QueueFamilyIndices VulkanEngine::VulkanApplication::FindQueueFamilies(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices indices;

	UINT32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	UINT32 i = 0;
	for (const auto& queueFamiy : queueFamilies)
	{
		if (queueFamiy.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		if (queueFamiy.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.computeFamily = i;

		VkBool32 presentationSupport = VK_FALSE;
		if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, _surface, &presentationSupport) != VK_SUCCESS)
			fprintf(stderr, "Unable to check presentation support");
		else
			if (presentationSupport == VK_TRUE)
				indices.presentationFamily = i;

		if (indices.IsComplete())
			break;

		i++;
	}

	return indices;
}

void VulkanEngine::VulkanApplication::QueueFamilyIndices::GetCreateInfos(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos) const
{
	std::set<UINT32> uniqueFamilyIndices =
	{
		graphicsFamily.value(),
		computeFamily.value(),
		presentationFamily.value()
	};

	float queuePriority = 1.f;

	for (const auto& index : uniqueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}
}

bool VulkanEngine::VulkanApplication::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	_queueFamilyIndices.GetCreateInfos(queueCreateInfos);

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
#ifdef ENABLE_VK_VAL_LAYERS
	createInfo.enabledLayerCount = static_cast<UINT32>(_validationLayers.size());
	createInfo.ppEnabledLayerNames = _validationLayers.data();
#endif // ENABLE_VK_VAL_LAYERS

	if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Logical Device");
		return false;
	}

	vkGetDeviceQueue(_device, _queueFamilyIndices.graphicsFamily.value(), 0, &_graphicsQueue);
	vkGetDeviceQueue(_device, _queueFamilyIndices.computeFamily.value(), 0, &_computeQueue);
	vkGetDeviceQueue(_device, _queueFamilyIndices.presentationFamily.value(), 0, &_presentationQueue);

	fprintf(stdout, "Created Logical Device\n");
	return true;
}

bool VulkanEngine::VulkanApplication::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	UINT32 extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
		if (requiredExtensions.empty())
			return true;
	}

	return false;
}

VulkanEngine::VulkanApplication::SwapChainSupportDetails VulkanEngine::VulkanApplication::QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	UINT32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	UINT32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

bool VulkanEngine::VulkanApplication::CreateSurface()
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = glfwGetWin32Window(window.get()->GetGLFWWindow());
	createInfo.hinstance = GetModuleHandle(nullptr);

	if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Window Surface");
		return false;
	}
#else // !VK_USE_PLATFORM_WIN32_KHR
	if (glfwCreateWindowSurface(_instance, _window.get()->GetGLFWWindow(), nullptr, &_surface) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Window Surface");
		return false;
	}
#endif // VK_USE_PLATFORM_WIN32_KHR

	fprintf(stdout, "Created Window Surface\n");
	return true;
}

#pragma endregion

#pragma region Swap Chain

VkExtent2D VulkanEngine::VulkanApplication::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<UINT32>().max())
		return capabilities.currentExtent;

	// we use this as fallback incase the resolution of window created by 
	// the window manager (in this case GLFW) is not detected by Vulkan
	// In those cases, Vulkan sets the width of currentExtent to max of UINT32
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#platformCreateSurface_win32
	int fbWidth, fbHeight;

	// We cannot query for window size as the coordinated of window size are 
	// virtual screen coordinates and will differ in pixels of screen
	// Hence we use Frame Buffer size
	// https://www.glfw.org/docs/3.3/group__window.html#ga0e2637a4161afb283f5300c7f94785c9
	glfwGetFramebufferSize(_window->GetGLFWWindow(), &fbWidth, &fbHeight);

	VkExtent2D extent
	{
		static_cast<UINT32>(fbWidth),
		static_cast<UINT32>(fbHeight),
	};

	extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return extent;
}

VkPresentModeKHR VulkanEngine::VulkanApplication::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
	}

	// This mode is always present and guaranteed to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR VulkanEngine::VulkanApplication::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
			&&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}

bool VulkanEngine::VulkanApplication::CreateSwapChain()
{
	SwapChainSupportDetails details = QuerySwapChainSupport(_physicalDevice, _surface);

	VkExtent2D extent = ChooseSwapExtent(details.capabilities);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(details.presentModes);
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(details.formats);

	// We need + 1 as its not always necessary to rely on drivers to give us
	// an image to render to at the exact moment we perform render operations
	// this count can be increased based on the hardware capabilities
	UINT32 imageCount = details.capabilities.minImageCount + 1;

	// maxImageCount = 0 means, there is no limit
	// If minImageCount + 1 exceeds maxImageCount we use that instead
	if (details.capabilities.maxImageCount > 0
		&&
		imageCount > details.capabilities.maxImageCount)
		imageCount = details.capabilities.maxImageCount;

	std::vector<UINT32> queueIndices(_queueFamilyIndices.GetSize());
	_queueFamilyIndices.GetIndices(queueIndices);

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = _surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if ((queueIndices[0] != queueIndices[1]) != queueIndices[2])
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = static_cast<UINT32>(queueIndices.size());
		createInfo.pQueueFamilyIndices = queueIndices.data();
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = VK_NULL_HANDLE;
	}

	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapChain) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Swap Chain\n");
		return false;
	}

	// Get the created Images from SwapChain
	vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
	_images.resize(imageCount);
	vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _images.data());

	_swapChainImageFormat = createInfo.imageFormat;
	_swapChainExtent = createInfo.imageExtent;

	fprintf(stdout, "Created Swap Chain\n");
	return true;
}

#pragma endregion

#pragma region Swap Chain

bool VulkanEngine::VulkanApplication::CreateImageViews()
{
	_imageViews.resize(_images.size());

	for (UINT32 i = 0; i < _imageViews.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = _images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = _swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(_device, &createInfo, nullptr, &_imageViews[i]) != VK_SUCCESS)
		{
			fprintf(stderr, "Failed to create %d%s Image View", i + 1, i == 0 ? "st" : i == 1 ? "nd" : i == 2 ? "rd" : "th");
			return false;
		}

		fprintf(stdout, "Created %d%s Image View\n", i + 1, i == 0 ? "st" : i == 1 ? "nd" : i == 2 ? "rd" : "th");
	}

	return true;
}

bool VulkanEngine::VulkanApplication::ReCreateSwapChain()
{
	_window.get()->WaitForMaximization();

	vkDeviceWaitIdle(_device);

	CleanupSwapChain();

	if (!CreateSwapChain() || !CreateImageViews() || !CreateFrameBuffers())
	{
		fprintf(stderr, "Swap chain recreation failed\n");
		return false;
	}

	return true;
}

void VulkanEngine::VulkanApplication::CleanupSwapChain()
{
	for (auto frameBuffer : _frameBuffers)
		vkDestroyFramebuffer(_device, frameBuffer, nullptr);

	for (auto imageView : _imageViews)
		vkDestroyImageView(_device, imageView, nullptr);

	vkDestroySwapchainKHR(_device, _swapChain, nullptr);
}

#pragma endregion

#pragma region Render Pass

bool VulkanEngine::VulkanApplication::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = _swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // index of attachment
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // this means former subpass, if same mentioned in dst then, subpass after this
	subpassDependency.dstSubpass = 0; // 0 meaning current subpass, dst > src to avoid cyclic dependency
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &subpassDependency;

	if (vkCreateRenderPass(_device, &createInfo, nullptr, &_renderPass) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Render Pass\n");
		return false;
	}

	fprintf(stdout, "Created Render Pass\n");
	return true;
}

#pragma endregion

#pragma region Graphics Pipeline

bool VulkanEngine::VulkanApplication::CreateGraphicsPipeline()
{
	// Shader
	std::vector<char> vertCode = ReadFile("res/Shaders/Triangle.vert.spv");

	VkShaderModuleCreateInfo vertShaderCreateInfo{};
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertShaderCreateInfo.codeSize = vertCode.size();
	vertShaderCreateInfo.pCode = reinterpret_cast<const UINT32*>(vertCode.data());

	if (vkCreateShaderModule(_device, &vertShaderCreateInfo, nullptr, &_vertShaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	std::vector<char> fragCode = ReadFile("res/Shaders/Triangle.frag.spv");

	VkShaderModuleCreateInfo fragShaderCreateInfo{};
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragShaderCreateInfo.codeSize = fragCode.size();
	fragShaderCreateInfo.pCode = reinterpret_cast<const UINT32*>(fragCode.data());

	if (vkCreateShaderModule(_device, &fragShaderCreateInfo, nullptr, &_fragShaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = _vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = _fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] =
	{
		vertShaderStageInfo,
		fragShaderStageInfo
	};

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	// Input Assembly - structure of input
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Dynamic Render states
	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<UINT32>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// Viewport And Scissors
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)_swapChainExtent.width;
	viewport.height = (float)_swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = _swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasClamp = 0;
	rasterizer.depthBiasConstantFactor = 0;
	rasterizer.depthBiasSlopeFactor = 0;
	rasterizer.lineWidth = 1.f;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// Color Blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	// Blending working
	//	if (blendEnable)
	//	{
	//		finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
	//		finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
	//	}
	//	else
	//		finalColor = newColor;
	//	finalColor = finalColor & colorWriteMask;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Graphic Pipeline Layout\n");
		return false;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = _renderPass;
	pipelineInfo.subpass = 0; // index of subpass
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Graphic Pipeline");
		return false;
	}

	vkDestroyShaderModule(_device, _vertShaderModule, nullptr);
	vkDestroyShaderModule(_device, _fragShaderModule, nullptr);

	fprintf(stdout, "Created Graphics Pipeline\n");
	return true;
}

bool VulkanEngine::VulkanApplication::CreateFrameBuffers()
{
	_frameBuffers.resize(_imageViews.size());

	for (UINT8 i = 0; i < _imageViews.size(); i++)
	{
		VkImageView attachments[] =
		{
			_imageViews[i]
		};

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = _renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = _swapChainExtent.width;
		createInfo.height = _swapChainExtent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(_device, &createInfo, nullptr, &_frameBuffers[i]) != VK_SUCCESS)
		{
			fprintf(stderr, "Failed to create %d%s Frame Buffer", i + 1, i == 0 ? "st" : i == 1 ? "nd" : i == 2 ? "rd" : "th");
			return false;
		}

		fprintf(stdout, "Created %d%s Frame Buffer\n", i + 1, i == 0 ? "st" : i == 1 ? "nd" : i == 2 ? "rd" : "th");
	}

	return true;
}

bool VulkanEngine::VulkanApplication::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(_physicalDevice);

	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();


	if (vkCreateCommandPool(_device, &createInfo, nullptr, &_commandPool) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Command Pool");
		return false;
	}

	fprintf(stdout, "Created Command Pool\n");

	return true;
}

bool VulkanEngine::VulkanApplication::CreateCommandBuffers()
{
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = _commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = static_cast<UINT32>(_commandBuffers.size());

	if (vkAllocateCommandBuffers(_device, &allocateInfo, _commandBuffers.data()) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Command Buffer");
		return false;
	}

	fprintf(stdout, "Created Command Buffer\n");
	return true;
}

bool VulkanEngine::VulkanApplication::RecordCommandBuffer(VkCommandBuffer commandBuffer)
{
	VkCommandBufferBeginInfo cbBeginInfo{};
	cbBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cbBeginInfo.pInheritanceInfo = nullptr;

	return vkBeginCommandBuffer(commandBuffer, &cbBeginInfo) != VK_SUCCESS;
}

bool VulkanEngine::VulkanApplication::EndRecordCommandBuffer(VkCommandBuffer commandBuffer)
{
	return vkEndCommandBuffer(commandBuffer) != VK_SUCCESS;
}

void VulkanEngine::VulkanApplication::BeginRenderPass(VkCommandBuffer commandBuffer, UINT32 imageIndex)
{
	VkRenderPassBeginInfo rpBeginInfo{};
	rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBeginInfo.renderPass = _renderPass;
	rpBeginInfo.framebuffer = _frameBuffers[imageIndex];
	rpBeginInfo.renderArea.offset = { 0, 0 };
	rpBeginInfo.renderArea.extent = _swapChainExtent;
	rpBeginInfo.clearValueCount = 1;
	rpBeginInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanEngine::VulkanApplication::EndRenderPass(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

void VulkanEngine::VulkanApplication::BindPipeline(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
}

void VulkanEngine::VulkanApplication::SetupViewport(VkCommandBuffer commandBuffer)
{
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(_swapChainExtent.width);
	viewport.height = static_cast<float>(_swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void VulkanEngine::VulkanApplication::SetupScissor(VkCommandBuffer commandBuffer)
{
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = _swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanEngine::VulkanApplication::Draw(VkCommandBuffer commandBuffer, UINT32 imageIndex)
{
	if (RecordCommandBuffer(commandBuffer))
		throw std::runtime_error("Failed to begin recording Command Buffer");
	BeginRenderPass(commandBuffer, imageIndex);
	BindPipeline(commandBuffer);
	SetupViewport(commandBuffer);
	SetupScissor(commandBuffer);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	EndRenderPass(commandBuffer);
	if (EndRecordCommandBuffer(commandBuffer))
		throw std::runtime_error("Failed to record Command Buffer");
}

bool VulkanEngine::VulkanApplication::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	// Create Fence in signaled state as we are waiting for its signal when frame starts
	// this will help avoid design issue where the first frame will wait indefinitely (or UINT64)
	// as the fence default state after creation will be unsignaled otherwise

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderFinishSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frameFences[i]) != VK_SUCCESS)
		{
			fprintf(stderr, "Failed to create Sync Objects\n");
			return false;
		}
	}

	fprintf(stdout, "Created Sync Objects\n");
	return true;
}

void VulkanEngine::VulkanApplication::DestroySyncObjects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(_device, _renderFinishSemaphores[i], nullptr);
		vkDestroyFence(_device, _frameFences[i], nullptr);
	}
}

#pragma endregion