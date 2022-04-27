#pragma once
#include<vector>
#include<string>
#include "vk_types.h"

namespace vkinit {
	//vulkan init code goes here
	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	VkApplicationInfo application_info(const char* pApplicationName,uint32_t  applicationVersion= VK_MAKE_VERSION(1, 0, 0),const char* pEngineName="no engine",uint32_t engineVersion= VK_MAKE_VERSION(1, 0, 0),uint32_t  apiVersion= VK_API_VERSION_1_2);
	VkValidationFeaturesEXT validation_Features_info(uint32_t enabledValidationFeatureCount,const VkValidationFeatureEnableEXT* pEnabledValidationFeatures,uint32_t disabledValidationFeatureCount=0,const VkValidationFeatureDisableEXT* pDisabledValidationFeatures=nullptr);
	VkInstanceCreateInfo instance_create_info(const VkApplicationInfo* pApplicationInfo,uint32_t  enabledExtensionCount,const char* const* ppEnabledExtensionNames, VkInstanceCreateFlags flags = 0);
	std::vector<const char*> enableExt_names(const char** deviceEnabledExtensionNames = nullptr,int count=0);
	VkDeviceQueueCreateInfo device_Queue_create_info(uint32_t queueFamilyIndex,const float* pQueuePriorities, uint32_t queueCount=1);
	VkDeviceCreateInfo device_create_info(const void* pNext,uint32_t queueCreateInfoCount,const VkDeviceQueueCreateInfo* pQueueCreateInfos,uint32_t enabledExtensionCount,const char* const* ppEnabledExtensionNames,const VkPhysicalDeviceFeatures* pEnabledFeatures, 
		uint32_t enabledLayerCount=0,VkDeviceCreateFlags flags=0);
}