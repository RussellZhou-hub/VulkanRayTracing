// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>

struct AllocatedBuffer {
    VkBuffer _buffer;
};

struct AllocatedImage {
    VkImage _image;
};

//we will add our main reusable types here