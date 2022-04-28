// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

struct AllocatedBuffer {
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

struct AllocatedImage {
    VkImage _image;
    VmaAllocation _allocation;
};

struct Texture {
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
};

//we will add our main reusable types here