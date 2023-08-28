#include <VulkanApplication.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

int main()
{
	{
		UPTR<VulkanEngine::VulkanApplication> va = VulkanEngine::VulkanApplication::Get();

		if (!va->Init())
			return 1;

		va->Run();

		va->Shutdown();
	}

	return 0;
}