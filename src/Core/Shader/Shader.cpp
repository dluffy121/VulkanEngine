#include "Shader.h"
#include <VulkanApplication.h>

VulkanEngine::Shader::Shader(const std::string& file) :
	name(FileName(file)),
	path(file)
{
	std::vector<char> code = ReadFile(file);

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const UINT32*>(code.data());

	if (vkCreateShaderModule(VulkanEngine::VulkanApplication::Get()->GetDevice(), &createInfo, nullptr, &module) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");
}

VulkanEngine::Shader::~Shader()
{
	vkDestroyShaderModule(VulkanEngine::VulkanApplication::Get()->GetDevice(), module, nullptr);
}

void VulkanEngine::Shader::Compile()
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = module;
	vertShaderStageInfo.pName = "main";
}
