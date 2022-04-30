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

VkSwapchainCreateInfoKHR vkinit::swapchain_create_info(VkSurfaceKHR surface, uint32_t minImageCount, VkFormat imageFormat, VkColorSpaceKHR imageColorSpace, VkExtent2D imageExtent, uint32_t imageArrayLayers, VkImageUsageFlags imageUsage)
{
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = minImageCount;
	swapchainCreateInfo.imageFormat = imageFormat;
	swapchainCreateInfo.imageColorSpace = imageColorSpace;
	swapchainCreateInfo.imageExtent = imageExtent;
	swapchainCreateInfo.imageArrayLayers = imageArrayLayers;
	swapchainCreateInfo.imageUsage = imageUsage;
	return swapchainCreateInfo;
}

VkImageViewCreateInfo vkinit::imageView_create_info(VkImage image, VkFormat format, uint32_t baseMipLevel, VkImageViewType viewType)
{
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	return imageViewCreateInfo;
}

VkAttachmentDescription vkinit::colorAttachment_des(VkFormat format, VkImageLayout finalLayout, VkSampleCountFlagBits samples)
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = format;
	colorAttachment.samples = samples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = finalLayout;
	return colorAttachment;
}

VkAttachmentDescription vkinit::depthAttachment_des(VkFormat format, VkImageLayout finalLayout, VkSampleCountFlagBits samples)
{
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = format;
	depthAttachment.samples = samples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = finalLayout;
	return depthAttachment;
}

VkSubpassDescription vkinit::subpass_des(uint32_t colorAttachmentCount, const VkAttachmentReference* pColorAttachments, const VkAttachmentReference* pDepthStencilAttachment, VkPipelineBindPoint pipelineBindPoint)
{
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = pipelineBindPoint;
	subpass.colorAttachmentCount = colorAttachmentCount;
	subpass.pColorAttachments = pColorAttachments;
	subpass.pDepthStencilAttachment = pDepthStencilAttachment;
	return subpass;
}

VkSubpassDependency vkinit::dependency_des(uint32_t dstSubpass, uint32_t srcSubpass)
{
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = srcSubpass;
	dependency.dstSubpass = dstSubpass;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	return dependency;
}

VkRenderPassCreateInfo vkinit::renderPass_create_info(uint32_t attachmentCount, const VkAttachmentDescription* pAttachments, uint32_t subpassCount, const VkSubpassDescription* pSubpasses, uint32_t dependencyCount, const VkSubpassDependency* pDependencies)
{
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachmentCount;
	renderPassInfo.pAttachments = pAttachments;
	renderPassInfo.subpassCount = subpassCount;
	renderPassInfo.pSubpasses = pSubpasses;
	renderPassInfo.dependencyCount = dependencyCount;
	renderPassInfo.pDependencies = pDependencies;
	return renderPassInfo;
}

VkCommandPoolCreateInfo vkinit::commandPool_create_info(uint32_t queueFamilyIndex)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	return commandPoolCreateInfo;
}

VkImageViewCreateInfo vkinit::view_ceate_info(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t baseMipLevel, VkImageViewType viewType)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectMask;
	viewInfo.subresourceRange.baseMipLevel = baseMipLevel;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	return viewInfo;
}

VkImageCreateInfo vkinit::image_create_info(VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkImageTiling tiling, VkSharingMode sharingMode,VkSampleCountFlagBits samples, uint32_t mipLevels, VkImageType imageType)
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = imageType;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.samples = samples;
	imageCreateInfo.sharingMode = sharingMode;
	return imageCreateInfo;
}

VkMemoryAllocateInfo vkinit::memoryAllocate_info(VkDeviceSize allocationSize, uint32_t memoryTypeIndex)
{
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = allocationSize;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
	return memoryAllocateInfo;
}

VkImageMemoryBarrier vkinit::barrier_des(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	return barrier;
}

VkBufferCreateInfo vkinit::buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = sharingMode;
	return bufferCreateInfo;
}

VkBufferImageCopy vkinit::imageCopy_region(uint32_t width, uint32_t height, uint32_t mipLevel)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};
	return region;
}

VkSamplerCreateInfo vkinit::sampler_create_info(VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW, VkBool32 anisotropyEnable, float maxAnisotropy, VkSamplerMipmapMode mipmapMode, VkFilter magFilter, VkFilter minFilter)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = magFilter;
	samplerInfo.minFilter = minFilter;
	samplerInfo.addressModeU = addressModeU;
	samplerInfo.addressModeV = addressModeV;
	samplerInfo.addressModeW = addressModeW;
	samplerInfo.anisotropyEnable = anisotropyEnable;
	samplerInfo.maxAnisotropy = maxAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = mipmapMode;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	return samplerInfo;
}

VkCommandBufferAllocateInfo vkinit::alloc_info(VkCommandPool commandPool, uint32_t commandBufferCount, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = level;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = commandBufferCount;
	return allocInfo;
}

VkCommandBufferBeginInfo vkinit::cmdbuf_begin_info(VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags;
	return beginInfo;
}

VkSubmitInfo vkinit::submit_info(const VkCommandBuffer* pCommandBuffers, uint32_t commandBufferCount)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = commandBufferCount;
	submitInfo.pCommandBuffers = pCommandBuffers;
	return submitInfo;
}

VkFramebufferCreateInfo vkinit::framebuffer_create_info(VkRenderPass renderPass, uint32_t attachmentCount, const VkImageView* pAttachments, uint32_t width, uint32_t height, uint32_t layers)
{
	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = renderPass;
	framebufferCreateInfo.attachmentCount = attachmentCount;
	framebufferCreateInfo.pAttachments = pAttachments;
	framebufferCreateInfo.width = width;
	framebufferCreateInfo.height = height;
	framebufferCreateInfo.layers = layers;
	return framebufferCreateInfo;
}

VkDescriptorPoolCreateInfo vkinit::descriptorPool_create_info(uint32_t maxSets, uint32_t poolSizeCount, const VkDescriptorPoolSize* pPoolSizes)
{
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = poolSizeCount;
	descriptorPoolCreateInfo.pPoolSizes = pPoolSizes;
	descriptorPoolCreateInfo.maxSets = maxSets;
	return descriptorPoolCreateInfo;
}

VkDescriptorSetLayoutBinding vkinit::descriptorSet_layout_bindings(uint32_t binding, uint32_t descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, const VkSampler* pImmutableSamplers)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
	descriptorSetLayoutBinding.binding = binding;
	descriptorSetLayoutBinding.descriptorCount = descriptorCount;
	descriptorSetLayoutBinding.descriptorType = descriptorType;
	descriptorSetLayoutBinding.pImmutableSamplers = pImmutableSamplers;
	descriptorSetLayoutBinding.stageFlags = stageFlags;
	return descriptorSetLayoutBinding;
}

VkDescriptorSetLayoutCreateInfo vkinit::descriptorSetLayout_create_info(uint32_t bindingCount, const VkDescriptorSetLayoutBinding* pBindings)
{
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
	descriptorSetLayoutCreateInfo.pBindings = pBindings;
	return descriptorSetLayoutCreateInfo;
}

VkDescriptorSetAllocateInfo vkinit::descriptorSet_allocate_info(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSetLayout* pSetLayouts)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
	descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
	return descriptorSetAllocateInfo;
}

VkWriteDescriptorSet vkinit::writeDescriptorSets_info(const void* pNext, VkDescriptorSet dstSet, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorImageInfo* pImageInfo, const VkDescriptorBufferInfo* pBufferInfo, const VkBufferView* pTexelBufferView, uint32_t descriptorCount, uint32_t dstArrayElement)
{
	VkWriteDescriptorSet writeDescriptorSet;
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.pNext = pNext;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.dstBinding = dstBinding;
	writeDescriptorSet.dstArrayElement = dstArrayElement;
	writeDescriptorSet.descriptorCount = descriptorCount;
	writeDescriptorSet.descriptorType = descriptorType;
	writeDescriptorSet.pImageInfo = pImageInfo;
	writeDescriptorSet.pBufferInfo = pBufferInfo;
	writeDescriptorSet.pTexelBufferView = pTexelBufferView;
	return writeDescriptorSet;
}

VkWriteDescriptorSetAccelerationStructureKHR vkinit::descriptorSetAS_info(const VkAccelerationStructureKHR* pAccelerationStructures, uint32_t accelerationStructureCount)
{
	VkWriteDescriptorSetAccelerationStructureKHR descriptorSetAccelerationStructure = {};
	descriptorSetAccelerationStructure.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorSetAccelerationStructure.pNext = NULL;
	descriptorSetAccelerationStructure.accelerationStructureCount = accelerationStructureCount;
	descriptorSetAccelerationStructure.pAccelerationStructures = pAccelerationStructures;
	return descriptorSetAccelerationStructure;
}

VkDescriptorBufferInfo vkinit::buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;
	return bufferInfo;
}

VkDescriptorImageInfo vkinit::image_info(VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = sampler;
	imageInfo.imageView = imageView;
	imageInfo.imageLayout = imageLayout;
	return imageInfo;
}

VkVertexInputBindingDescription vkinit::vertexBinding_des(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
{
	VkVertexInputBindingDescription des{};
	des.binding = binding;
	des.stride = stride;
	des.inputRate = inputRate;
	return des;
}

VkVertexInputAttributeDescription vkinit::vertexAttribute_des(uint32_t location, uint32_t binding, uint32_t offset, VkFormat format)
{
	VkVertexInputAttributeDescription des{};
	des.location = location;
	des.binding = binding;
	des.format = format;
	des.offset = offset;
	return des;
}

VkPipelineVertexInputStateCreateInfo vkinit::vertexInputState_create_info(const VkVertexInputBindingDescription* pVertexBindingDescriptions, const VkVertexInputAttributeDescription* pVertexAttributeDescriptions, uint32_t vertexBindingDescriptionCount, uint32_t vertexAttributeDescriptionCount)
{
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = pVertexBindingDescriptions;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = pVertexAttributeDescriptions;
	return vertexInputStateCreateInfo;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::inputAssembly_create_info(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
{
	VkPipelineInputAssemblyStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info.topology = topology;
	info.primitiveRestartEnable = primitiveRestartEnable;
	return info;
}

VkViewport vkinit::viewport_des(float x, float y, float width, float height, float minDepth, float maxDepth)
{
	VkViewport viewport = {};
	viewport.x =x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

VkRect2D vkinit::scissor(VkOffset2D offset, VkExtent2D extent)
{
	VkRect2D scissor{};
	scissor.offset = offset;
	scissor.extent = extent;
	return scissor;
}

VkPipelineViewportStateCreateInfo vkinit::viewportState_create_info(const VkViewport* pViewports, const VkRect2D* pScissors, uint32_t viewportCount, uint32_t scissorCount)
{
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = viewportCount;
	viewportStateCreateInfo.pViewports = pViewports;
	viewportStateCreateInfo.scissorCount = scissorCount;
	viewportStateCreateInfo.pScissors = pScissors;
	return viewportStateCreateInfo;
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterizationState_create_info(VkBool32 depthBiasEnable, float lineWidth, VkBool32 depthClampEnable, VkCullModeFlags cullMode, VkBool32 rasterizerDiscardEnable, VkFrontFace frontFace, VkPolygonMode polygonMode)
{
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = depthClampEnable;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = rasterizerDiscardEnable;
	rasterizationStateCreateInfo.polygonMode = polygonMode;
	rasterizationStateCreateInfo.lineWidth = lineWidth;
	rasterizationStateCreateInfo.cullMode = cullMode;
	rasterizationStateCreateInfo.frontFace = frontFace;
	rasterizationStateCreateInfo.depthBiasEnable = depthBiasEnable;
	return rasterizationStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo vkinit::multisampleState_create_info(VkSampleCountFlagBits rasterizationSamples, VkBool32 sampleShadingEnable)
{
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = sampleShadingEnable;
	multisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
	return multisampleStateCreateInfo;
}

VkPipelineDepthStencilStateCreateInfo vkinit::depthStencil_create_info(VkBool32 stencilTestEnable, VkBool32 depthBoundsTestEnable, VkBool32 depthWriteEnable, VkBool32 depthTestEnable, VkCompareOp depthCompareOp)
{
	VkPipelineDepthStencilStateCreateInfo info{};
	info.stencilTestEnable = stencilTestEnable;
	info.depthBoundsTestEnable = depthBoundsTestEnable;
	info.depthCompareOp = depthCompareOp;
	info.depthTestEnable = depthTestEnable;
	info.depthWriteEnable = depthWriteEnable;
	return info;
}

VkPipelineColorBlendStateCreateInfo vkinit::colorBlendState_create_info(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* pAttachments, float* blendConstants, VkBool32 logicOpEnable, VkLogicOp logicOp)
{
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = logicOpEnable;
	colorBlendStateCreateInfo.logicOp = logicOp;
	colorBlendStateCreateInfo.attachmentCount = attachmentCount;
	colorBlendStateCreateInfo.pAttachments = pAttachments;
	if (blendConstants != nullptr) {
		colorBlendStateCreateInfo.blendConstants[0] = blendConstants[0];
		colorBlendStateCreateInfo.blendConstants[1] = blendConstants[1];
		colorBlendStateCreateInfo.blendConstants[2] = blendConstants[2];
		colorBlendStateCreateInfo.blendConstants[3] = blendConstants[3];
	}
	else {
		colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
	}
	return colorBlendStateCreateInfo;
}

VkPipelineLayoutCreateInfo vkinit::pipelineLayout_create_info(uint32_t setLayoutCount, const VkDescriptorSetLayout* pSetLayouts)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
	pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
	return pipelineLayoutCreateInfo;
}

VkGraphicsPipelineCreateInfo vkinit::graphicsPipeline_create_info(uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages, const VkPipelineVertexInputStateCreateInfo* pVertexInputState, const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState, const VkPipelineViewportStateCreateInfo* pViewportState, const VkPipelineRasterizationStateCreateInfo* pRasterizationState, const VkPipelineMultisampleStateCreateInfo* pMultisampleState, const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState, const VkPipelineColorBlendStateCreateInfo* pColorBlendState, VkPipelineLayout layout, VkRenderPass renderPass, uint32_t subpass, VkPipeline basePipelineHandle)
{
	VkGraphicsPipelineCreateInfo info{};
	info.stageCount = stageCount;
	info.pStages = pStages;
	info.pVertexInputState = pVertexInputState;
	info.pInputAssemblyState = pInputAssemblyState;
	info.pViewportState = pViewportState;
	info.pRasterizationState = pRasterizationState;
	info.pMultisampleState = pMultisampleState;
	info.pDepthStencilState = pDepthStencilState;
	info.pColorBlendState = pColorBlendState;
	info.layout = layout;
	info.renderPass = renderPass;
	info.subpass = subpass;
	info.basePipelineHandle = basePipelineHandle;
	return info;
}
