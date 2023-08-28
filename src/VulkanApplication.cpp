#include "VulkanApplication.h"
#include <Window/Window.h>

bool VulkanEngine::VulkanApplication::Init()
{
	if (!InitGLFW())
		return false;

#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma push_macro("CreateWindow")
#undef CreateWindow
#endif // VK_USE_PLATFORM_WIN32_KHR
	window = VulkanEngine::WindowFactory::CreateWindow(800, 600, "Vulkan window");
#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma pop_macro("CreateWindow")
#endif // VK_USE_PLATFORM_WIN32_KHR

	if (!InitVulkan())
		return false;

	return true;
}

void VulkanEngine::VulkanApplication::Run()
{
	while (!glfwWindowShouldClose(window->GetGLFWWindow()))
	{
		glfwPollEvents();
	}
}

void VulkanEngine::VulkanApplication::Shutdown()
{
	ShutdownVulkan();
	ShutdownGLFW();
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

	return true;
}

void VulkanEngine::VulkanApplication::ShutdownVulkan()
{
	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

#ifdef ENABLE_VK_VAL_LAYERS
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif // ENABLE_VK_VAL_LAYERS

	vkDestroyInstance(instance, nullptr);

	fprintf(stdout, "Vulkan Instance destroyed\n");
}

bool VulkanEngine::VulkanApplication::CreateVulkanInstance()
{
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = VK_NULL_HANDLE;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto extensions = GetRequiredExtensions();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	PopulateDebugCreateInfo(debugCreateInfo);

	VkInstanceCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef ENABLE_VK_VAL_LAYERS
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else // !ENABLE_VK_VAL_LAYERS
	createInfo.pNext = VK_NULL_HANDLE;
#endif // ENABLE_VK_VAL_LAYERS
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<UINT32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef ENABLE_VK_VAL_LAYERS
	createInfo.enabledLayerCount = static_cast<UINT32>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else // !ENABLE_VK_VAL_LAYERS
	createInfo.enabledLayerCount = 0;
#endif // ENABLE_VK_VAL_LAYERS

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
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

	for (const char* layerName : validationLayers)
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
	fprintf(stderr, "Validation layer: \n%s", pCallbackData->pMessage);

	return VK_FALSE;
}

bool VulkanEngine::VulkanApplication::SetupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
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
	createInfo.pNext = VK_NULL_HANDLE;
	createInfo.flags = 0;
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
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		fprintf(stderr, "No Vulkan supported graphic device found");
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	std::multimap<int, VkPhysicalDevice> candidates;

	UINT8 i = 0;
	for (const auto& device : devices)
	{
		fprintf(stdout, "Device %d : ", i++);
		UINT16 score = RateDevice(device);
		candidates.insert(std::make_pair(score, device));
	}

	physicalDevice = candidates.rbegin()->second;

	if (candidates.rbegin()->first <= 0
		||
		physicalDevice == VK_NULL_HANDLE)
	{
		fprintf(stderr, "No suitable device found to run application");
		return false;
	}

	queueFamilyIndices = FindQueueFamilies(physicalDevice);
	if (!queueFamilyIndices.isComplete())
	{
		fprintf(stderr, "No suitable device found to run application");
		return false;
	}
	fprintf(stdout, "Queue Families :\n\t%s\n", queueFamilyIndices.Print().c_str());

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	fprintf(stdout, "Device %d '%s' will be used to run application", i, deviceProperties.deviceName);

	return true;
}

UINT16 VulkanEngine::VulkanApplication::RateDevice(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

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

VulkanEngine::VulkanApplication::QueueFamilyIndices VulkanEngine::VulkanApplication::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	UINT32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	UINT32 i = 0;
	for (const auto& queueFamiy : queueFamilies)
	{
		if (queueFamiy.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		if (queueFamiy.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.computeFamily = i;

		VkBool32 presentationSupport = VK_FALSE;
		if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentationSupport) != VK_SUCCESS)
			fprintf(stderr, "Unable to check presentation support");
		else
			if (presentationSupport == VK_TRUE)
				indices.presentationFamily = i;

		if (indices.isComplete())
			break;

		i++;
	}

	return indices;
}

void VulkanEngine::VulkanApplication::QueueFamilyIndices::GetCreateInfos(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos)
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
		VkDeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = VK_NULL_HANDLE;
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}
}

bool VulkanEngine::VulkanApplication::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueFamilyIndices.GetCreateInfos(queueCreateInfos);

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = VK_NULL_HANDLE;
	createInfo.flags = 0;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = 0;
#ifdef ENABLE_VK_VAL_LAYERS
	createInfo.enabledLayerCount = static_cast<UINT32>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else // !ENABLE_VK_VAL_LAYERS
	createInfo.enabledLayerCount = 0;
#endif // ENABLE_VK_VAL_LAYERS

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Logical Device");
		return false;
	}

	vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilyIndices.computeFamily.value(), 0, &computeQueue);
	vkGetDeviceQueue(device, queueFamilyIndices.presentationFamily.value(), 0, &presentationQueue);

	fprintf(stdout, "Created Logical Device\n");
	return true;
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
	if (glfwCreateWindowSurface(instance, window.get()->GetGLFWWindow(), nullptr, &surface) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create Window Surface");
		return false;
	}
#endif // VK_USE_PLATFORM_WIN32_KHR

	fprintf(stdout, "Create Window Surface\n");
	return true;
}

#pragma endregion