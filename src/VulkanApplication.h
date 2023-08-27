#pragma once

#include <Common.h>
#include <Window/Window.h>

class VulkanApplication
{
public:
	bool Init();
	void Run();
	void Shutdown();

private:
#pragma region GLFW

	UPTR<Window> window = nullptr;

	bool InitGLFW();
	void ShutdownGLFW();

#pragma endregion

#pragma region Vulkan

	VkInstance instance = VK_NULL_HANDLE;

	bool InitVulkan();
	void ShutdownVulkan();
	bool CreateVulkanInstance();
	std::vector<const char*> GetRequiredExtensions();
	void PrintAvailableExtensions();

#ifdef ENABLE_VK_VAL_LAYERS

	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	bool CheckValidationLayers();

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	bool SetupDebugMessenger();

	void PopulateDebugCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

#endif // ENABLE_VK_VAL_LAYERS

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	bool PickPhysicalDevice();

	UINT16 RateDevice(VkPhysicalDevice devic);

	struct QueueFamilyIndices
	{
		std::optional<UINT32> graphicsFamily;
		std::optional<UINT32> computeFamily;

		inline std::string Print()
		{
			return std::format(
				"gr {} | cmp {}",
				graphicsFamily.value_or(-1),
				computeFamily.value_or(-1)
			);
		}

		inline bool isComplete()
		{
			return graphicsFamily.has_value() && computeFamily.has_value();
		}
	};

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

#pragma endregion


public:
	VulkanApplication(const VulkanApplication&) = delete;
	VulkanApplication& operator=(const VulkanApplication&) = delete;

	inline static UPTR<VulkanApplication> Get()
	{
		UPTR<VulkanApplication> s_instance = UPTR<VulkanApplication>{ new VulkanApplication() };
		return s_instance;
	}

private:
	VulkanApplication() = default;
};