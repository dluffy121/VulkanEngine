#pragma once

#include <Common.h>
#include <Window/Window.h>

namespace VulkanEngine
{
	class VulkanApplication
	{
	public:
		bool Init();
		void Run();
		void Shutdown();
		void DrawFrame();

		VkDevice GetDevice() const { return _device; }

	private:

#pragma region GLFW

		UPTR<Window> _window = nullptr;

		bool InitGLFW();
		void ShutdownGLFW();

#pragma endregion

#pragma region Vulkan

		VkInstance _instance = VK_NULL_HANDLE;

		bool InitVulkan();
		void ShutdownVulkan();
		bool CreateVulkanInstance();
		std::vector<const char*> GetRequiredExtensions();
		void PrintAvailableExtensions();

#ifdef ENABLE_VK_VAL_LAYERS

		const std::vector<const char*> _validationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};

		bool CheckValidationLayers();

		VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

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

		VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;

		bool PickPhysicalDevice();

		UINT16 RateDevice(VkPhysicalDevice physicalDevice);

		struct QueueFamilyIndices
		{
			std::optional<UINT32> graphicsFamily;
			std::optional<UINT32> computeFamily;
			std::optional<UINT32> presentationFamily;

			inline UINT32 GetSize() const { return 3; }

			inline std::string Print() const
			{
				return std::format(
					"gr {} | cmp {} | prsn {}",
					graphicsFamily.value_or(-1),
					computeFamily.value_or(-1),
					presentationFamily.value_or(-1)
				);
			}

			inline bool IsComplete() const
			{
				return graphicsFamily.has_value()
					&& computeFamily.has_value()
					&& presentationFamily.has_value();
			}

			inline void GetIndices(std::vector<UINT32>& indices) const
			{
				indices[0] = graphicsFamily.value();
				indices[1] = computeFamily.value();
				indices[2] = presentationFamily.value();
			}

			void GetCreateInfos(std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos) const;
		};

		QueueFamilyIndices _queueFamilyIndices;

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice);

		VkDevice _device = VK_NULL_HANDLE;

		bool CreateLogicalDevice();

		VkQueue _graphicsQueue = VK_NULL_HANDLE;
		VkQueue _computeQueue = VK_NULL_HANDLE;
		VkQueue _presentationQueue = VK_NULL_HANDLE;

		const std::vector<const char*> deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;

			inline bool IsAdequate()
			{
				return !formats.empty()
					&& !presentModes.empty();
			}
		};

		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

#pragma endregion

#pragma region Surfacce

		VkSurfaceKHR _surface;

		bool CreateSurface();

#pragma endregion

#pragma region Swap Chain

		VkSwapchainKHR _swapChain;
		std::vector<VkImage> _images;
		VkFormat _swapChainImageFormat;
		VkExtent2D _swapChainExtent;

		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

		bool CreateSwapChain();

#pragma endregion

#pragma region Swap Chain

		std::vector<VkImageView> _imageViews;

		bool CreateImageViews();

		bool ReCreateSwapChain();

		void CleanupSwapChain();

#pragma endregion

#pragma region Render Pass

		VkRenderPass _renderPass = VK_NULL_HANDLE;

		bool CreateRenderPass();

#pragma endregion

#pragma region Graphics Pipeline

		VkShaderModule _vertShaderModule;
		VkShaderModule _fragShaderModule;

		bool CreateGraphicsPipeline();

		VkPipelineLayout _pipelineLayout;
		VkPipeline _graphicsPipeline;

#pragma endregion

#pragma region Graphics Pipeline

		std::vector<VkFramebuffer> _frameBuffers;

		bool CreateFrameBuffers();

#pragma endregion

#pragma region Commands

		const UINT8 MAX_FRAMES_IN_FLIGHT = 2;

		VkCommandPool _commandPool;
		std::vector<VkCommandBuffer> _commandBuffers{ MAX_FRAMES_IN_FLIGHT };

		bool CreateCommandPool();
		bool CreateCommandBuffers();

		VkClearValue clearColor =
		{
			{ 0.f, 0.f, 0.f, 1.f },
		};

		bool RecordCommandBuffer(VkCommandBuffer commandBuffer);
		bool EndRecordCommandBuffer(VkCommandBuffer commandBuffer);

		void BeginRenderPass(VkCommandBuffer commandBuffer, UINT32 imageIndex);
		void EndRenderPass(VkCommandBuffer commandBuffer);

		void BindPipeline(VkCommandBuffer commandBuffer);

		void SetupViewport(VkCommandBuffer commandBuffer);
		void SetupScissor(VkCommandBuffer commandBuffer);


#pragma endregion

#pragma region Draw

		UINT8 _currentFrame = 0;

		void Draw(VkCommandBuffer commandBuffer, UINT32 imageIndex);

#pragma endregion

#pragma region Synchronization

		std::vector<VkSemaphore> _imageAvailableSemaphores{ MAX_FRAMES_IN_FLIGHT };
		std::vector<VkSemaphore> _renderFinishSemaphores{ MAX_FRAMES_IN_FLIGHT };
		std::vector<VkFence> _frameFences{ MAX_FRAMES_IN_FLIGHT };

		bool CreateSyncObjects();
		void DestroySyncObjects();

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
}