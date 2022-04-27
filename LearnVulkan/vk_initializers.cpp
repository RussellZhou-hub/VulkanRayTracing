#include"vk_initializers.h"

VkCommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;

	info.queueFamilyIndex = queueFamilyIndex;
	info.flags = flags;
	return info;
}

VkApplicationInfo vkinit::application_info(const char* pApplicationName, uint32_t applicationVersion, const char* pEngineName, uint32_t engineVersion, uint32_t apiVersion)
{
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = pApplicationName;
	applicationInfo.applicationVersion = applicationVersion;
	applicationInfo.pEngineName = pEngineName;
	applicationInfo.engineVersion = engineVersion;
	applicationInfo.apiVersion = apiVersion;
	return applicationInfo;
}

VkValidationFeaturesEXT vkinit::validation_Features_info(uint32_t enabledValidationFeatureCount, const VkValidationFeatureEnableEXT* pEnabledValidationFeatures, uint32_t disabledValidationFeatureCount, const VkValidationFeatureDisableEXT* pDisabledValidationFeatures)
{
	VkValidationFeaturesEXT validationFeatures{};
	validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validationFeatures.pNext = nullptr;
	validationFeatures.pEnabledValidationFeatures = pEnabledValidationFeatures;
	validationFeatures.enabledValidationFeatureCount = enabledValidationFeatureCount;
	validationFeatures.pDisabledValidationFeatures = pDisabledValidationFeatures;
	validationFeatures.disabledValidationFeatureCount = disabledValidationFeatureCount;
	return validationFeatures;
}

VkInstanceCreateInfo vkinit::instance_create_info(const VkApplicationInfo* pApplicationInfo, uint32_t enabledExtensionCount, const char* const* ppEnabledExtensionNames, VkInstanceCreateFlags flags)
{
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.flags = flags;
	instanceCreateInfo.pApplicationInfo = pApplicationInfo;
	instanceCreateInfo.enabledExtensionCount = enabledExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
	return instanceCreateInfo;
}

std::vector<const char*> vkinit::enableExt_names(const char** deviceEnabledExtensionNames, int count)
{
	std::vector<std::string> ext;
	ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	ext.push_back("VK_KHR_ray_query");
	ext.push_back("VK_KHR_acceleration_structure");
	ext.push_back("VK_KHR_spirv_1_4");
	ext.push_back("VK_KHR_shader_float_controls");
	ext.push_back("VK_KHR_get_memory_requirements2");
	ext.push_back("VK_EXT_descriptor_indexing");
	ext.push_back("VK_KHR_buffer_device_address");
	ext.push_back("VK_KHR_deferred_host_operations");
	ext.push_back("VK_KHR_pipeline_library");
	ext.push_back("VK_KHR_maintenance3");
	ext.push_back("VK_KHR_maintenance1");
	ext.push_back("VK_KHR_shader_non_semantic_info");

	for (auto i = 0; i < count; i++) {
		auto iter = std::find(ext.begin(), ext.end(), std::string(deviceEnabledExtensionNames[i]));
		if (iter == ext.end())
		{
			ext.push_back(deviceEnabledExtensionNames[i]);
		}
	}

	std::vector<const char*> ext_char;
	for (auto i = ext.begin(); i != ext.end(); i++) {
		ext_char.push_back( (*i).c_str() );
	}

	return ext_char;
}

VkDeviceQueueCreateInfo vkinit::device_Queue_create_info(uint32_t queueFamilyIndex, const float* pQueuePriorities, uint32_t queueCount)
{
	VkDeviceQueueCreateInfo deviceQueueCreateInfos = {};
	deviceQueueCreateInfos.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfos.pNext = NULL;
	deviceQueueCreateInfos.flags = 0;
	deviceQueueCreateInfos.queueFamilyIndex = queueFamilyIndex;
	deviceQueueCreateInfos.queueCount = queueCount;
	deviceQueueCreateInfos.pQueuePriorities = pQueuePriorities;
	return deviceQueueCreateInfos;
}

VkDeviceCreateInfo vkinit::device_create_info(const void* pNext, uint32_t queueCreateInfoCount, const VkDeviceQueueCreateInfo* pQueueCreateInfos, uint32_t enabledExtensionCount, const char* const* ppEnabledExtensionNames, const VkPhysicalDeviceFeatures* pEnabledFeatures, uint32_t enabledLayerCount, VkDeviceCreateFlags flags)
{
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = pNext;
	deviceCreateInfo.flags = flags;
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
	deviceCreateInfo.pQueueCreateInfos = pQueueCreateInfos;
	deviceCreateInfo.enabledLayerCount = enabledLayerCount;
	deviceCreateInfo.enabledExtensionCount = enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
	deviceCreateInfo.pEnabledFeatures = pEnabledFeatures;
	return deviceCreateInfo;
}
