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
	VkRenderPassBeginInfo renderPass_begin_info(VkRenderPass renderPass,VkFramebuffer framebuffer, VkExtent2D extent,uint32_t clearValueCount,const VkClearValue* pClearValues=nullptr, VkOffset2D renderAreaOffset = { 0, 0 });
	//******************************************end*******render pass stuff **************************************
	//******************************************command pool*****************************************************
	VkCommandPoolCreateInfo commandPool_create_info(uint32_t queueFamilyIndex);

	//******************************************end**************************************************************
	//******************************************Image************************************************************
	VkImageViewCreateInfo view_ceate_info(VkImage image,VkFormat format,  VkImageAspectFlags aspectMask, uint32_t baseMipLevel = 0, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
	VkImageCreateInfo image_create_info(VkFormat format, uint32_t width, uint32_t height,VkImageUsageFlags usage,
						VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, uint32_t mipLevels = 1, VkImageType imageType= VK_IMAGE_TYPE_2D);
	VkMemoryAllocateInfo memoryAllocate_info(VkDeviceSize allocationSize,uint32_t memoryTypeIndex);
	VkImageSubresourceRange subresource_range(uint32_t baseMipLevel=0,uint32_t levelCount=1,uint32_t baseArrayLayer=0,uint32_t layerCount=1, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
	VkImageSubresourceLayers subresource_layers(uint32_t mipLevel=0,uint32_t baseArrayLayer=0,uint32_t layerCount=1, VkImageAspectFlags  aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
	VkOffset3D offset(int32_t x = 0, int32_t y = 0, int32_t z = 0);
	VkExtent3D extent(uint32_t width, uint32_t height, uint32_t depth = 1);
	VkImageCopy imageCopy(VkImageSubresourceLayers srcSubresource, VkOffset3D srcOffset, VkImageSubresourceLayers dstSubresource, VkOffset3D dstOffset, VkExtent3D extent);
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
	VkDescriptorImageInfo* get_textures_descriptor_ImageInfos(uint32_t texture_count, std::vector<Texture> tex);
	//******************************************end********************************************************
	// *****************************************AS*********************************************************
	VkAccelerationStructureGeometryTrianglesDataKHR AS_GeometryTriangles_data(VkDeviceOrHostAddressConstKHR vertexData,VkDeviceSize vertexStride,uint32_t maxVertex,VkDeviceOrHostAddressConstKHR indexData,
													VkDeviceOrHostAddressConstKHR transformData,VkIndexType indexType = VK_INDEX_TYPE_UINT32, VkFormat vertexFormat = VK_FORMAT_R32G32B32_SFLOAT);
	VkAccelerationStructureBuildRangeInfoKHR AS_BuildRangeInfoKHR(uint32_t primitiveCount,uint32_t primitiveOffset=0,uint32_t firstVertex = 0,uint32_t transformOffset = 0);
	// *****************************************end********************************************************
	//******************************************Pipeline***************************************************
	VkVertexInputBindingDescription vertexBinding_des(uint32_t binding,uint32_t stride,VkVertexInputRate inputRate);
	VkVertexInputAttributeDescription vertexAttribute_des(uint32_t location,uint32_t binding,uint32_t offset,VkFormat format);
	VkPipelineVertexInputStateCreateInfo vertexInputState_create_info(const VkVertexInputBindingDescription* pVertexBindingDescriptions,const VkVertexInputAttributeDescription* pVertexAttributeDescriptions,
																uint32_t vertexBindingDescriptionCount = 1,uint32_t vertexAttributeDescriptionCount=1);
	VkPipelineInputAssemblyStateCreateInfo inputAssembly_create_info(VkPrimitiveTopology topology,VkBool32 primitiveRestartEnable);
	VkViewport viewport_des(float x,float y,float width,float height,float minDepth,float maxDepth);
	VkRect2D scissor(VkOffset2D offset, VkExtent2D extent);
	VkPipelineViewportStateCreateInfo viewportState_create_info(const VkViewport* pViewports,const VkRect2D* pScissors, uint32_t viewportCount=1, uint32_t scissorCount=1);
	VkPipelineRasterizationStateCreateInfo rasterizationState_create_info(VkBool32  depthBiasEnable = VK_FALSE,float lineWidth=1.0f, VkBool32 depthClampEnable = VK_FALSE,
											VkCullModeFlags  cullMode = VK_CULL_MODE_BACK_BIT, VkBool32 rasterizerDiscardEnable = VK_FALSE, 
											VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL);
	VkPipelineMultisampleStateCreateInfo multisampleState_create_info(VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,VkBool32 sampleShadingEnable = VK_FALSE);
	VkPipelineDepthStencilStateCreateInfo depthStencil_create_info(VkBool32 stencilTestEnable = VK_FALSE, VkBool32 depthBoundsTestEnable = VK_FALSE, VkBool32 depthWriteEnable = VK_TRUE,
											VkBool32 depthTestEnable = VK_TRUE,VkCompareOp depthCompareOp= VK_COMPARE_OP_LESS);
	VkPipelineColorBlendStateCreateInfo colorBlendState_create_info(uint32_t attachmentCount,const VkPipelineColorBlendAttachmentState* pAttachments,
	float* blendConstants = nullptr ,VkBool32 logicOpEnable = VK_FALSE, VkLogicOp logicOp = VK_LOGIC_OP_COPY);
	VkPipelineLayoutCreateInfo pipelineLayout_create_info(uint32_t setLayoutCount,const VkDescriptorSetLayout* pSetLayouts);
	VkGraphicsPipelineCreateInfo graphicsPipeline_create_info(uint32_t stageCount,const VkPipelineShaderStageCreateInfo* pStages,
	const VkPipelineVertexInputStateCreateInfo* pVertexInputState,
	const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
	const VkPipelineViewportStateCreateInfo* pViewportState,
	const VkPipelineRasterizationStateCreateInfo* pRasterizationState,
	const VkPipelineMultisampleStateCreateInfo* pMultisampleState,
	const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState,
	const VkPipelineColorBlendStateCreateInfo* pColorBlendState,
	VkPipelineLayout layout,
	VkRenderPass renderPass,
	uint32_t subpass,
	VkPipeline basePipelineHandle= VK_NULL_HANDLE);
	//******************************************end********************************************************
}