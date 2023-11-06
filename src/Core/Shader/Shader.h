#pragma once

#include <Common.h>

namespace VulkanEngine
{
	class Shader
	{
	private:
		const std::string& name;
		const std::string& path;

		VkShaderModule module;

	public:
		Shader(const std::string& file);
		~Shader();

		inline VkShaderModule GetModule() const { return module; }
		void Compile();
	};
}