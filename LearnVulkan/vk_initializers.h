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
	//******************************************swap chain**********************************************
	VkSwapchainCreateInfoKHR swapchain_create_info(VkSurfaceKHR surface,uint32_t minImageCount,VkFormat imageFormat,VkColorSpaceKHR imageColorSpace,VkExtent2D imageExtent,uint32_t imageArrayLayers=1,
							VkImageUsageFlags imageUsage= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	VkImageViewCreateInfo imageView_create_info(VkImage image,VkFormat format,uint32_t baseMipLevel,VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
	//******************************************end********swap chain**********************************************
	//******************************************render pass stuff **************************************
	VkAttachmentDescription colorAttachment_des(VkFormat format,VkImageLayout finalLayout= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
	VkAttachmentDescription depthAttachment_des(VkFormat format= VK_FORMAT_D32_SFLOAT, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
	VkSubpassDescription subpass_des(uint32_t colorAttachmentCount,const VkAttachmentReference* pColorAttachments,const VkAttachmentReference* pDepthStencilAttachment,VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
	VkSubpassDependency dependency_des(uint32_t dstSubpass, uint32_t srcSubpass = VK_SUBPASS_EXTERNAL);
	VkRenderPassCreateInfo renderPass_create_info(uint32_t attachmentCount,const VkAttachmentDescription* pAttachments,uint32_t subpassCount,const VkSubpassDescription* pSubpasses,
							uint32_t dependencyCount,const VkSubpassDependency* pDependencies);
	//******************************************end*******render pass stuff **************************************
	//******************************************command pool*****************************************************
	VkCommandPoolCreateInfo commandPool_create_info(uint32_t queueFamilyIndex);

	//******************************************end**************************************************************
	//******************************************Image************************************************************
	VkImageViewCreateInfo view_ceate_info(VkImage image,VkFormat format,  VkImageAspectFlags aspectMask, uint32_t baseMipLevel = 0, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
	VkImageCreateInfo image_create_info(VkFormat format, uint32_t width, uint32_t height,VkImageUsageFlags usage,
						VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, uint32_t mipLevels = 1, VkImageType imageType= VK_IMAGE_TYPE_2D);
	VkMemoryAllocateInfo memoryAllocate_info(VkDeviceSize allocationSize,uint32_t memoryTypeIndex);
	//******************************************end**************************************************************
	//******************************************Texture**********************************************************
	VkImageMemoryBarrier barrier_des(VkImage image,VkImageLayout oldLayout,VkImageLayout newLayout);
	VkBufferCreateInfo buffer_create_info(VkDeviceSize size,VkBufferUsageFlags usage,VkSharingMode sharingMode= VK_SHARING_MODE_EXCLUSIVE);
	VkBufferImageCopy imageCopy_region(uint32_t width, uint32_t height,uint32_t mipLevel = 0 );
	VkSamplerCreateInfo sampler_create_info(VkSamplerAddressMode addressModeU= VK_SAMPLER_ADDRESS_MODE_REPEAT,VkSamplerAddressMode addressModeV= VK_SAMPLER_ADDRESS_MODE_REPEAT,VkSamplerAddressMode addressModeW= VK_SAMPLER_ADDRESS_MODE_REPEAT,
						VkBool32 anisotropyEnable = VK_FALSE,float maxAnisotropy = 1.0f,VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,VkFilter magFilter = VK_FILTER_LINEAR,VkFilter minFilter = VK_FILTER_LINEAR);
	//******************************************end**************************************************************
	//******************************************commandBuffer****************************************************
	VkCommandBufferAllocateInfo alloc_info(VkCommandPool commandPool, uint32_t commandBufferCount=1,VkCommandBufferLevel level= VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo cmdbuf_begin_info(VkCommandBufferUsageFlags flags= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VkSubmitInfo submit_info(const VkCommandBuffer* pCommandBuffers, uint32_t commandBufferCount=1);
	//******************************************end**************************************************************
	//******************************************frameBuffer******************************************************
	VkFramebufferCreateInfo framebuffer_create_info(VkRenderPass renderPass,uint32_t attachmentCount,const VkImageView* pAttachments,uint32_t width,uint32_t height,uint32_t layers=1);
	//******************************************end**************************************************************
	//******************************************descriptorSet**********************************************
	VkDescriptorPoolCreateInfo descriptorPool_create_info(uint32_t maxSets,uint32_t poolSizeCount,const VkDescriptorPoolSize* pPoolSizes);
	VkDescriptorSetLayoutBinding descriptorSet_layout_bindings(uint32_t binding, uint32_t descriptorCount, VkDescriptorType descriptorType,VkShaderStageFlags stageFlags,const VkSampler* pImmutableSamplers=nullptr);
	VkDescriptorSetLayoutCreateInfo descriptorSetLayout_create_info(uint32_t bindingCount,const VkDescriptorSetLayoutBinding* pBindings);
	VkDescriptorSetAllocateInfo descriptorSet_allocate_info(VkDescriptorPool descriptorPool,uint32_t descriptorSetCount,const VkDescriptorSetLayout* pSetLayouts);
	VkWriteDescriptorSet writeDescriptorSets_info(const void* pNext,VkDescriptorSet dstSet,uint32_t dstBinding,VkDescriptorType  descriptorType,
												const VkDescriptorImageInfo* pImageInfo=nullptr,const VkDescriptorBufferInfo* pBufferInfo = nullptr,
												const VkBufferView* pTexelBufferView = nullptr, uint32_t descriptorCount = 1, uint32_t dstArrayElement = 0);
	VkWriteDescriptorSetAccelerationStructureKHR descriptorSetAS_info(const VkAccelerationStructureKHR* pAccelerationStructures, uint32_t accelerationStructureCount=1);
	VkDescriptorBufferInfo buffer_info(VkBuffer buffer,VkDeviceSize offset=0,VkDeviceSize range= VK_WHOLE_SIZE);
	VkDescriptorImageInfo image_info(VkImageView imageView, VkSampler sampler= (VkSampler)VK_DESCRIPTOR_TYPE_SAMPLER,VkImageLayout imageLayout= VK_IMAGE_LAYOUT_GENERAL);
	//******************************************end********************************************************
}