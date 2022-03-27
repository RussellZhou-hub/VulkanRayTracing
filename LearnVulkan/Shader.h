#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define SEEK_END 2
#define VK_ENABLE_BETA_EXTENSIONS
#define GLFW_INCLUDE_VULKAN
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include<string>
#include "ray_query.h"

using namespace std;

class Shader {
public:
	Shader();
	~Shader();
	void load(VkRayTracingApplication* app, std::string path);
	string judgeTypeByName( string& path);

	VkShaderModule shaderModule;
	bool isUsed;  //是否都被加载过
	char* shaderFileBuffer;
	VkRayTracingApplication* app;
	VkPipelineShaderStageCreateInfo ShaderStageInfo;
};