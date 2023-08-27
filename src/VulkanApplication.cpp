#include "VulkanApplication.h"
#include <Window/Window.h>

bool VulkanApplication::Init()
{
	if (!InitGLFW())
		return false;

	if (!InitVulkan())
		return false;

	window = WindowFactory::CreateWindow(800, 600, "Vulkan window");

	return true;
}

void VulkanApplication::Run()
{
	while (!glfwWindowShouldClose(window->GetGLFWWindow()))
	{
		glfwPollEvents();
	}
}

void VulkanApplication::Shutdown()
{
	ShutdownVulkan();
	ShutdownGLFW();
}

#pragma region GLFW

bool VulkanApplication::InitGLFW()
{
	if (GLFW_FALSE == glfwInit())
		return false;

	fprintf(stdout, "Initialized GLFW %s\n", glfwGetVersionString());

	return true;
}

void VulkanApplication::ShutdownGLFW()
{
	glfwTerminate();

	fprintf(stdout, "GLFW Terminated");
}

#pragma endregion

#pragma region Vulkan

bool VulkanApplication::InitVulkan()
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

	if (!PickPhysicalDevice())
		return false;

	return true;
}

void VulkanApplication::ShutdownVulkan()
{
#ifdef ENABLE_VK_VAL_LAYERS
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif // ENABLE_VK_VAL_LAYERS

	vkDestroyInstance(instance, nullptr);

	fprintf(stdout, "Vulkan Instance destroyed\n");
}

bool VulkanApplication::CreateVulkanInstance()
{
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
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
	createInfo.pNext = nullptr;
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

std::vector<const char*> VulkanApplication::GetRequiredExtensions()
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

void VulkanApplication::PrintAvailableExtensions()
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

bool VulkanApplication::CheckValidationLayers()
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

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApplication::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	fprintf(stderr, "Validation layer: \n%s", pCallbackData->pMessage);

	return VK_FALSE;
}

bool VulkanApplication::SetupDebugMessenger()
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

void VulkanApplication::PopulateDebugCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
}

VkResult VulkanApplication::CreateDebugUtilsMessengerEXT(
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

void VulkanApplication::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

#endif // ENABLE_VK_VAL_LAYERS

bool VulkanApplication::PickPhysicalDevice()
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

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	fprintf(stdout, "Device %d '%s' will be used to run application", i, deviceProperties.deviceName);

	return true;
}

UINT16 VulkanApplication::RateDevice(VkPhysicalDevice device)
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

	QueueFamilyIndices qfi = FindQueueFamilies(device);
	if (!qfi.isComplete())
		score = 0;

	fprintf(stdout, "Score : %d		\
					\n\tName: %s	\
					\n\tId : %d		\
					\n\tVendor : %d \
					\n\tDriver: %d	\
					\n\tAPI: %d		\
					\n\tQueues: %s\n",
			score,
			deviceProperties.deviceName,
			deviceProperties.deviceID,
			deviceProperties.vendorID,
			deviceProperties.driverVersion,
			deviceProperties.apiVersion,
			qfi.Print().c_str());

	return score;
}

VulkanApplication::QueueFamilyIndices VulkanApplication::FindQueueFamilies(VkPhysicalDevice device)
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

		if (indices.isComplete())
			break;

		i++;
	}

	return indices;
}

#pragma endregion