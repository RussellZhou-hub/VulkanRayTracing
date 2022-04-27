#define _CRT_SECURE_NO_WARNINGS
#define SEEK_END 2
#define VK_ENABLE_BETA_EXTENSIONS
#define GLFW_INCLUDE_VULKAN
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include<vector>
#include<string>
#include<set>
#include<stdexcept>
#include <errno.h>

//#include"App.h"
//#include "tinyobj_loader_c.c"
#include "ray_query.h"
#include "Utils.h"
#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"
#include <iostream>
#define VMA_IMPLEMENTATION
#include<vma/vk_mem_alloc.h>
#include "Shader.h"
#include"vk_initializers.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

VkRayTracingApplication::VkRayTracingApplication():colorAttachCount(6){
    currentFrame = 0;
}

Camera::Camera() {
    position[0] = 0.0f; position[1] = 0.0f; position[2] = 0.0f; position[3] = 1.0f;
    right[0] = 1.0f; right[1] = 0.0f; right[2] = 0.0f; right[3] = 1.0f;
    up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f; up[3] = 1.0f;
    forward[0] = 0.0f; forward[1] = 0.0f; forward[2] = 1.0f; forward[3] = 1.0f;

    //lightA = glm::vec4(2.81,8.28,-1.6,1.0);
    //lightB = glm::vec4(0.81, 8.28, 0.39, 1.0);
    //lightC = glm::vec4(0.81, 8.28, -1.6, 1.0);
    lightPad = glm::vec4(0.0,0.0,0.0,1.0);

    lightA = glm::vec4(1.596529, 7.516576, -1.609730, 1.0f);
    lightB = glm::vec4(-0.403471, 7.516576, 0.390270, 1.0f);
    lightC = glm::vec4(-0.403471, 7.516576, - 1.609730, 1.0f);

    frameCount = 0;
    ViewPortWidth = WIDTH;
    ViewPortHeight = HEIGHT;
}

void ProcessMouseScroll(float yoffset)
{
    cameraPosition[0] += cos(-cameraYaw - (M_PI / 2)) * 0.1f * yoffset;
    cameraPosition[2] += sin(-cameraYaw - (M_PI / 2)) * 0.1f * yoffset;
    //float Zoom = (float)yoffset;
    //if (Zoom < 1.0f)
    //    Zoom = 1.0f;
    //if (Zoom > 45.0f)
     //   Zoom = 45.0f;
    //cameraPosition[1] += Zoom;
    //isCameraMoved = 1;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ProcessMouseScroll(yoffset);
}


bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void VkRayTracingApplication::run(Scene& scene, Camera& camera, ShadingMode& shadingMode)
{
    
    initVulkan(&scene);
    mainLoop(this, &camera,&shadingMode);
    cleanup(this, &scene);
}

void VkRayTracingApplication::initializeScene(Scene* scene, std::string fileName)
{
    //tinyobj_attrib_init(&scene->attributes);
    //tinyobj_parse_obj(&scene->attributes, &scene->shapes, &scene->numShapes, &scene->materials, &scene->numMaterials, fileNameOBJ, &readFile, TINYOBJ_FLAG_TRIANGULATE);
    //loadModel(scene);
    load_meshes(scene, fileName);
}

void VkRayTracingApplication::initVulkan(Scene* scene)
{
    initializeVulkanContext(this);
    pickPhysicalDevice(this);
    createLogicalConnection(this);
    createSwapchain(this);
    createRenderPass(this);
    createCommandPool(this);
    createDepthResources(this);
    
    std::string str = GetExePath();
    str += "\\data\\cube_scene"; 
    //str += "\\data\\CornellBox\\CornellBox-Glossy";
    if (str == "\\data\\cube_scene") isRenderCornellBox = true;
    else isRenderCornellBox = true;
    initializeScene(scene, str);
    //initializeScene(&scene, "D:/Data/Vulkan/Resources/cube_box/cube_scene.obj");

    createTextureImage(this);
    createTextureImageView();
    createTextureSampler();

    createVertexBuffer(this, scene);
    createIndexBuffer(this, scene);
    createMaterialsBuffer(this, scene);
    createTextures(this);

    createFramebuffers(this);

    //Acceleration Structure Setup
    createBottomLevelAccelerationStructure(this, scene);
    createTopLevelAccelerationStructure(this);

    createUniformBuffer(this);
    createDescriptorSets(this);

    createGraphicsPipeline(this);
    createGraphicsPipeline_indirectLgt(this);
    createGraphicsPipeline_indirectLgt_2(this);
    //createCommandBuffers(this,scene);
    //createCommandBuffers_2pass(this, scene);
    createCommandBuffers_3pass(this, scene);

    createSynchronizationObjects(this);
}

void VkRayTracingApplication::mainLoop(VkRayTracingApplication* app, Camera* camera, ShadingMode* shadingMode)
{
    //When compared to a "standard non-raytraced" application, 
    //there are no major differences when creating the synchronization objects or writing the main loop. 
    while (!glfwWindowShouldClose(app->window)) {
        glfwPollEvents();

        //set icon
        std::string str = GetExePath();
        std::string baseIconPath = str + "\\data\\icon";
        str += "\\data\\icon\\my_icon.png";

        GLFWimage images[2];
        images[0].pixels = stbi_load(str.c_str(), &images[0].width, &images[0].height, 0, 4); //rgba channels
        str = baseIconPath + "\\my_icon_small.png";
        images[1].pixels = stbi_load(str.c_str(), &images[0].width, &images[0].height, 0, 4); //rgba channels
        //images[0] = load_icon("my_icon.png");
        //images[1] = load_icon("my_icon_small.png");

        glfwSetWindowIcon(window, 2, images);
        stbi_image_free(images[0].pixels);
        stbi_image_free(images[1].pixels);

        if (glfwGetKey(app->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        isCameraMoved = 0;

        if (keyDownIndex[GLFW_KEY_W]) {
            cameraPosition[1] += 0.1f;
            //cameraPosition[0] += cos(-cameraYaw - (M_PI / 2)) * 0.1f;
            //cameraPosition[2] += sin(-cameraYaw - (M_PI / 2)) * 0.1f;
            isCameraMoved = 1;
        }
        if (keyDownIndex[GLFW_KEY_S]) {
            cameraPosition[1] -= 0.1f;
            //cameraPosition[0] -= cos(-cameraYaw - (M_PI / 2)) * 0.1f;
            //cameraPosition[2] -= sin(-cameraYaw - (M_PI / 2)) * 0.1f;
            isCameraMoved = 1;
        }
        if (keyDownIndex[GLFW_KEY_A]) {
            cameraPosition[0] -= cos(-cameraYaw) * 0.1f;
            cameraPosition[2] -= sin(-cameraYaw) * 0.1f;
            isCameraMoved = 1;
        }
        if (keyDownIndex[GLFW_KEY_D]) {
            cameraPosition[0] += cos(-cameraYaw) * 0.1f;
            cameraPosition[2] += sin(-cameraYaw) * 0.1f;
            isCameraMoved = 1;
        }

        if (keyDownIndex[GLFW_KEY_UP]) {
            camera->lightA.y += 0.1f;
            camera->lightB.y += 0.1f;
            camera->lightC.y += 0.1f;
        }
        if (keyDownIndex[GLFW_KEY_DOWN]) {
            camera->lightA.y -= 0.1f;
            camera->lightB.y -= 0.1f;
            camera->lightC.y -= 0.1f;
        }
        if (keyDownIndex[GLFW_KEY_LEFT]) {
            camera->lightA.x -= cos(-cameraYaw) * 0.1f;
            camera->lightB.x -= cos(-cameraYaw) * 0.1f;
            camera->lightC.x -= cos(-cameraYaw) * 0.1f;
            camera->lightA.z -= sin(-cameraYaw) * 0.1f;
            camera->lightB.z -= sin(-cameraYaw) * 0.1f;
            camera->lightC.z -= sin(-cameraYaw) * 0.1f;
        }
        if (keyDownIndex[GLFW_KEY_RIGHT]) {
            camera->lightA.x += cos(-cameraYaw) * 0.1f;
            camera->lightB.x += cos(-cameraYaw) * 0.1f;
            camera->lightC.x += cos(-cameraYaw) * 0.1f;
            camera->lightA.z += sin(-cameraYaw) * 0.1f;
            camera->lightB.z += sin(-cameraYaw) * 0.1f;
            camera->lightC.z += sin(-cameraYaw) * 0.1f;
        }
        if (keyDownIndex[GLFW_KEY_INSERT]) {
            camera->lightA.x += cos(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightB.x += cos(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightC.x += cos(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightA.z += sin(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightB.z += sin(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightC.z += sin(-cameraYaw - (M_PI / 2)) * 0.1f;
        }
        if (keyDownIndex[GLFW_KEY_DELETE]) {
            camera->lightA.x -= cos(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightB.x -= cos(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightC.x -= cos(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightA.z -= sin(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightB.z -= sin(-cameraYaw - (M_PI / 2)) * 0.1f;
            camera->lightC.z -= sin(-cameraYaw - (M_PI / 2)) * 0.1f;
        }
        if (keyDownIndex[GLFW_KEY_HOME]) {
            camera->lightA *= 1.1f;
            camera->lightB *= 1.1f;
            camera->lightC*= 1.1f;
        }
        if (keyDownIndex[GLFW_KEY_END]) {
            camera->lightA /= 1.1f;
            camera->lightB /= 1.1f;
            camera->lightC /= 1.1f;
        }

        if (keyDownIndex[GLFW_KEY_SPACE]) {
            cameraPosition[1] += 0.1f;
            isCameraMoved = 1;
        }
        if (keyDownIndex[GLFW_KEY_LEFT_CONTROL]) {
            cameraPosition[1] -= 0.1f;
            isCameraMoved = 1;
        }
        if (keyDownIndex[GLFW_KEY_1]) {    //shadow Ray without denoising
            shadingMode->enable2thRay = 0;
            shadingMode->enableShadowMotion = 0;
            shadingMode->enableSVGF = 0;
            shadingMode->enable2thRMotion = 0;
            shadingMode->enableSVGF_withIndAlbedo = 0;
        }
        if (keyDownIndex[GLFW_KEY_2]) {    //2th Ray without denoising
            shadingMode->enable2thRay = 1;
            shadingMode->enableShadowMotion = 0;
            shadingMode->enableSVGF = 0;
            shadingMode->enable2thRMotion = 0;
            shadingMode->enableSVGF_withIndAlbedo = 0;
        }
        if (keyDownIndex[GLFW_KEY_3]) {   //shadow method:motion vector along
            shadingMode->enable2thRay = 1;
            shadingMode->enableShadowMotion = 1;
            shadingMode->enableSVGF = 0;
            shadingMode->enable2thRMotion = 1;
            shadingMode->enableSVGF_withIndAlbedo = 0;
        }
        if (keyDownIndex[GLFW_KEY_4]) {   //enable SVGF
            shadingMode->enable2thRay = 1;
            shadingMode->enableShadowMotion = 0;
            shadingMode->enableSVGF = 1;
            shadingMode->enable2thRMotion = 0;
            shadingMode->enableSVGF_withIndAlbedo = 0;
        }
        if (keyDownIndex[GLFW_KEY_5]) {     //my method
            shadingMode->enable2thRay = 1;
            shadingMode->enableShadowMotion = 0;
            shadingMode->enableSVGF = 1;
            shadingMode->enable2thRMotion = 0;
            shadingMode->enableSVGF_withIndAlbedo = 1;
        }

        static double previousMousePositionX;
        static double previousMousePositionY;

        double xPos, yPos;
        glfwGetCursorPos(app->window, &xPos, &yPos);
        glfwSetScrollCallback(app->window, scroll_callback);

        if (previousMousePositionX != xPos || previousMousePositionY != yPos) {
            double mouseDifferenceX = previousMousePositionX - xPos;
            double mouseDifferenceY = previousMousePositionY - yPos;

            cameraYaw += mouseDifferenceX * 0.0005f;

            previousMousePositionX = xPos;
            previousMousePositionY = yPos;

            isCameraMoved = 1;
        }

        camera->position[0] = cameraPosition[0]; camera->position[1] = cameraPosition[1]; camera->position[2] = cameraPosition[2];

        camera->forward[0] = cosf(cameraPitch) * cosf(-cameraYaw - (M_PI / 2.0));
        camera->forward[1] = sinf(cameraPitch);
        camera->forward[2] = cosf(cameraPitch) * sinf(-cameraYaw - (M_PI / 2.0));

        camera->right[0] = camera->forward[1] * camera->up[2] - camera->forward[2] * camera->up[1];
        camera->right[1] = camera->forward[2] * camera->up[0] - camera->forward[0] * camera->up[2];
        camera->right[2] = camera->forward[0] * camera->up[1] - camera->forward[1] * camera->up[0];

        if (isCameraMoved == 1) {
            camera->frameCount = 0;
        }
        else {
            camera->frameCount += 1;
        }

        //cout << "cameraPos: " << camera->position[0] << " " << camera->position[1] << " " << camera->position[2] << "\n";
        //cout << "cameraPitch: " << cameraPitch << " cameraYaw: " << cameraYaw << "\n";

        drawFrame(app, camera, shadingMode);
    }

    vkDeviceWaitIdle(app->logicalDevice);
}

void VkRayTracingApplication::cleanup(VkRayTracingApplication* app, Scene* scene)
{
    PFN_vkDestroyAccelerationStructureKHR pvkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkDestroyAccelerationStructureKHR");
    PFN_vkDestroyDebugUtilsMessengerEXT pvkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(app->instance, "vkDestroyDebugUtilsMessengerEXT");

    for (size_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
        vkDestroySemaphore(app->logicalDevice, app->renderFinishedSemaphores[x], NULL);
        vkDestroySemaphore(app->logicalDevice, app->imageAvailableSemaphores[x], NULL);
        vkDestroyFence(app->logicalDevice, app->inFlightFences[x], NULL);
    }

    free(app->imageAvailableSemaphores);
    free(app->renderFinishedSemaphores);
    free(app->inFlightFences);
    free(app->imagesInFlight);

    vkFreeCommandBuffers(app->logicalDevice, app->commandPool, app->imageCount, app->commandBuffers);
    free(app->commandBuffers);

    vkDestroyPipeline(app->logicalDevice, app->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(app->logicalDevice, app->pipelineLayout, NULL);

    free(app->vertexBindingDescriptions);
    free(app->vertexAttributeDescriptions);

    vkDestroyDescriptorSetLayout(app->logicalDevice, app->rayTraceDescriptorSetLayouts[1], NULL);
    vkDestroyDescriptorSetLayout(app->logicalDevice, app->rayTraceDescriptorSetLayouts[0], NULL);
    free(app->rayTraceDescriptorSetLayouts);
    vkDestroyDescriptorPool(app->logicalDevice, app->descriptorPool, NULL);

    vkDestroyBuffer(app->logicalDevice, app->uniformBuffer, NULL);
    vkFreeMemory(app->logicalDevice, app->uniformBufferMemory, NULL);

    pvkDestroyAccelerationStructureKHR(app->logicalDevice, app->topLevelAccelerationStructure, NULL);
    vkDestroyBuffer(app->logicalDevice, app->topLevelAccelerationStructureBuffer, NULL);
    vkFreeMemory(app->logicalDevice, app->topLevelAccelerationStructureBufferMemory, NULL);

    pvkDestroyAccelerationStructureKHR(app->logicalDevice, app->accelerationStructure, NULL);
    vkDestroyBuffer(app->logicalDevice, app->accelerationStructureBuffer, NULL);
    vkFreeMemory(app->logicalDevice, app->accelerationStructureBufferMemory, NULL);

    vkDestroyImageView(app->logicalDevice, app->rayTraceImageView, NULL);
    vkFreeMemory(app->logicalDevice, app->rayTraceImageMemory, NULL);
    vkDestroyImage(app->logicalDevice, app->rayTraceImage, NULL);

    vkDestroyBuffer(app->logicalDevice, app->materialBuffer, NULL);
    vkFreeMemory(app->logicalDevice, app->materialBufferMemory, NULL);

    vkDestroyBuffer(app->logicalDevice, app->materialIndexBuffer, NULL);
    vkFreeMemory(app->logicalDevice, app->materialIndexBufferMemory, NULL);

    vkDestroyBuffer(app->logicalDevice, app->indexBuffer, NULL);
    vkFreeMemory(app->logicalDevice, app->indexBufferMemory, NULL);

    vkDestroyBuffer(app->logicalDevice, app->vertexPositionBuffer, NULL);
    vkFreeMemory(app->logicalDevice, app->vertexPositionBufferMemory, NULL);

    for (int x = 0; x < app->imageCount; x++) {
        vkDestroyFramebuffer(app->logicalDevice, app->swapchainFramebuffers[x], NULL);
    }
    free(app->swapchainFramebuffers);

    vkDestroyImageView(app->logicalDevice, app->depthImageView, NULL);
    vkFreeMemory(app->logicalDevice, app->depthImageMemory, NULL);
    vkDestroyImage(app->logicalDevice, app->depthImage, NULL);

    vkDestroyCommandPool(app->logicalDevice, app->commandPool, NULL);

    vkDestroyRenderPass(app->logicalDevice, app->renderPass, NULL);
    vkDestroyRenderPass(app->logicalDevice, app->renderPass_indierctLgt, NULL);
    

    for (int x = 0; x < app->imageCount; x++) {
        vkDestroyImageView(app->logicalDevice, app->swapchainImageViews[x], NULL);
    }
    free(app->swapchainImageViews);

    free(app->swapchainImages);
    vkDestroySwapchainKHR(app->logicalDevice, app->swapchain, NULL);

    vkDestroyImage(logicalDevice, textureImage, nullptr);
    vkFreeMemory(logicalDevice, textureImageMemory, nullptr);
    vkDestroyImageView(logicalDevice, textureImageView, nullptr);
    vkDestroySampler(logicalDevice, textureSampler, nullptr);


    vkDestroyDevice(app->logicalDevice, NULL);

    if (ENABLE_VALIDATION) {
        pvkDestroyDebugUtilsMessengerEXT(app->instance, app->debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
    vkDestroyInstance(app->instance, NULL);
    glfwDestroyWindow(app->window);
    glfwTerminate();

    //tinyobj_attrib_free(&scene->attributes);
    //tinyobj_shapes_free(scene->shapes, scene->numShapes);
    //tinyobj_materials_free(scene->materials, scene->numMaterials);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        keyDownIndex[key] = 1;
    }
    if (action == GLFW_RELEASE) {
        keyDownIndex[key] = 0;
    }
}

void readFile(const char* fileName, char** buffer, uint64_t* length)
{
    uint64_t stringSize = 0;
    uint64_t readSize = 0;
    FILE* handler;

    handler = fopen(fileName, "r");
    if (handler == NULL) { fputs("File error", stderr); exit(1); }

    if (handler) {
        fseek(handler, 0, SEEK_END);
        stringSize = ftell(handler);
        rewind(handler);
        *buffer = (char*)malloc(sizeof(char) * (stringSize + 1));
        //(*buffer)[stringSize] = '\0';
        readSize = fread(*buffer, sizeof(char), (size_t)stringSize, handler);
        if (stringSize != readSize) {
            //*buffer = NULL;
            //free(*buffer);
        }
        fclose(handler);
    }

    *length = readSize;
}

void VkRayTracingApplication::createBuffer(VkRayTracingApplication* app, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(app->logicalDevice, &bufferCreateInfo, NULL, buffer) == VK_SUCCESS) {
        printf("created buffer with size %ld\n", size);
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(app->logicalDevice, *buffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;

    uint32_t memoryTypeIndex = -1;
    for (int x = 0; x < app->memoryProperties.memoryTypeCount; x++) {
        if ((memoryRequirements.memoryTypeBits & (1 << x)) && (app->memoryProperties.memoryTypes[x].propertyFlags & propertyFlags) == propertyFlags) {
            memoryTypeIndex = x;
            break;
        }
    }
    memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(app->logicalDevice, &memoryAllocateInfo, NULL, bufferMemory) == VK_SUCCESS) {
        printf("allocated buffer memory\n");
    }

    vkBindBufferMemory(app->logicalDevice, *buffer, *bufferMemory, 0);
}

void VkRayTracingApplication::copyBuffer(VkRayTracingApplication* app, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo bufferAllocateInfo = {};
    bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocateInfo.commandPool = app->commandPool;
    bufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(app->logicalDevice, &bufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    VkBufferCopy bufferCopy = {};
    bufferCopy.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo_copyBuffer = {};
    submitInfo_copyBuffer.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo_copyBuffer.commandBufferCount = 1;
    submitInfo_copyBuffer.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(app->graphicsQueue, 1, &submitInfo_copyBuffer, VK_NULL_HANDLE);
    vkQueueWaitIdle(app->graphicsQueue);

    vkFreeCommandBuffers(app->logicalDevice, app->commandPool, 1, &commandBuffer);
}

void VkRayTracingApplication::createImage(VkRayTracingApplication* app, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage* image, VkDeviceMemory* imageMemory)
{
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usageFlags;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(app->logicalDevice, &imageCreateInfo, NULL, image) == VK_SUCCESS) {
        printf("created image\n");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(app->logicalDevice, *image, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;

    uint32_t memoryTypeIndex = -1;
    for (int x = 0; x < app->memoryProperties.memoryTypeCount; x++) {
        if ((memoryRequirements.memoryTypeBits & (1 << x)) && (app->memoryProperties.memoryTypes[x].propertyFlags & propertyFlags) == propertyFlags) {
            memoryTypeIndex = x;
            break;
        }
    }
    memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(app->logicalDevice, &memoryAllocateInfo, NULL, imageMemory) != VK_SUCCESS) {
        printf("allocated image memory\n");
    }

    vkBindImageMemory(app->logicalDevice, *image, *imageMemory, 0);
}

void VkRayTracingApplication::initializeVulkanContext(VkRayTracingApplication* app)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    app->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan RayTracing", NULL, NULL);

    glfwSetInputMode(app->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(app->window, keyCallback);

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    uint32_t extensionCount = glfwExtensionCount + 2;
    const char** extensionNames = (const char**)malloc(sizeof(const char*) * extensionCount);
    memcpy(extensionNames, glfwExtensionNames, sizeof(const char*) * glfwExtensionCount);
    extensionNames[glfwExtensionCount] = "VK_KHR_get_physical_device_properties2";
    extensionNames[glfwExtensionCount + 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    VkApplicationInfo applicationInfo = vkinit::application_info("vulkan_ray_tracing");
    VkInstanceCreateInfo instanceCreateInfo = vkinit::instance_create_info(&applicationInfo, extensionCount, extensionNames);

    const std::vector<VkValidationFeatureEnableEXT> enabledValidationLayers = {
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
    };

    // debugPrintf
    VkValidationFeaturesEXT validationFeatures=vkinit::validation_Features_info(enabledValidationLayers.size(), enabledValidationLayers.data());

    instanceCreateInfo.pNext = &validationFeatures;;

    if (ENABLE_VALIDATION) {
        uint32_t layerCount = 1;
        const char** layerNames = (const char**)malloc(sizeof(const char*) * layerCount);
        layerNames[0] = "VK_LAYER_KHRONOS_validation";

        VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
        messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT| VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
        messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messengerCreateInfo.pfnUserCallback = debugCallback;

        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&messengerCreateInfo;
        instanceCreateInfo.enabledLayerCount = layerCount;
        instanceCreateInfo.ppEnabledLayerNames = layerNames;


        if (vkCreateInstance(&instanceCreateInfo, NULL, &app->instance) == VK_SUCCESS) {
            printf("created Vulkan instance\n");
        }

        PFN_vkCreateDebugUtilsMessengerEXT pvkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(app->instance, "vkCreateDebugUtilsMessengerEXT");
        if (pvkCreateDebugUtilsMessengerEXT(app->instance, &messengerCreateInfo, NULL, &app->debugMessenger) == VK_SUCCESS) {
            printf("created debug messenger\n");
        }

        free(layerNames);
    }
    else {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.pNext = NULL;

        if (vkCreateInstance(&instanceCreateInfo, NULL, &app->instance) == VK_SUCCESS) {
            printf("created Vulkan instance\n");
        }
    }

    if (glfwCreateWindowSurface(app->instance, app->window, NULL, &app->surface) == VK_SUCCESS) {
        printf("created window surface\n");
    }

    free(extensionNames);
}

bool VkRayTracingApplication::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    //debug
    if (!requiredExtensions.empty()) {
        auto i = 1;
        for (const auto& extension : availableExtensions) {
            std::cout << "suport extension" << i << ": " << extension.extensionName << "\n";
        }
    }

    return requiredExtensions.empty();
}

bool VkRayTracingApplication::isDeviceSuitable(const VkPhysicalDevice device)
{
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    return extensionsSupported;
}

void VkRayTracingApplication::pickPhysicalDevice(VkRayTracingApplication* app)
{
    //The code assumes that the only (or first) available device is compatible with the VK_KHR_ray_tracing extension.
      //If you are using multiple devices, 
      //you might need to refactor the code to select the device that is compatible with the extension.
    app->physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    //VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(app->instance, &deviceCount, devices.data());

    

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            app->physicalDevice = device;
            break;
        }
    }

    if (!app->physicalDevice) {
        exit(-1);
        system("pause");
    }

    //app->physicalDevice = devices[0];

    if (app->physicalDevice != VK_NULL_HANDLE) {
        printf("picked physical device\n");
    }

    vkGetPhysicalDeviceMemoryProperties(app->physicalDevice, &app->memoryProperties);

    devices.clear();
}

void VkRayTracingApplication::createLogicalConnection(VkRayTracingApplication* app)
{
    app->graphicsQueueIndex = -1;
    app->presentQueueIndex = -1;
    app->computeQueueIndex = -1;

    //VkPhysicalDeviceProperties2 properties = {};
    //properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    //vkGetPhysicalDeviceProperties2(app->physicalDevice, &properties);


    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(app->physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilyProperties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(app->physicalDevice, &queueFamilyCount, queueFamilyProperties);

    for (int x = 0; x < queueFamilyCount; x++) {
        if (app->graphicsQueueIndex == -1 && queueFamilyProperties[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            app->graphicsQueueIndex = x;
        }

        if (app->computeQueueIndex == -1 && queueFamilyProperties[x].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            app->computeQueueIndex = x;
        }

        VkBool32 isPresentSupported = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(app->physicalDevice, x, app->surface, &isPresentSupported);

        if (app->presentQueueIndex == -1 && isPresentSupported) {
            app->presentQueueIndex = x;
        }

        if (app->graphicsQueueIndex != -1 && app->presentQueueIndex != -1 && app->computeQueueIndex != -1) {
            break;
        }
    }

    enabledDeviceExtensions = vkinit::enableExt_names();

    uint32_t deviceEnabledExtensionCount = enabledDeviceExtensions.size();
    const char** deviceEnabledExtensionNames = (const char**)malloc(sizeof(const char*) * deviceEnabledExtensionCount);
    deviceEnabledExtensionNames[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceEnabledExtensionNames[1] = "VK_KHR_ray_query";
    deviceEnabledExtensionNames[2] = "VK_KHR_acceleration_structure";
    deviceEnabledExtensionNames[3] = "VK_KHR_spirv_1_4";
    deviceEnabledExtensionNames[4] = "VK_KHR_shader_float_controls";
    deviceEnabledExtensionNames[5] = "VK_KHR_get_memory_requirements2";
    deviceEnabledExtensionNames[6] = "VK_EXT_descriptor_indexing";
    deviceEnabledExtensionNames[7] = "VK_KHR_buffer_device_address";
    deviceEnabledExtensionNames[8] = "VK_KHR_deferred_host_operations";
    deviceEnabledExtensionNames[9] = "VK_KHR_pipeline_library";
    deviceEnabledExtensionNames[10] = "VK_KHR_maintenance3";
    deviceEnabledExtensionNames[11] = "VK_KHR_maintenance1";
    deviceEnabledExtensionNames[12] = "VK_KHR_shader_non_semantic_info";

    float queuePriority = 1.0f;
    uint32_t deviceQueueCreateInfoCount = 3;
    VkDeviceQueueCreateInfo* deviceQueueCreateInfos = (VkDeviceQueueCreateInfo*)malloc(sizeof(VkDeviceQueueCreateInfo) * deviceQueueCreateInfoCount);

    deviceQueueCreateInfos[0] = vkinit::device_Queue_create_info(app->graphicsQueueIndex, &queuePriority);
    deviceQueueCreateInfos[1] = vkinit::device_Queue_create_info(app->presentQueueIndex, &queuePriority);
    deviceQueueCreateInfos[2] = vkinit::device_Queue_create_info(app->computeQueueIndex, &queuePriority);

    VkPhysicalDeviceBufferDeviceAddressFeaturesEXT bufferDeviceAddressFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT,
      .pNext = NULL,
      .bufferDeviceAddress = VK_TRUE,
      .bufferDeviceAddressCaptureReplay = VK_FALSE,
      .bufferDeviceAddressMultiDevice = VK_FALSE
    };

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
      .pNext = &bufferDeviceAddressFeatures,
      .rayQuery = VK_TRUE
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
      .pNext = &rayQueryFeatures,
      .accelerationStructure = VK_TRUE,
      .accelerationStructureCaptureReplay = VK_TRUE,
      .accelerationStructureIndirectBuild = VK_FALSE,
      .accelerationStructureHostCommands = VK_FALSE,
      .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE
    };

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.geometryShader = VK_TRUE;
    deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = vkinit::device_create_info(&accelerationStructureFeatures, deviceQueueCreateInfoCount, deviceQueueCreateInfos, (uint32_t)enabledDeviceExtensions.size(),deviceEnabledExtensionNames, &deviceFeatures);

    if (vkCreateDevice(app->physicalDevice, &deviceCreateInfo, NULL, &app->logicalDevice) == VK_SUCCESS) {
        printf("created logical connection to device\n");
    }

    vkGetDeviceQueue(app->logicalDevice, app->graphicsQueueIndex, 0, &app->graphicsQueue);
    vkGetDeviceQueue(app->logicalDevice, app->presentQueueIndex, 0, &app->presentQueue);
    vkGetDeviceQueue(app->logicalDevice, app->computeQueueIndex, 0, &app->computeQueue);

    free(deviceEnabledExtensionNames);
    free(queueFamilyProperties);
    free(deviceQueueCreateInfos);

    //initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = app->physicalDevice;
    allocatorInfo.device = app->logicalDevice;
    allocatorInfo.instance = app->instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

void VkRayTracingApplication::createSwapchain(VkRayTracingApplication* app)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->physicalDevice, app->surface, &surfaceCapabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(app->physicalDevice, app->surface, &formatCount, NULL);
    VkSurfaceFormatKHR* surfaceFormats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(app->physicalDevice, app->surface, &formatCount, surfaceFormats);

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(app->physicalDevice, app->surface, &presentModeCount, NULL);
    VkPresentModeKHR* surfacePresentModes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(app->physicalDevice, app->surface, &presentModeCount, surfacePresentModes);

    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
    VkPresentModeKHR presentMode = surfacePresentModes[0];
    VkExtent2D extent = surfaceCapabilities.currentExtent;

    app->imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && app->imageCount > surfaceCapabilities.maxImageCount) {
        app->imageCount = surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = app->surface;
    swapchainCreateInfo.minImageCount = app->imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    if (app->graphicsQueueIndex != app->presentQueueIndex) {
        uint32_t queueFamilyIndices[2] = { app->graphicsQueueIndex, app->presentQueueIndex };

        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(app->logicalDevice, &swapchainCreateInfo, NULL, &app->swapchain) == VK_SUCCESS) {
        printf("created swapchain\n");
    }

    vkGetSwapchainImagesKHR(app->logicalDevice, app->swapchain, &app->imageCount, NULL);
    app->swapchainImages = (VkImage*)malloc(sizeof(VkImage) * app->imageCount);
    vkGetSwapchainImagesKHR(app->logicalDevice, app->swapchain, &app->imageCount, app->swapchainImages);

    app->swapchainImageFormat = surfaceFormat.format;
    app->swapchainExtent = extent;

    app->swapchainImageViews = (VkImageView*)malloc(sizeof(VkImageView) * app->imageCount);

    for (int x = 0; x < app->imageCount; x++) {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = app->swapchainImages[x];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = app->swapchainImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo, NULL, &app->swapchainImageViews[x]) == VK_SUCCESS) {
            printf("created image view #%d\n", x);
        }
    }

    free(surfaceFormats);
    free(surfacePresentModes);
}

void VkRayTracingApplication::createRenderPass(VkRayTracingApplication* app)
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = app->swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription colorAttachment_2 = {};
    colorAttachment_2.format = app->swapchainImageFormat;
    colorAttachment_2.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment_2.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment_2.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment_2.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment_2.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment_2.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment_2.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference* colorAttachmentRef = (VkAttachmentReference*)malloc(app->colorAttachCount*sizeof(VkAttachmentReference));
    for(auto i=0;i< app->colorAttachCount;i++){
        colorAttachmentRef[i].attachment = i;  //location in fragment shader
        colorAttachmentRef[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    };

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = app->colorAttachCount;  //location in fragment shader
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = app->colorAttachCount;
    subpass.pColorAttachments = colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[7] = { colorAttachment,colorAttachment_2,colorAttachment_2,colorAttachment_2,colorAttachment_2,colorAttachment_2, depthAttachment };

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = app->colorAttachCount+1;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;


    if (vkCreateRenderPass(app->logicalDevice, &renderPassInfo, NULL, &app->renderPass) == VK_SUCCESS) {
        printf("created render pass\n");
    }
    if (vkCreateRenderPass(app->logicalDevice, &renderPassInfo, NULL, &app->renderPass_indierctLgt) == VK_SUCCESS) {
        printf("created renderPass_indierctLgt pass\n");
    }
    if (vkCreateRenderPass(app->logicalDevice, &renderPassInfo, NULL, &app->renderPass_indierctLgt_2) == VK_SUCCESS) {
        printf("created renderPass_indierctLgt_2 pass\n");
    }
}

void VkRayTracingApplication::createCommandPool(VkRayTracingApplication* app)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = app->graphicsQueueIndex;

    if (vkCreateCommandPool(app->logicalDevice, &commandPoolCreateInfo, NULL, &app->commandPool) == VK_SUCCESS) {
        printf("created command pool\n");
    }
}

void VkRayTracingApplication::createVertexBuffer(VkRayTracingApplication* app, Scene* scene)
{
    VkDeviceSize positionBufferSize;
    if(isRenderCornellBox) positionBufferSize = sizeof(float) * scene->attributes.num_vertices * 3;
    else {
        positionBufferSize = vertices.size() * sizeof(Vertex);  //c++ version tinyobjloader
    }

    VkBuffer positionStagingBuffer;
    VkDeviceMemory positionStagingBufferMemory;
    createBuffer(app, positionBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &positionStagingBuffer, &positionStagingBufferMemory);

    void* positionData;
    vkMapMemory(app->logicalDevice, positionStagingBufferMemory, 0, positionBufferSize, 0, &positionData);
    if (isRenderCornellBox) memcpy(positionData, scene->attributes.vertices, positionBufferSize);
    else {
        memcpy(positionData, vertices.data(), positionBufferSize);
    }
    vkUnmapMemory(app->logicalDevice, positionStagingBufferMemory);

    createBuffer(app, positionBufferSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR|VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->vertexPositionBuffer, &app->vertexPositionBufferMemory);

    copyBuffer(app, positionStagingBuffer, app->vertexPositionBuffer, positionBufferSize);

    vkDestroyBuffer(app->logicalDevice, positionStagingBuffer, NULL);
    vkFreeMemory(app->logicalDevice, positionStagingBufferMemory, NULL);
}

void VkRayTracingApplication::createIndexBuffer(VkRayTracingApplication* app, Scene* scene)
{
    VkDeviceSize bufferSize = sizeof(uint32_t) * scene->attributes.num_faces;

    uint32_t* positionIndices = (uint32_t*)malloc(bufferSize);
    for (int x = 0; x < scene->attributes.num_faces; x++) {
        positionIndices[x] = scene->attributes.faces[x].v_idx;
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(app, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    void* data;
    vkMapMemory(app->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, positionIndices, bufferSize);
    vkUnmapMemory(app->logicalDevice, stagingBufferMemory);

    createBuffer(app, bufferSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR|VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->indexBuffer, &app->indexBufferMemory);

    copyBuffer(app, stagingBuffer, app->indexBuffer, bufferSize);

    vkDestroyBuffer(app->logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(app->logicalDevice, stagingBufferMemory, NULL);

    free(positionIndices);
}

void VkRayTracingApplication::createMaterialsBuffer(VkRayTracingApplication* app, Scene* scene)
{
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * scene->attributes.num_face_num_verts;  //总三角形面数

    VkBuffer indexStagingBuffer;
    VkDeviceMemory indexStagingBufferMemory;
    createBuffer(app, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexStagingBuffer, &indexStagingBufferMemory);

    //int index = scene->attributes.material_ids[40];

    void* indexData;
    vkMapMemory(app->logicalDevice, indexStagingBufferMemory, 0, indexBufferSize, 0, &indexData);
    memcpy(indexData, scene->attributes.material_ids, indexBufferSize);
    vkUnmapMemory(app->logicalDevice, indexStagingBufferMemory);

    createBuffer(app, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->materialIndexBuffer, &app->materialIndexBufferMemory);

    copyBuffer(app, indexStagingBuffer, app->materialIndexBuffer, indexBufferSize);

    vkDestroyBuffer(app->logicalDevice, indexStagingBuffer, NULL);
    vkFreeMemory(app->logicalDevice, indexStagingBufferMemory, NULL);

    VkDeviceSize materialBufferSize = sizeof(struct Material) * scene->numMaterials;

    
    struct Material* materials = (struct Material*)malloc(materialBufferSize);
    for (int x = 0; x < scene->numMaterials; x++) {
        memcpy(materials[x].ambient, scene->materials[x].ambient, sizeof(float) * 3);
        memcpy(materials[x].diffuse, scene->materials[x].diffuse, sizeof(float) * 3);
        memcpy(materials[x].specular, scene->materials[x].specular, sizeof(float) * 3);
        memcpy(materials[x].emission, scene->materials[x].emission, sizeof(float) * 3);
    }

    

    VkBuffer materialStagingBuffer;
    VkDeviceMemory materialStagingBufferMemory;
    createBuffer(app, materialBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &materialStagingBuffer, &materialStagingBufferMemory);

    //修改灯光颜色
    //materials[6].emission[0] = 1.0f;
    //materials[6].emission[1] = 0.8f;
    //materials[6].emission[2] = 0.7f;
    //Material mat = materials[6];

    void* materialData;
    vkMapMemory(app->logicalDevice, materialStagingBufferMemory, 0, materialBufferSize, 0, &materialData);
    memcpy(materialData, materials, materialBufferSize);
    vkUnmapMemory(app->logicalDevice, materialStagingBufferMemory);

    createBuffer(app, materialBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->materialBuffer, &app->materialBufferMemory);

    copyBuffer(app, materialStagingBuffer, app->materialBuffer, materialBufferSize);

    vkDestroyBuffer(app->logicalDevice, materialStagingBuffer, NULL);
    vkFreeMemory(app->logicalDevice, materialStagingBufferMemory, NULL);

    free(materials);
}

void VkRayTracingApplication::createTextures(VkRayTracingApplication* app)
{
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->rayTraceImage, &app->rayTraceImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->Image_indirectLgt, &app->ImageMemory_indirectLgt);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->Image_indirectLgt_2, &app->ImageMemory_indirectLgt_2);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->HDirectIrradImage, &app->HDirectIrradImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->HDirectAlbedoImage, &app->HDirectAlbedoImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->HNormalImage, &app->HNormalImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->HWorldPosImage, &app->HWorldPosImageMemory);
    createImage(app, WIDTH, HEIGHT, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->HDepthImage, &app->HDepthImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->HVarImage, &app->HVarImageMemory);
    
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->GnormalImage, &app->GnormalImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->GDirectImage, &app->GDirectImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->GDirectIrradImage, &app->GDirectIrradImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->G_albedoImage, &app->G_albedoImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->GIrradImage, &app->GIrradImageMemory);
    createImage(app, WIDTH, HEIGHT, app->swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->GWorldPosImage, &app->GWorldPosImageMemory);


    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = NULL;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = app->swapchainImageFormat;
    imageViewCreateInfo.subresourceRange = subresourceRange;
    imageViewCreateInfo.image = app->rayTraceImage;
    
    VkImageViewCreateInfo G_imageViewCreateInfo_2 = imageViewCreateInfo;
    G_imageViewCreateInfo_2.image = app->GnormalImage;

    VkImageViewCreateInfo G_imageViewCreateInfo_3 = imageViewCreateInfo;
    G_imageViewCreateInfo_3.image = app->GDirectImage;

    VkImageViewCreateInfo G_imageViewCreateInfo_4 = imageViewCreateInfo;
    G_imageViewCreateInfo_4.image = app->GDirectIrradImage;

    VkImageViewCreateInfo G_imageViewCreateInfo_5 = imageViewCreateInfo;
    G_imageViewCreateInfo_5.image = app->G_albedoImage;

    VkImageViewCreateInfo G_imageViewCreateInfo_6 = imageViewCreateInfo;
    G_imageViewCreateInfo_6.image = app->GIrradImage;

    VkImageViewCreateInfo G_imageViewCreateInfo_7 = imageViewCreateInfo;
    G_imageViewCreateInfo_7.image = app->GWorldPosImage;

    VkImageViewCreateInfo imageViewCreateInfo_2 = imageViewCreateInfo;
    imageViewCreateInfo_2.image = app->Image_indirectLgt;

    VkImageViewCreateInfo imageViewCreateInfo_3 = imageViewCreateInfo;
    imageViewCreateInfo_3.image = app->Image_indirectLgt_2;

    VkImageViewCreateInfo imageViewCreateInfo_DirectIrrad = imageViewCreateInfo;
    imageViewCreateInfo_DirectIrrad.image = app->HDirectIrradImage;

    VkImageViewCreateInfo imageViewCreateInfo_DirectAlbedo = imageViewCreateInfo;
    imageViewCreateInfo_DirectAlbedo.image = app->HDirectAlbedoImage;

    VkImageViewCreateInfo imageViewCreateInfo_Normal = imageViewCreateInfo;
    imageViewCreateInfo_Normal.image = app->HNormalImage;

    VkImageViewCreateInfo imageViewCreateInfo_WorldPos = imageViewCreateInfo;
    imageViewCreateInfo_WorldPos.image = app->HWorldPosImage;

    VkImageViewCreateInfo imageViewCreateInfo_Depth = imageViewCreateInfo;
    imageViewCreateInfo_Depth.image = app->HDepthImage;
    imageViewCreateInfo_Depth.format = VK_FORMAT_D32_SFLOAT;
    imageViewCreateInfo_Depth.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkImageViewCreateInfo imageViewCreateInfo_Var = imageViewCreateInfo;
    imageViewCreateInfo_Var.image = app->HVarImage;

    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo, NULL, &app->rayTraceImageView) == VK_SUCCESS) {
        printf("created GnormalImageView \n");
    }
    if (vkCreateImageView(app->logicalDevice, &G_imageViewCreateInfo_2, NULL, &app->GnormalImageView) == VK_SUCCESS) {
        printf("created GnormalImageView \n");
    }
    if (vkCreateImageView(app->logicalDevice, &G_imageViewCreateInfo_3, NULL, &app->GDirectImageView) == VK_SUCCESS) {
        printf("created GDirectImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &G_imageViewCreateInfo_4, NULL, &app->GDirectIrradImageView) == VK_SUCCESS) {
        printf("created GDepthImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &G_imageViewCreateInfo_5, NULL, &app->G_albedoImageView) == VK_SUCCESS) {
        printf("created GDepthImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &G_imageViewCreateInfo_6, NULL, &app->GIrradImageView) == VK_SUCCESS) {
        printf("created GDepthImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &G_imageViewCreateInfo_7, NULL, &app->GWorldPosImageView) == VK_SUCCESS) {
        printf("created GDepthImageView\n");
    }

    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_2, NULL, &app->ImageView_indirectLgt) == VK_SUCCESS) {
        printf("created image_indirectLgt view\n");
    }
    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_3, NULL, &app->ImageView_indirectLgt_2) == VK_SUCCESS) {
        printf("created image_indirectLgt_2 view\n");
    }

    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_3, NULL, &app->ImageView_indirectLgt_2) == VK_SUCCESS) {
        printf("created image_indirectLgt_2 view\n");
    }
    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_DirectIrrad, NULL, &app->HDirectIrradImageView) == VK_SUCCESS) {
        printf("created HDirectIrradImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_DirectAlbedo, NULL, &app->HDirectAlbedoImageView) == VK_SUCCESS) {
        printf("created HDirectAlbedoImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_Normal, NULL, &app->HNormalImageView) == VK_SUCCESS) {
        printf("created HDirectAlbedoImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_WorldPos, NULL, &app->HWorldPosImageView) == VK_SUCCESS) {
        printf("created HDirectAlbedoImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_Depth, NULL, &app->HDepthImageView) == VK_SUCCESS) {
        printf("created HDepthImageView\n");
    }
    if (vkCreateImageView(app->logicalDevice, &imageViewCreateInfo_Var, NULL, &app->HVarImageView) == VK_SUCCESS) {
        printf("created HVarImageView\n");
    }

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = NULL;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.image = app->rayTraceImage;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = 0;

    VkCommandBufferAllocateInfo bufferAllocateInfo = {};
    bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocateInfo.commandPool = app->commandPool;
    bufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(app->logicalDevice, &bufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(app->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(app->computeQueue);

    vkFreeCommandBuffers(app->logicalDevice, app->commandPool, 1, &commandBuffer);
}

void VkRayTracingApplication::createBottomLevelAccelerationStructure(VkRayTracingApplication* app, Scene* scene)
{
    PFN_vkGetAccelerationStructureBuildSizesKHR pvkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkGetAccelerationStructureBuildSizesKHR");
    PFN_vkCreateAccelerationStructureKHR pvkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkCreateAccelerationStructureKHR");
    PFN_vkGetBufferDeviceAddressKHR pvkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkGetBufferDeviceAddressKHR");
    PFN_vkCmdBuildAccelerationStructuresKHR pvkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkCmdBuildAccelerationStructuresKHR");

    VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo = {};
    vertexBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    vertexBufferDeviceAddressInfo.buffer = app->vertexPositionBuffer;

    VkDeviceAddress vertexBufferAddress = pvkGetBufferDeviceAddressKHR(app->logicalDevice, &vertexBufferDeviceAddressInfo);

    VkDeviceOrHostAddressConstKHR vertexDeviceOrHostAddressConst = {};
    vertexDeviceOrHostAddressConst.deviceAddress = vertexBufferAddress;

    VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo = {};
    indexBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    indexBufferDeviceAddressInfo.buffer = app->indexBuffer;

    VkDeviceAddress indexBufferAddress = pvkGetBufferDeviceAddressKHR(app->logicalDevice, &indexBufferDeviceAddressInfo);

    VkDeviceOrHostAddressConstKHR indexDeviceOrHostAddressConst = {};
    indexDeviceOrHostAddressConst.deviceAddress = indexBufferAddress;
    VkDeviceOrHostAddressConstKHR transformDeviceOrHostAddressConst = {};
    transformDeviceOrHostAddressConst.deviceAddress = NULL;


    VkAccelerationStructureGeometryTrianglesDataKHR accelerationStructureGeometryTrianglesData = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
      .pNext = NULL,
      .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
      .vertexData = vertexDeviceOrHostAddressConst,
      .vertexStride = sizeof(float) * 3,
      .maxVertex = scene->attributes.num_vertices,
      .indexType = VK_INDEX_TYPE_UINT32,
      .indexData = indexDeviceOrHostAddressConst,
      .transformData = transformDeviceOrHostAddressConst
    };

    VkAccelerationStructureGeometryDataKHR accelerationStructureGeometryData = {
      .triangles = accelerationStructureGeometryTrianglesData
    };

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .pNext = NULL,
      .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
      .geometry = accelerationStructureGeometryData,
      .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .pNext = NULL,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .flags = 0,
      .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .srcAccelerationStructure = VK_NULL_HANDLE,
      .dstAccelerationStructure = VK_NULL_HANDLE,
      .geometryCount = 1,
      .pGeometries = &accelerationStructureGeometry,
      .ppGeometries = NULL,
      .scratchData = {}
    };

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
      .pNext = NULL,
      .accelerationStructureSize = 0,
      .updateScratchSize = 0,
      .buildScratchSize = 0
    };

    pvkGetAccelerationStructureBuildSizesKHR(app->logicalDevice,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR,
        &accelerationStructureBuildGeometryInfo,
        &accelerationStructureBuildGeometryInfo.geometryCount,
        &accelerationStructureBuildSizesInfo);

    createBuffer(app, accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->accelerationStructureBuffer, &app->accelerationStructureBufferMemory);

    VkBuffer scratchBuffer;
    VkDeviceMemory scratchBufferMemory;
    createBuffer(app,
        accelerationStructureBuildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &scratchBuffer,
        &scratchBufferMemory);


    VkBufferDeviceAddressInfo scratchBufferDeviceAddressInfo = {};
    scratchBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchBufferDeviceAddressInfo.buffer = scratchBuffer;

    VkDeviceAddress scratchBufferAddress = pvkGetBufferDeviceAddressKHR(app->logicalDevice, &scratchBufferDeviceAddressInfo);

    VkDeviceOrHostAddressKHR scratchDeviceOrHostAddress = {};
    scratchDeviceOrHostAddress.deviceAddress = scratchBufferAddress;

    accelerationStructureBuildGeometryInfo.scratchData = scratchDeviceOrHostAddress;

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .pNext = NULL,
      .createFlags = 0,
      .buffer = app->accelerationStructureBuffer,
      .offset = 0,
      .size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .deviceAddress = NULL
    };

    pvkCreateAccelerationStructureKHR(app->logicalDevice, &accelerationStructureCreateInfo, NULL, &app->accelerationStructure);

    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = app->accelerationStructure;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfoKHR = {
        .primitiveCount = scene->attributes.num_face_num_verts,   //三角形面数
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
    };
    const VkAccelerationStructureBuildRangeInfoKHR* accelerationStructureBuildRangeInfo = &accelerationStructureBuildRangeInfoKHR;
    

    const VkAccelerationStructureBuildRangeInfoKHR** accelerationStructureBuildRangeInfos = &accelerationStructureBuildRangeInfo;

    VkCommandBufferAllocateInfo bufferAllocateInfo = {};
    bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocateInfo.commandPool = app->commandPool;
    bufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(app->logicalDevice, &bufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    pvkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureBuildRangeInfos);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(app->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(app->computeQueue);

    vkFreeCommandBuffers(app->logicalDevice, app->commandPool, 1, &commandBuffer);

    vkDestroyBuffer(app->logicalDevice, scratchBuffer, NULL);
    vkFreeMemory(app->logicalDevice, scratchBufferMemory, NULL);
}

void VkRayTracingApplication::createTopLevelAccelerationStructure(VkRayTracingApplication* app)
{
    PFN_vkGetAccelerationStructureBuildSizesKHR pvkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkGetAccelerationStructureBuildSizesKHR");
    PFN_vkCreateAccelerationStructureKHR pvkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkCreateAccelerationStructureKHR");
    PFN_vkGetBufferDeviceAddressKHR pvkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkGetBufferDeviceAddressKHR");
    PFN_vkCmdBuildAccelerationStructuresKHR pvkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkCmdBuildAccelerationStructuresKHR");
    PFN_vkGetAccelerationStructureDeviceAddressKHR pvkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR");

    VkTransformMatrixKHR transformMatrix = {};
    transformMatrix.matrix[0][0] = 1.0;
    transformMatrix.matrix[1][1] = 1.0;
    transformMatrix.matrix[2][2] = 1.0;

    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = app->accelerationStructure;

    VkDeviceAddress accelerationStructureDeviceAddress = pvkGetAccelerationStructureDeviceAddressKHR(app->logicalDevice, &accelerationStructureDeviceAddressInfo);

    VkAccelerationStructureInstanceKHR geometryInstance = {};
    geometryInstance.transform = transformMatrix;
    geometryInstance.instanceCustomIndex = 0;
    geometryInstance.mask = 0xFF;
    geometryInstance.instanceShaderBindingTableRecordOffset = 0;
    geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    geometryInstance.accelerationStructureReference = accelerationStructureDeviceAddress;

    VkDeviceSize geometryInstanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR);

    VkBuffer geometryInstanceStagingBuffer;
    VkDeviceMemory geometryInstanceStagingBufferMemory;
    createBuffer(app, geometryInstanceBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &geometryInstanceStagingBuffer, &geometryInstanceStagingBufferMemory);

    void* geometryInstanceData;
    vkMapMemory(app->logicalDevice, geometryInstanceStagingBufferMemory, 0, geometryInstanceBufferSize, 0, &geometryInstanceData);
    memcpy(geometryInstanceData, &geometryInstance, geometryInstanceBufferSize);
    vkUnmapMemory(app->logicalDevice, geometryInstanceStagingBufferMemory);

    VkBuffer geometryInstanceBuffer;
    VkDeviceMemory geometryInstanceBufferMemory;
    createBuffer(app, geometryInstanceBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &geometryInstanceBuffer, &geometryInstanceBufferMemory);

    copyBuffer(app, geometryInstanceStagingBuffer, geometryInstanceBuffer, geometryInstanceBufferSize);

    vkDestroyBuffer(app->logicalDevice, geometryInstanceStagingBuffer, NULL);
    vkFreeMemory(app->logicalDevice, geometryInstanceStagingBufferMemory, NULL);

    VkBufferDeviceAddressInfo geometryInstanceBufferDeviceAddressInfo = {};
    geometryInstanceBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    geometryInstanceBufferDeviceAddressInfo.buffer = geometryInstanceBuffer;

    VkDeviceAddress geometryInstanceBufferAddress = pvkGetBufferDeviceAddressKHR(app->logicalDevice, &geometryInstanceBufferDeviceAddressInfo);

    VkDeviceOrHostAddressConstKHR geometryInstanceDeviceOrHostAddressConst = {
      .deviceAddress = geometryInstanceBufferAddress
    };

    VkAccelerationStructureGeometryInstancesDataKHR accelerationStructureGeometryInstancesData = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
      .pNext = NULL,
      .arrayOfPointers = VK_FALSE,
      .data = geometryInstanceDeviceOrHostAddressConst
    };

    VkAccelerationStructureGeometryDataKHR accelerationStructureGeometryData = {
      .instances = accelerationStructureGeometryInstancesData
    };

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .pNext = NULL,
      .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
      .geometry = accelerationStructureGeometryData,
      .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .pNext = NULL,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
      .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .srcAccelerationStructure = VK_NULL_HANDLE,
      .dstAccelerationStructure = VK_NULL_HANDLE,
      .geometryCount = 1,
      .pGeometries = &accelerationStructureGeometry,
      .ppGeometries = NULL,
      .scratchData = {}
    };

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
      .pNext = NULL,
      .accelerationStructureSize = 0,
      .updateScratchSize = 0,
      .buildScratchSize = 0
    };

    pvkGetAccelerationStructureBuildSizesKHR(app->logicalDevice,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR,
        &accelerationStructureBuildGeometryInfo,
        &accelerationStructureBuildGeometryInfo.geometryCount,
        &accelerationStructureBuildSizesInfo);

    createBuffer(app, accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->topLevelAccelerationStructureBuffer, &app->topLevelAccelerationStructureBufferMemory);

    VkBuffer scratchBuffer;
    VkDeviceMemory scratchBufferMemory;
    createBuffer(app,
        accelerationStructureBuildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &scratchBuffer,
        &scratchBufferMemory);


    VkBufferDeviceAddressInfo scratchBufferDeviceAddressInfo = {};
    scratchBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchBufferDeviceAddressInfo.buffer = scratchBuffer;

    VkDeviceAddress scratchBufferAddress = pvkGetBufferDeviceAddressKHR(app->logicalDevice, &scratchBufferDeviceAddressInfo);

    VkDeviceOrHostAddressKHR scratchDeviceOrHostAddress = {};
    scratchDeviceOrHostAddress.deviceAddress = scratchBufferAddress;

    accelerationStructureBuildGeometryInfo.scratchData = scratchDeviceOrHostAddress;

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .pNext = NULL,
      .createFlags = 0,
      .buffer = app->topLevelAccelerationStructureBuffer,
      .offset = 0,
      .size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .deviceAddress = NULL
    };

    pvkCreateAccelerationStructureKHR(app->logicalDevice, &accelerationStructureCreateInfo, NULL, &app->topLevelAccelerationStructure);

    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = app->topLevelAccelerationStructure;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfoKHR = {
        .primitiveCount = 1,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
    };
    const VkAccelerationStructureBuildRangeInfoKHR* accelerationStructureBuildRangeInfo = &accelerationStructureBuildRangeInfoKHR;
    const VkAccelerationStructureBuildRangeInfoKHR** accelerationStructureBuildRangeInfos = &accelerationStructureBuildRangeInfo;

    VkCommandBufferAllocateInfo bufferAllocateInfo = {};
    bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocateInfo.commandPool = app->commandPool;
    bufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(app->logicalDevice, &bufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    pvkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureBuildRangeInfos);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(app->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(app->computeQueue);

    vkFreeCommandBuffers(app->logicalDevice, app->commandPool, 1, &commandBuffer);

    vkDestroyBuffer(app->logicalDevice, scratchBuffer, NULL);
    vkFreeMemory(app->logicalDevice, scratchBufferMemory, NULL);
}

void VkRayTracingApplication::createUniformBuffer(VkRayTracingApplication* app)
{
    //We create a uniform buffer to hold the camera so we can pass the camera's data into the shaders.
    VkDeviceSize bufferSize = sizeof(Camera);
    createBuffer(app, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &app->uniformBuffer, &app->uniformBufferMemory);

    VkDeviceSize bufferSize_s = sizeof(ShadingMode);
    createBuffer(app, bufferSize_s, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &app->uniformBuffer_shadingMode, &app->uniformBufferMemory_shadingMode);
}

void VkRayTracingApplication::createDescriptorSets(VkRayTracingApplication* app)
{
    app->rayTraceDescriptorSetLayouts = (VkDescriptorSetLayout*)malloc(sizeof(VkDescriptorSetLayout) * 2);

    VkDescriptorPoolSize descriptorPoolSizes[5];
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    descriptorPoolSizes[0].descriptorCount = 1;

    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[1].descriptorCount = 2;

    descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorPoolSizes[2].descriptorCount = 4;

    descriptorPoolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorPoolSizes[3].descriptorCount = 10;

    descriptorPoolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSizes[4].descriptorCount = 4;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.poolSizeCount = 5;
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
    descriptorPoolCreateInfo.maxSets = 2;

    if (vkCreateDescriptorPool(app->logicalDevice, &descriptorPoolCreateInfo, NULL, &app->descriptorPool) == VK_SUCCESS) {
        printf("\033[22;32m%s\033[0m\n", "created descriptor pool");
    }

    {
        VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[15];
        descriptorSetLayoutBindings[0].binding = 0;
        descriptorSetLayoutBindings[0].descriptorCount = 1;
        descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        descriptorSetLayoutBindings[0].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[1].binding = 1;
        descriptorSetLayoutBindings[1].descriptorCount = 1;
        descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetLayoutBindings[1].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[2].binding = 2;
        descriptorSetLayoutBindings[2].descriptorCount = 1;
        descriptorSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorSetLayoutBindings[2].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[3].binding = 3;
        descriptorSetLayoutBindings[3].descriptorCount = 1;
        descriptorSetLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorSetLayoutBindings[3].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[4].binding = 4;           //前一帧图像
        descriptorSetLayoutBindings[4].descriptorCount = 1;
        descriptorSetLayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[4].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[5].binding = 5;
        descriptorSetLayoutBindings[5].descriptorCount = 1;
        descriptorSetLayoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetLayoutBindings[5].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[5].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[6].binding = 6;           //前一帧indierctLgt
        descriptorSetLayoutBindings[6].descriptorCount = 1;
        descriptorSetLayoutBindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[6].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[7].binding = 7;           //前一帧indierctLgt_2
        descriptorSetLayoutBindings[7].descriptorCount = 1;
        descriptorSetLayoutBindings[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[7].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[8].binding = 8;           //dierctLgt irrad
        descriptorSetLayoutBindings[8].descriptorCount = 1;
        descriptorSetLayoutBindings[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[8].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[8].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[9].binding = 9;           //dierctLgt albedo
        descriptorSetLayoutBindings[9].descriptorCount = 1;
        descriptorSetLayoutBindings[9].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[9].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[9].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[10].binding = 10;           //normal
        descriptorSetLayoutBindings[10].descriptorCount = 1;
        descriptorSetLayoutBindings[10].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[10].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[10].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[11].binding = 11;           //world
        descriptorSetLayoutBindings[11].descriptorCount = 1;
        descriptorSetLayoutBindings[11].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[11].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[11].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[12].binding = 12;           //Depth
        descriptorSetLayoutBindings[12].descriptorCount = 1;
        descriptorSetLayoutBindings[12].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[12].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[12].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[13].binding = 13;           //Variance
        descriptorSetLayoutBindings[13].descriptorCount = 1;
        descriptorSetLayoutBindings[13].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBindings[13].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[13].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[14].binding = 14;           //Texture
        descriptorSetLayoutBindings[14].descriptorCount = 1;
        descriptorSetLayoutBindings[14].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorSetLayoutBindings[14].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[14].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount = 15;
        descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;

        if (vkCreateDescriptorSetLayout(app->logicalDevice, &descriptorSetLayoutCreateInfo, NULL, &app->rayTraceDescriptorSetLayouts[0]) == VK_SUCCESS) {
            printf("\033[22;32m%s\033[0m\n", "created descriptor set layout");
        }

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = app->descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.pSetLayouts = &app->rayTraceDescriptorSetLayouts[0];

        if (vkAllocateDescriptorSets(app->logicalDevice, &descriptorSetAllocateInfo, &app->rayTraceDescriptorSet) == VK_SUCCESS) {
            printf("\033[22;32m%s\033[0m\n", "allocated descriptor sets");
        }

        VkWriteDescriptorSet writeDescriptorSets[15];

        VkWriteDescriptorSetAccelerationStructureKHR descriptorSetAccelerationStructure = {};
        descriptorSetAccelerationStructure.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptorSetAccelerationStructure.pNext = NULL;
        descriptorSetAccelerationStructure.accelerationStructureCount = 1;
        descriptorSetAccelerationStructure.pAccelerationStructures = &app->topLevelAccelerationStructure;

        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].pNext = &descriptorSetAccelerationStructure;
        writeDescriptorSets[0].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].dstArrayElement = 0;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        writeDescriptorSets[0].pImageInfo = NULL;
        writeDescriptorSets[0].pBufferInfo = NULL;
        writeDescriptorSets[0].pTexelBufferView = NULL;

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = app->uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].pNext = NULL;
        writeDescriptorSets[1].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].dstArrayElement = 0;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[1].pImageInfo = NULL;
        writeDescriptorSets[1].pBufferInfo = &bufferInfo;
        writeDescriptorSets[1].pTexelBufferView = NULL;

        VkDescriptorBufferInfo indexBufferInfo = {};
        indexBufferInfo.buffer = app->indexBuffer;
        indexBufferInfo.offset = 0;
        indexBufferInfo.range = VK_WHOLE_SIZE;

        writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[2].pNext = NULL;
        writeDescriptorSets[2].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[2].dstBinding = 2;
        writeDescriptorSets[2].dstArrayElement = 0;
        writeDescriptorSets[2].descriptorCount = 1;
        writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[2].pImageInfo = NULL;
        writeDescriptorSets[2].pBufferInfo = &indexBufferInfo;
        writeDescriptorSets[2].pTexelBufferView = NULL;

        VkDescriptorBufferInfo vertexBufferInfo = {};
        vertexBufferInfo.buffer = app->vertexPositionBuffer;
        vertexBufferInfo.offset = 0;
        vertexBufferInfo.range = VK_WHOLE_SIZE;

        writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[3].pNext = NULL;
        writeDescriptorSets[3].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[3].dstBinding = 3;
        writeDescriptorSets[3].dstArrayElement = 0;
        writeDescriptorSets[3].descriptorCount = 1;
        writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[3].pImageInfo = NULL;
        writeDescriptorSets[3].pBufferInfo = &vertexBufferInfo;
        writeDescriptorSets[3].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.sampler = (VkSampler)VK_DESCRIPTOR_TYPE_SAMPLER;
        imageInfo.imageView = app->rayTraceImageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[4].pNext = NULL;
        writeDescriptorSets[4].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[4].dstBinding = 4;
        writeDescriptorSets[4].dstArrayElement = 0;
        writeDescriptorSets[4].descriptorCount = 1;
        writeDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[4].pImageInfo = &imageInfo;
        writeDescriptorSets[4].pBufferInfo = NULL;
        writeDescriptorSets[4].pTexelBufferView = NULL;

        VkDescriptorBufferInfo bufferInfo_shadingMode = {};
        bufferInfo_shadingMode.buffer = app->uniformBuffer_shadingMode;
        bufferInfo_shadingMode.offset = 0;
        bufferInfo_shadingMode.range = VK_WHOLE_SIZE;

        writeDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[5].pNext = NULL;
        writeDescriptorSets[5].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[5].dstBinding = 5;
        writeDescriptorSets[5].dstArrayElement = 0;
        writeDescriptorSets[5].descriptorCount = 1;
        writeDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[5].pImageInfo = NULL;
        writeDescriptorSets[5].pBufferInfo = &bufferInfo_shadingMode;
        writeDescriptorSets[5].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_indirectLgt = {};
        imageInfo_indirectLgt.sampler = (VkSampler)VK_DESCRIPTOR_TYPE_SAMPLER;
        imageInfo_indirectLgt.imageView = app->ImageView_indirectLgt;
        imageInfo_indirectLgt.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        writeDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[6].pNext = NULL;
        writeDescriptorSets[6].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[6].dstBinding = 6;
        writeDescriptorSets[6].dstArrayElement = 0;
        writeDescriptorSets[6].descriptorCount = 1;
        writeDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[6].pImageInfo = &imageInfo_indirectLgt;
        writeDescriptorSets[6].pBufferInfo = NULL;
        writeDescriptorSets[6].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_indirectLgt_2 = {};
        imageInfo_indirectLgt_2.sampler = (VkSampler)VK_DESCRIPTOR_TYPE_SAMPLER;
        imageInfo_indirectLgt_2.imageView = app->ImageView_indirectLgt_2;
        imageInfo_indirectLgt_2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        writeDescriptorSets[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[7].pNext = NULL;
        writeDescriptorSets[7].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[7].dstBinding = 7;
        writeDescriptorSets[7].dstArrayElement = 0;
        writeDescriptorSets[7].descriptorCount = 1;
        writeDescriptorSets[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[7].pImageInfo = &imageInfo_indirectLgt_2;
        writeDescriptorSets[7].pBufferInfo = NULL;
        writeDescriptorSets[7].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_directLgt = imageInfo_indirectLgt_2;
        imageInfo_directLgt.imageView = app->HDirectIrradImageView;

        writeDescriptorSets[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[8].pNext = NULL;
        writeDescriptorSets[8].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[8].dstBinding = 8;
        writeDescriptorSets[8].dstArrayElement = 0;
        writeDescriptorSets[8].descriptorCount = 1;
        writeDescriptorSets[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[8].pImageInfo = &imageInfo_directLgt;
        writeDescriptorSets[8].pBufferInfo = NULL;
        writeDescriptorSets[8].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_directLgt_albedo = imageInfo_indirectLgt_2;
        imageInfo_directLgt_albedo.imageView = app->HDirectAlbedoImageView;

        writeDescriptorSets[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[9].pNext = NULL;
        writeDescriptorSets[9].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[9].dstBinding = 9;
        writeDescriptorSets[9].dstArrayElement = 0;
        writeDescriptorSets[9].descriptorCount = 1;
        writeDescriptorSets[9].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[9].pImageInfo = &imageInfo_directLgt_albedo;
        writeDescriptorSets[9].pBufferInfo = NULL;
        writeDescriptorSets[9].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_normal = imageInfo_indirectLgt_2;
        imageInfo_normal.imageView = app->HNormalImageView;

        writeDescriptorSets[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[10].pNext = NULL;
        writeDescriptorSets[10].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[10].dstBinding = 10;
        writeDescriptorSets[10].dstArrayElement = 0;
        writeDescriptorSets[10].descriptorCount = 1;
        writeDescriptorSets[10].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[10].pImageInfo = &imageInfo_normal;
        writeDescriptorSets[10].pBufferInfo = NULL;
        writeDescriptorSets[10].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_world = imageInfo_indirectLgt_2;
        imageInfo_world.imageView = app->HWorldPosImageView;

        writeDescriptorSets[11].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[11].pNext = NULL;
        writeDescriptorSets[11].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[11].dstBinding = 11;
        writeDescriptorSets[11].dstArrayElement = 0;
        writeDescriptorSets[11].descriptorCount = 1;
        writeDescriptorSets[11].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[11].pImageInfo = &imageInfo_world;
        writeDescriptorSets[11].pBufferInfo = NULL;
        writeDescriptorSets[11].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_depth = imageInfo_indirectLgt_2;
        imageInfo_depth.imageView = app->HDepthImageView;

        writeDescriptorSets[12].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[12].pNext = NULL;
        writeDescriptorSets[12].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[12].dstBinding = 12;
        writeDescriptorSets[12].dstArrayElement = 0;
        writeDescriptorSets[12].descriptorCount = 1;
        writeDescriptorSets[12].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[12].pImageInfo = &imageInfo_depth;
        writeDescriptorSets[12].pBufferInfo = NULL;
        writeDescriptorSets[12].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_var = imageInfo_indirectLgt_2;
        imageInfo_var.imageView = app->HVarImageView;

        writeDescriptorSets[13].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[13].pNext = NULL;
        writeDescriptorSets[13].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[13].dstBinding = 13;
        writeDescriptorSets[13].dstArrayElement = 0;
        writeDescriptorSets[13].descriptorCount = 1;
        writeDescriptorSets[13].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[13].pImageInfo = &imageInfo_var;
        writeDescriptorSets[13].pBufferInfo = NULL;
        writeDescriptorSets[13].pTexelBufferView = NULL;

        VkDescriptorImageInfo imageInfo_tex{};
        imageInfo_tex.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo_tex.imageView = textureImageView;
        imageInfo_tex.sampler = textureSampler;

        writeDescriptorSets[14].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[14].pNext = NULL;
        writeDescriptorSets[14].dstSet = app->rayTraceDescriptorSet;
        writeDescriptorSets[14].dstBinding = 14;
        writeDescriptorSets[14].dstArrayElement = 0;
        writeDescriptorSets[14].descriptorCount = 1;
        writeDescriptorSets[14].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[14].pImageInfo = &imageInfo_tex;
        writeDescriptorSets[14].pBufferInfo = NULL;
        writeDescriptorSets[14].pTexelBufferView = NULL;

        vkUpdateDescriptorSets(app->logicalDevice, 15, writeDescriptorSets, 0, NULL);
    }

    {
        VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2];
        descriptorSetLayoutBindings[0].binding = 0;
        descriptorSetLayoutBindings[0].descriptorCount = 1;
        descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorSetLayoutBindings[0].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorSetLayoutBindings[1].binding = 1;
        descriptorSetLayoutBindings[1].descriptorCount = 1;
        descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorSetLayoutBindings[1].pImmutableSamplers = NULL;
        descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount = 2;
        descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;

        if (vkCreateDescriptorSetLayout(app->logicalDevice, &descriptorSetLayoutCreateInfo, NULL, &app->rayTraceDescriptorSetLayouts[1]) == VK_SUCCESS) {
            printf("\033[22;32m%s\033[0m\n", "created descriptor set layout");
        }

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = app->descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.pSetLayouts = &app->rayTraceDescriptorSetLayouts[1];

        if (vkAllocateDescriptorSets(app->logicalDevice, &descriptorSetAllocateInfo, &app->materialDescriptorSet) == VK_SUCCESS) {
            printf("\033[22;32m%s\033[0m\n", "allocated descriptor sets");
        }

        VkWriteDescriptorSet writeDescriptorSets[2];

        VkDescriptorBufferInfo materialIndexBufferInfo = {};
        materialIndexBufferInfo.buffer = app->materialIndexBuffer;
        materialIndexBufferInfo.offset = 0;
        materialIndexBufferInfo.range = VK_WHOLE_SIZE;

        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].pNext = NULL;
        writeDescriptorSets[0].dstSet = app->materialDescriptorSet;
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].dstArrayElement = 0;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[0].pImageInfo = NULL;
        writeDescriptorSets[0].pBufferInfo = &materialIndexBufferInfo;
        writeDescriptorSets[0].pTexelBufferView = NULL;

        VkDescriptorBufferInfo materialBufferInfo = {};
        materialBufferInfo.buffer = app->materialBuffer;
        materialBufferInfo.offset = 0;
        materialBufferInfo.range = VK_WHOLE_SIZE;

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].pNext = NULL;
        writeDescriptorSets[1].dstSet = app->materialDescriptorSet;
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].dstArrayElement = 0;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[1].pImageInfo = NULL;
        writeDescriptorSets[1].pBufferInfo = &materialBufferInfo;
        writeDescriptorSets[1].pTexelBufferView = NULL;

        vkUpdateDescriptorSets(app->logicalDevice, 2, writeDescriptorSets, 0, NULL);
    }
}

void VkRayTracingApplication::createRayTracePipeline(VkRayTracingApplication* app)
{
    //Our ray tracing pipeline uses the following 4 shaders: 
      //raytrace.rgen, 
      //raytrace.rmiss, 
      //raytrace_shadow.rmiss, 
      //raytrace.rchit.
      //First, the rays are generated with the raytrace.rgen shader stage which are then casted onto the acceleration structure.
      //If the ray intersects geometry, the raytrace.rchit shader stage will handle the ray's payload. 
      //If the ray does not intersect any geometry, the raytrace.rchit shader stage will handle the ray's payload.

      //We load the shaders just like how we would load a vertex or fragment shader.
      //The only difference is setting the shader stage info to the correct shader stage.
    PFN_vkCreateRayTracingPipelinesKHR pvkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkCreateRayTracingPipelinesKHR");

    std::string str = GetExePath();
    std::string basePath = str + "\\data\\";
    str += "\\data\\shaders\\raytrace.rgen.spv";
    FILE* rgenFile = fopen(str.c_str(), "rb");
    //FILE* rgenFile = fopen("shaders/raytrace.rgen.spv", "rb");
    fseek(rgenFile, 0, SEEK_END);
    uint32_t rgenFileSize = ftell(rgenFile);
    fseek(rgenFile, 0, SEEK_SET);

    char* rgenFileBuffer = (char*)malloc(sizeof(char*) * rgenFileSize);
    fread(rgenFileBuffer, 1, rgenFileSize, rgenFile);
    fclose(rgenFile);

    str = basePath + "shaders\\raytrace.rmiss.spv";
    FILE* rmissFile = fopen(str.c_str(), "rb");
    //FILE* rmissFile = fopen("shaders/raytrace.rmiss.spv", "rb");
    fseek(rmissFile, 0, SEEK_END);
    uint32_t rmissFileSize = ftell(rmissFile);
    fseek(rmissFile, 0, SEEK_SET);

    char* rmissFileBuffer = (char*)malloc(sizeof(char*) * rmissFileSize);
    fread(rmissFileBuffer, 1, rmissFileSize, rmissFile);
    fclose(rmissFile);

    str = basePath + "shaders\\raytrace_shadow.rmiss.spv";
    FILE* rmissShadowFile = fopen(str.c_str(), "rb");
    //FILE* rmissShadowFile = fopen("shaders/raytrace_shadow.rmiss.spv", "rb");
    fseek(rmissShadowFile, 0, SEEK_END);
    uint32_t rmissShadowFileSize = ftell(rmissShadowFile);
    fseek(rmissShadowFile, 0, SEEK_SET);

    char* rmissShadowFileBuffer = (char*)malloc(sizeof(char*) * rmissShadowFileSize);
    fread(rmissShadowFileBuffer, 1, rmissShadowFileSize, rmissShadowFile);
    fclose(rmissShadowFile);


    str = basePath + "shaders\\raytrace.rchit.spv";
    FILE* rchitFile = fopen(str.c_str(), "rb");
    //FILE* rchitFile = fopen("shaders/raytrace.rchit.spv", "rb");
    fseek(rchitFile, 0, SEEK_END);
    uint32_t rchitFileSize = ftell(rchitFile);
    fseek(rchitFile, 0, SEEK_SET);

    char* rchitFileBuffer = (char*)malloc(sizeof(char*) * rchitFileSize);
    fread(rchitFileBuffer, 1, rchitFileSize, rchitFile);
    fclose(rchitFile);

    VkShaderModuleCreateInfo rgenShaderModuleCreateInfo = {};
    rgenShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    rgenShaderModuleCreateInfo.codeSize = rgenFileSize;
    rgenShaderModuleCreateInfo.pCode = (uint32_t*)rgenFileBuffer;

    VkShaderModule rgenShaderModule;
    if (vkCreateShaderModule(app->logicalDevice, &rgenShaderModuleCreateInfo, NULL, &rgenShaderModule) == VK_SUCCESS) {
        printf("\033[22;32m%s\033[0m\n", "created rgen shader module");
    }

    VkShaderModuleCreateInfo rmissShaderModuleCreateInfo = {};
    rmissShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    rmissShaderModuleCreateInfo.codeSize = rmissFileSize;
    rmissShaderModuleCreateInfo.pCode = (uint32_t*)rmissFileBuffer;

    VkShaderModule rmissShaderModule;
    if (vkCreateShaderModule(app->logicalDevice, &rmissShaderModuleCreateInfo, NULL, &rmissShaderModule) == VK_SUCCESS) {
        printf("\033[22;32m%s\033[0m\n", "created rmiss shader module");
    }

    VkShaderModuleCreateInfo rmissShadowShaderModuleCreateInfo = {};
    rmissShadowShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    rmissShadowShaderModuleCreateInfo.codeSize = rmissShadowFileSize;
    rmissShadowShaderModuleCreateInfo.pCode = (uint32_t*)rmissShadowFileBuffer;

    VkShaderModule rmissShadowShaderModule;
    if (vkCreateShaderModule(app->logicalDevice, &rmissShadowShaderModuleCreateInfo, NULL, &rmissShadowShaderModule) == VK_SUCCESS) {
        printf("\033[22;32m%s\033[0m\n", "created rmiss shadow shader module");
    }

    VkShaderModuleCreateInfo rchitShaderModuleCreateInfo = {};
    rchitShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    rchitShaderModuleCreateInfo.codeSize = rchitFileSize;
    rchitShaderModuleCreateInfo.pCode = (uint32_t*)rchitFileBuffer;

    VkShaderModule rchitShaderModule;
    if (vkCreateShaderModule(app->logicalDevice, &rchitShaderModuleCreateInfo, NULL, &rchitShaderModule) == VK_SUCCESS) {
        printf("\033[22;32m%s\033[0m\n", "created rchit shader module");
    }

    VkPipelineShaderStageCreateInfo rgenShaderStageInfo = {};
    rgenShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rgenShaderStageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    rgenShaderStageInfo.module = rgenShaderModule;
    rgenShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo rmissShaderStageInfo = {};
    rmissShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rmissShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    rmissShaderStageInfo.module = rmissShaderModule;
    rmissShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo rmissShadowShaderStageInfo = {};
    rmissShadowShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rmissShadowShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    rmissShadowShaderStageInfo.module = rmissShadowShaderModule;
    rmissShadowShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo rchitShaderStageInfo = {};
    rchitShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rchitShaderStageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    rchitShaderStageInfo.module = rchitShaderModule;
    rchitShaderStageInfo.pName = "main";

    //We now create the shader groups to contain the previously described shader stages.
    VkPipelineShaderStageCreateInfo shaderStages[4] = { rgenShaderStageInfo, rmissShaderStageInfo, rmissShadowShaderStageInfo, rchitShaderStageInfo };

    VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfos[4];
    shaderGroupCreateInfos[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroupCreateInfos[0].pNext = NULL;
    shaderGroupCreateInfos[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shaderGroupCreateInfos[0].generalShader = 0;
    shaderGroupCreateInfos[0].closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[0].anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[0].intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[0].pShaderGroupCaptureReplayHandle = NULL;

    shaderGroupCreateInfos[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroupCreateInfos[1].pNext = NULL;
    shaderGroupCreateInfos[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shaderGroupCreateInfos[1].generalShader = 1;
    shaderGroupCreateInfos[1].closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[1].anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[1].intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[1].pShaderGroupCaptureReplayHandle = NULL;

    shaderGroupCreateInfos[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroupCreateInfos[2].pNext = NULL;
    shaderGroupCreateInfos[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shaderGroupCreateInfos[2].generalShader = 2;
    shaderGroupCreateInfos[2].closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[2].anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[2].intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[2].pShaderGroupCaptureReplayHandle = NULL;

    shaderGroupCreateInfos[3].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroupCreateInfos[3].pNext = NULL;
    shaderGroupCreateInfos[3].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    shaderGroupCreateInfos[3].generalShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[3].closestHitShader = 3;
    shaderGroupCreateInfos[3].anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[3].intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfos[3].pShaderGroupCaptureReplayHandle = NULL;

    //The pipeline layout can be created using the previously created descriptor set layouts.
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 2;
    pipelineLayoutCreateInfo.pSetLayouts = app->rayTraceDescriptorSetLayouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(app->logicalDevice, &pipelineLayoutCreateInfo, NULL, &app->rayTracePipelineLayout) == VK_SUCCESS) {
        printf("created ray trace pipline layout\n");
    }

    // With all the structures we just defined, we can finally create the ray tracing pipeline!
    VkPipelineLibraryCreateInfoKHR pipelineLibraryCreateInfo = {};
    pipelineLibraryCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
    pipelineLibraryCreateInfo.pNext = NULL;
    pipelineLibraryCreateInfo.libraryCount = 0;
    pipelineLibraryCreateInfo.pLibraries = NULL;

    VkRayTracingPipelineCreateInfoKHR rayPipelineCreateInfo = {};
    rayPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayPipelineCreateInfo.pNext = NULL;
    rayPipelineCreateInfo.flags = 0;
    rayPipelineCreateInfo.stageCount = 4;
    rayPipelineCreateInfo.pStages = shaderStages;
    rayPipelineCreateInfo.groupCount = 4;
    rayPipelineCreateInfo.pGroups = shaderGroupCreateInfos;
    rayPipelineCreateInfo.maxPipelineRayRecursionDepth = 16;
    rayPipelineCreateInfo.pLibraryInfo = &pipelineLibraryCreateInfo;
    rayPipelineCreateInfo.pLibraryInterface = NULL;
    rayPipelineCreateInfo.layout = app->rayTracePipelineLayout;
    rayPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    rayPipelineCreateInfo.basePipelineIndex = -1;

    if (pvkCreateRayTracingPipelinesKHR(app->logicalDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayPipelineCreateInfo, NULL, &app->rayTracePipeline) == VK_SUCCESS) {
        printf("created ray trace pipeline\n");
    }

    vkDestroyShaderModule(app->logicalDevice, rgenShaderModule, NULL);
    vkDestroyShaderModule(app->logicalDevice, rmissShaderModule, NULL);
    vkDestroyShaderModule(app->logicalDevice, rmissShadowShaderModule, NULL);
    vkDestroyShaderModule(app->logicalDevice, rchitShaderModule, NULL);

    free(rgenFileBuffer);
    free(rmissFileBuffer);
    free(rmissShadowFileBuffer);
    free(rchitFileBuffer);
}

void VkRayTracingApplication::createShaderBindingTable(VkRayTracingApplication* app)
{
    //The shader binding table holds the handles of all the shaders and will be used to determine which shader should be executed. 
      //Since we have 4 shaders, 1 ray generation, 1 ray closest hit, and 2 ray miss, 
      //we must make sure that all the shader handles will fit within the shader binding table buffer.
    PFN_vkGetRayTracingShaderGroupHandlesKHR pvkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(app->logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR");

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = {};
    rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 properties = {};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &rayTracingProperties;

    vkGetPhysicalDeviceProperties2(app->physicalDevice, &properties);

    VkDeviceSize shaderBindingTableSize = rayTracingProperties.shaderGroupHandleSize * 4;

    app->shaderBindingTableBufferMemory;
    createBuffer(app, shaderBindingTableSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &app->shaderBindingTableBuffer, &app->shaderBindingTableBufferMemory);

    void* shaderHandleStorage = (void*)malloc(sizeof(uint8_t) * shaderBindingTableSize);
    if (pvkGetRayTracingShaderGroupHandlesKHR(app->logicalDevice, app->rayTracePipeline, 0, 4, shaderBindingTableSize, shaderHandleStorage) == VK_SUCCESS) {
        printf("got ray tracing shader group handles\n");
    }

    void* data;
    vkMapMemory(app->logicalDevice, app->shaderBindingTableBufferMemory, 0, shaderBindingTableSize, 0, &data);
    for (int x = 0; x < 4; x++) {
        memcpy(data, (uint8_t*)shaderHandleStorage + x * rayTracingProperties.shaderGroupHandleSize, rayTracingProperties.shaderGroupHandleSize);
        //uint32_t intData = (uint32_t)data;
        //intData += rayTracingProperties.shaderGroupBaseAlignment;
        //data = (void*)intData;
        //data =data + rayTracingProperties.shaderGroupBaseAlignment;
        intptr_t intData = (intptr_t)data;
        intData += rayTracingProperties.shaderGroupBaseAlignment;
        data = (void*)intData;
    }
    vkUnmapMemory(app->logicalDevice, app->shaderBindingTableBufferMemory);

    free(shaderHandleStorage);
}

void VkRayTracingApplication::createCommandBuffers(VkRayTracingApplication* app, Scene* scene)
{
    app->commandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * app->imageCount);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = app->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = app->imageCount;

    if (vkAllocateCommandBuffers(app->logicalDevice, &commandBufferAllocateInfo, app->commandBuffers) == VK_SUCCESS) {
        printf("allocated command buffers\n");
    }

    for (int x = 0; x < app->imageCount; x++) {
        VkCommandBufferBeginInfo commandBufferBeginCreateInfo = {};
        commandBufferBeginCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = app->renderPass;
        renderPassBeginInfo.framebuffer = app->swapchainFramebuffers[x];    //指定渲染结果保存到这里
        VkOffset2D renderAreaOffset = { 0, 0 };
        renderPassBeginInfo.renderArea.offset = renderAreaOffset;
        renderPassBeginInfo.renderArea.extent = app->swapchainExtent;

        VkClearValue clearValues[7] = {
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.depthStencil = {1.0f, 0}}
        };

        // Clear values for all attachments written in the fragment shader
        //std::array<VkClearValue, 4> clearValues;
        //clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        //clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        //clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        //clearValues[3].depthStencil = { 1.0f, 0 };

        renderPassBeginInfo.clearValueCount = app->colorAttachCount+1;
        renderPassBeginInfo.pClearValues = clearValues;

        VkBuffer vertexBuffers[1] = { app->vertexPositionBuffer };
        VkDeviceSize offsets[1] = { 0 };

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        if (vkBeginCommandBuffer(app->commandBuffers[x], &commandBufferBeginCreateInfo) == VK_SUCCESS) {
            printf("begin recording command buffer for image #%d\n", x);
        }

        vkCmdBeginRenderPass(app->commandBuffers[x], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline);

        vkCmdBindVertexBuffers(app->commandBuffers[x], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(app->commandBuffers[x], app->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 0, 1, &app->rayTraceDescriptorSet, 0, 0);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 1, 1, &app->materialDescriptorSet, 0, 0);

        vkCmdDrawIndexed(app->commandBuffers[x], scene->attributes.num_faces, 1, 0, 0, 0);
        vkCmdEndRenderPass(app->commandBuffers[x]);

        {
            VkImageSubresourceLayers subresourceLayers = {};
            subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceLayers.mipLevel = 0;
            subresourceLayers.baseArrayLayer = 0;
            subresourceLayers.layerCount = 1;

            VkOffset3D offset = {};
            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};
            extent.width = WIDTH;
            extent.height = HEIGHT;
            extent.depth = 1;

            VkImageCopy imageCopy = {};
            imageCopy.srcSubresource = subresourceLayers;
            imageCopy.srcOffset = offset;
            imageCopy.dstSubresource = subresourceLayers;
            imageCopy.dstOffset = offset;
            imageCopy.extent = extent;

            //copy 当前帧 渲染结果 到 rayTraceImage
            vkCmdCopyImage(app->commandBuffers[x], app->swapchainImages[x], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->rayTraceImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            //vkCmdCopyImage(app->commandBuffers[x], app->GnormalImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->Image_indirectLgt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        }


        if (vkEndCommandBuffer(app->commandBuffers[x]) == VK_SUCCESS) {
            printf("end recording command buffer for image #%d\n", x);
        }
    }
}

void VkRayTracingApplication::createCommandBuffers_2pass(VkRayTracingApplication* app, Scene* scene)
{
    app->commandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * app->imageCount);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = app->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = app->imageCount;

    if (vkAllocateCommandBuffers(app->logicalDevice, &commandBufferAllocateInfo, app->commandBuffers) == VK_SUCCESS) {
        printf("allocated command buffers\n");
    }

    for (int x = 0; x < app->imageCount; x++) {
        VkCommandBufferBeginInfo commandBufferBeginCreateInfo = {};
        commandBufferBeginCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = app->renderPass;
        renderPassBeginInfo.framebuffer = app->swapchainFramebuffers[x];    //指定渲染结果保存到这里
        VkOffset2D renderAreaOffset = { 0, 0 };
        renderPassBeginInfo.renderArea.offset = renderAreaOffset;
        renderPassBeginInfo.renderArea.extent = app->swapchainExtent;

        VkRenderPassBeginInfo renderPassBeginInfo_indirectLgt = renderPassBeginInfo;
        renderPassBeginInfo_indirectLgt.renderPass = app->renderPass_indierctLgt;

        VkClearValue clearValues[7] = {
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.depthStencil = {1.0f, 0}}
        };

        renderPassBeginInfo.clearValueCount = app->colorAttachCount + 1;
        renderPassBeginInfo.pClearValues = clearValues;

        renderPassBeginInfo_indirectLgt.clearValueCount = 2;
        renderPassBeginInfo_indirectLgt.pClearValues = clearValues;

        VkBuffer vertexBuffers[1] = { app->vertexPositionBuffer };
        VkDeviceSize offsets[1] = { 0 };

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        if (vkBeginCommandBuffer(app->commandBuffers[x], &commandBufferBeginCreateInfo) == VK_SUCCESS) {
            printf("begin recording command buffer for image #%d\n", x);
        }

        vkCmdBeginRenderPass(app->commandBuffers[x], &renderPassBeginInfo_indirectLgt, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline_indierctLgt);

        vkCmdBindVertexBuffers(app->commandBuffers[x], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(app->commandBuffers[x], app->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 0, 1, &app->rayTraceDescriptorSet, 0, 0);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 1, 1, &app->materialDescriptorSet, 0, 0);

        vkCmdDrawIndexed(app->commandBuffers[x], scene->attributes.num_faces, 1, 0, 0, 0);
        vkCmdEndRenderPass(app->commandBuffers[x]);

        {
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.image = app->swapchainImages[x];
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(app->commandBuffers[x], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
        }

        {
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.image = app->rayTraceImage;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(app->commandBuffers[x], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
        }

        {
            VkImageSubresourceLayers subresourceLayers = {};
            subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceLayers.mipLevel = 0;
            subresourceLayers.baseArrayLayer = 0;
            subresourceLayers.layerCount = 1;

            VkOffset3D offset = {};
            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};
            extent.width = WIDTH;
            extent.height = HEIGHT;
            extent.depth = 1;

            VkImageCopy imageCopy = {};
            imageCopy.srcSubresource = subresourceLayers;
            imageCopy.srcOffset = offset;
            imageCopy.dstSubresource = subresourceLayers;
            imageCopy.dstOffset = offset;
            imageCopy.extent = extent;

            //copy 当前帧 渲染结果 到 Image_indirectLgt
            vkCmdCopyImage(app->commandBuffers[x], app->swapchainImages[x], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->Image_indirectLgt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        }

        {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            imageMemoryBarrier.image = app->swapchainImages[x];
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(app->commandBuffers[x], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
        }

        {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.image = app->rayTraceImage;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(app->commandBuffers[x], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
        }

        // 2th renderpass
        vkCmdBeginRenderPass(app->commandBuffers[x], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline);

        vkCmdBindVertexBuffers(app->commandBuffers[x], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(app->commandBuffers[x], app->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 0, 1, &app->rayTraceDescriptorSet, 0, 0);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 1, 1, &app->materialDescriptorSet, 0, 0);

        vkCmdDrawIndexed(app->commandBuffers[x], scene->attributes.num_faces, 1, 0, 0, 0);
        vkCmdEndRenderPass(app->commandBuffers[x]);

        {
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.image = app->swapchainImages[x];
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(app->commandBuffers[x], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
        }

        {
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.image = app->rayTraceImage;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(app->commandBuffers[x], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
        }

        {
            VkImageSubresourceLayers subresourceLayers = {};
            subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceLayers.mipLevel = 0;
            subresourceLayers.baseArrayLayer = 0;
            subresourceLayers.layerCount = 1;

            VkOffset3D offset = {};
            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};
            extent.width = WIDTH;
            extent.height = HEIGHT;
            extent.depth = 1;

            VkImageCopy imageCopy = {};
            imageCopy.srcSubresource = subresourceLayers;
            imageCopy.srcOffset = offset;
            imageCopy.dstSubresource = subresourceLayers;
            imageCopy.dstOffset = offset;
            imageCopy.extent = extent;

            //copy 当前帧 渲染结果 到 rayTraceImage
            vkCmdCopyImage(app->commandBuffers[x], app->swapchainImages[x], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->rayTraceImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        }

        {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            imageMemoryBarrier.image = app->swapchainImages[x];
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(app->commandBuffers[x], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
        }

        {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = NULL;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.image = app->rayTraceImage;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(app->commandBuffers[x], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
        }

        if (vkEndCommandBuffer(app->commandBuffers[x]) == VK_SUCCESS) {
            printf("end recording command buffer for image #%d\n", x);
        }
    }
}

void VkRayTracingApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void VkRayTracingApplication::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

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

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void VkRayTracingApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(commandBuffer);
}

VkImageView VkRayTracingApplication::createImageView(VkImage image, VkFormat format)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void VkRayTracingApplication::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void VkRayTracingApplication::createCommandBuffers_3pass(VkRayTracingApplication* app, Scene* scene)
{
    app->commandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * app->imageCount);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = app->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = app->imageCount;

    if (vkAllocateCommandBuffers(app->logicalDevice, &commandBufferAllocateInfo, app->commandBuffers) == VK_SUCCESS) {
        printf("allocated command buffers\n");
    }

    for (int x = 0; x < app->imageCount; x++) {
        VkCommandBufferBeginInfo commandBufferBeginCreateInfo = {};
        commandBufferBeginCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = app->renderPass;
        renderPassBeginInfo.framebuffer = app->swapchainFramebuffers[x];    //指定渲染结果保存到这里
        VkOffset2D renderAreaOffset = { 0, 0 };
        renderPassBeginInfo.renderArea.offset = renderAreaOffset;
        renderPassBeginInfo.renderArea.extent = app->swapchainExtent;

        VkRenderPassBeginInfo renderPassBeginInfo_indirectLgt = renderPassBeginInfo;
        renderPassBeginInfo_indirectLgt.renderPass = app->renderPass_indierctLgt;
        //renderPassBeginInfo_indirectLgt.framebuffer = app->GFramebuffersLv0[x];

        VkRenderPassBeginInfo renderPassBeginInfo_indirectLgt_2 = renderPassBeginInfo;
        renderPassBeginInfo_indirectLgt.renderPass = app->renderPass_indierctLgt_2;

        VkClearValue clearValues[7] = {
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
          {.depthStencil = {1.0f, 0}}
        };

        renderPassBeginInfo.clearValueCount = app->colorAttachCount + 1;
        renderPassBeginInfo.pClearValues = clearValues;

        renderPassBeginInfo_indirectLgt.clearValueCount = app->colorAttachCount + 1;
        renderPassBeginInfo_indirectLgt.pClearValues = clearValues;

        renderPassBeginInfo_indirectLgt_2.clearValueCount = app->colorAttachCount + 1;
        renderPassBeginInfo_indirectLgt_2.pClearValues = clearValues;

        VkBuffer vertexBuffers[1] = { app->vertexPositionBuffer };
        VkDeviceSize offsets[1] = { 0 };

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        if (vkBeginCommandBuffer(app->commandBuffers[x], &commandBufferBeginCreateInfo) == VK_SUCCESS) {
            printf("begin recording command buffer for image #%d\n", x);
        }

        vkCmdBeginRenderPass(app->commandBuffers[x], &renderPassBeginInfo_indirectLgt, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline_indierctLgt);

        vkCmdBindVertexBuffers(app->commandBuffers[x], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(app->commandBuffers[x], app->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 0, 1, &app->rayTraceDescriptorSet, 0, 0);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 1, 1, &app->materialDescriptorSet, 0, 0);

        vkCmdDrawIndexed(app->commandBuffers[x], scene->attributes.num_faces, 1, 0, 0, 0);
        vkCmdEndRenderPass(app->commandBuffers[x]);

        {
            VkImageSubresourceLayers subresourceLayers = {};
            subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceLayers.mipLevel = 0;
            subresourceLayers.baseArrayLayer = 0;
            subresourceLayers.layerCount = 1;

            VkOffset3D offset = {};
            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};
            extent.width = WIDTH;
            extent.height = HEIGHT;
            extent.depth = 1;

            VkImageCopy imageCopy = {};
            imageCopy.srcSubresource = subresourceLayers;
            imageCopy.srcOffset = offset;
            imageCopy.dstSubresource = subresourceLayers;
            imageCopy.dstOffset = offset;
            imageCopy.extent = extent;

            //copy 当前帧 渲染结果 到 Image_indirectLgt
            vkCmdCopyImage(app->commandBuffers[x], app->swapchainImages[x], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->HDirectAlbedoImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->GDirectIrradImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->HDirectIrradImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->GIrradImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->Image_indirectLgt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->G_albedoImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->Image_indirectLgt_2, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->GnormalImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->HNormalImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->GWorldPosImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->HWorldPosImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->depthImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->HDepthImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        }

        

        // 2th renderpass
        vkCmdBeginRenderPass(app->commandBuffers[x], &renderPassBeginInfo_indirectLgt_2, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline_indierctLgt_2);

        vkCmdBindVertexBuffers(app->commandBuffers[x], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(app->commandBuffers[x], app->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 0, 1, &app->rayTraceDescriptorSet, 0, 0);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 1, 1, &app->materialDescriptorSet, 0, 0);

        vkCmdDrawIndexed(app->commandBuffers[x], scene->attributes.num_faces, 1, 0, 0, 0);
        vkCmdEndRenderPass(app->commandBuffers[x]);

        {
            VkImageSubresourceLayers subresourceLayers = {};
            subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceLayers.mipLevel = 0;
            subresourceLayers.baseArrayLayer = 0;
            subresourceLayers.layerCount = 1;

            VkOffset3D offset = {};
            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};
            extent.width = WIDTH;
            extent.height = HEIGHT;
            extent.depth = 1;

            VkImageCopy imageCopy = {};
            imageCopy.srcSubresource = subresourceLayers;
            imageCopy.srcOffset = offset;
            imageCopy.dstSubresource = subresourceLayers;
            imageCopy.dstOffset = offset;
            imageCopy.extent = extent;

            //copy 当前帧 渲染结果 到 Image_indirectLgt
            vkCmdCopyImage(app->commandBuffers[x], app->swapchainImages[x], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->HVarImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->GDirectIrradImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->HDirectIrradImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->GIrradImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->Image_indirectLgt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
            vkCmdCopyImage(app->commandBuffers[x], app->G_albedoImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->Image_indirectLgt_2, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        }

        //3th pass
        vkCmdBeginRenderPass(app->commandBuffers[x], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline);

        vkCmdBindVertexBuffers(app->commandBuffers[x], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(app->commandBuffers[x], app->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 0, 1, &app->rayTraceDescriptorSet, 0, 0);
        vkCmdBindDescriptorSets(app->commandBuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipelineLayout, 1, 1, &app->materialDescriptorSet, 0, 0);

        vkCmdDrawIndexed(app->commandBuffers[x], scene->attributes.num_faces, 1, 0, 0, 0);
        vkCmdEndRenderPass(app->commandBuffers[x]);


        {
            VkImageSubresourceLayers subresourceLayers = {};
            subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceLayers.mipLevel = 0;
            subresourceLayers.baseArrayLayer = 0;
            subresourceLayers.layerCount = 1;

            VkOffset3D offset = {};
            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};
            extent.width = WIDTH;
            extent.height = HEIGHT;
            extent.depth = 1;

            VkImageCopy imageCopy = {};
            imageCopy.srcSubresource = subresourceLayers;
            imageCopy.srcOffset = offset;
            imageCopy.dstSubresource = subresourceLayers;
            imageCopy.dstOffset = offset;
            imageCopy.extent = extent;

            //copy 当前帧 渲染结果 到 rayTraceImage
            vkCmdCopyImage(app->commandBuffers[x], app->swapchainImages[x], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->rayTraceImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        }


        if (vkEndCommandBuffer(app->commandBuffers[x]) == VK_SUCCESS) {
            printf("end recording command buffer for image #%d\n", x);
        }
    }
}

void VkRayTracingApplication::createSynchronizationObjects(VkRayTracingApplication* app)
{
    app->imageAvailableSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    app->renderFinishedSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    app->inFlightFences = (VkFence*)malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
    app->imagesInFlight = (VkFence*)malloc(sizeof(VkFence) * app->imageCount);
    for (int x = 0; x < app->imageCount; x++) {
        app->imagesInFlight[x] = VK_NULL_HANDLE;
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
        if (vkCreateSemaphore(app->logicalDevice, &semaphoreCreateInfo, NULL, &app->imageAvailableSemaphores[x]) == VK_SUCCESS &&
            vkCreateSemaphore(app->logicalDevice, &semaphoreCreateInfo, NULL, &app->renderFinishedSemaphores[x]) == VK_SUCCESS &&
            vkCreateFence(app->logicalDevice, &fenceCreateInfo, NULL, &app->inFlightFences[x]) == VK_SUCCESS) {
            printf("created synchronization objects for frame #%d\n", x);
        }
    }
}

void VkRayTracingApplication::updateUniformBuffer(VkRayTracingApplication* app, Camera* camera,ShadingMode* shadingMode)
{
    void* data;
    vkMapMemory(app->logicalDevice, app->uniformBufferMemory, 0, sizeof(Camera), 0, &data);
    memcpy(data, camera, sizeof(struct Camera));
    vkUnmapMemory(app->logicalDevice, app->uniformBufferMemory);

    void* data_s;
    vkMapMemory(app->logicalDevice, app->uniformBufferMemory_shadingMode, 0, sizeof(ShadingMode), 0, &data_s);
    memcpy(data_s, shadingMode, sizeof(ShadingMode));
    vkUnmapMemory(app->logicalDevice, app->uniformBufferMemory_shadingMode);
}

void VkRayTracingApplication::drawFrame(VkRayTracingApplication* app, Camera* camera, ShadingMode* shadingMode)
{
    vkWaitForFences(app->logicalDevice, 1, &app->inFlightFences[app->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex=0;
    vkAcquireNextImageKHR(app->logicalDevice, app->swapchain, UINT64_MAX, app->imageAvailableSemaphores[app->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (app->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(app->logicalDevice, 1, &app->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    app->imagesInFlight[imageIndex] = app->inFlightFences[app->currentFrame];

    //计算 Previous frame's matrix
    glm::vec4 positionVector = camera->position - glm::vec4(0.0, 0.0, 0.0, 1.0);
    //glm::mat4 viewMatrix = lookAt(positionVector, positionVector + camera->forward, camera->up);
    
    glm::mat4 viewMatrix = {
    glm::vec4(camera->right[0], camera->up[0], camera->forward[0], 0),
    glm::vec4(camera->right[1], camera->up[1], camera->forward[1], 0),
    glm::vec4(camera->right[2], camera->up[2], camera->forward[2], 0),
    
    glm::vec4(-dot(camera->right, positionVector), -dot(camera->up, positionVector), -dot(camera->forward, positionVector), 1)
    };
    //shadingMode->invViewMatrix = glm::inverse(viewMatrix);
    //TODO:改成glm：：lookat
    camera->viewMatrix = viewMatrix;

    float farDist = 1000.0;
    float nearDist = 0.0001;
    float frustumDepth = farDist - nearDist;
    float oneOverDepth = 1.0 / frustumDepth;
    float fov = 1.0472;
    float aspect = 3840 / 2160;

    glm::mat4 projectionMatrix = {
      glm::vec4(1.0 / tan(0.5f * fov) / aspect, 0, 0, 0),
      glm::vec4(0, 1.0 / tan(0.5f * fov), 0, 0),
      glm::vec4(0, 0, farDist * oneOverDepth, 1),
      glm::vec4(0, 0, (-farDist * nearDist) * oneOverDepth, 0)
    };
    //shadingMode->invProjMatrix = glm::inverse(projectionMatrix);
    camera->projMatrix = projectionMatrix;
    
    updateUniformBuffer(app, camera,shadingMode);

    shadingMode->PrevViewMatrix = viewMatrix;
    shadingMode->PrevProjectionMatrix = projectionMatrix;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[1] = { app->imageAvailableSemaphores[app->currentFrame] };
    VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &app->commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[1] = { app->renderFinishedSemaphores[app->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(app->logicalDevice, 1, &app->inFlightFences[app->currentFrame]);

    VkResult errorCode = vkQueueSubmit(app->graphicsQueue, 1, &submitInfo, app->inFlightFences[app->currentFrame]);
    if (errorCode != VK_SUCCESS) {
        printf("failed to submit draw command buffer\n");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[1] = { app->swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(app->presentQueue, &presentInfo);

    app->currentFrame = (app->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    
}

void VkRayTracingApplication::createDepthResources(VkRayTracingApplication* app)
{
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    createImage(app, app->swapchainExtent.width, app->swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &app->depthImage, &app->depthImageMemory);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = app->depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(app->logicalDevice, &viewInfo, NULL, &app->depthImageView) == VK_SUCCESS) {
        printf("created texture image view\n");
    }
}

void VkRayTracingApplication::createFramebuffers(VkRayTracingApplication* app)
{
    app->swapchainFramebuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer*) * app->imageCount);

    for (int x = 0; x < app->imageCount; x++) {
        VkImageView attachments[7] = {
          app->swapchainImageViews[x],
          app->GDirectIrradImageView,
          app->G_albedoImageView,
          app->GIrradImageView,
          app->GnormalImageView,
          app->GWorldPosImageView,
          app->depthImageView
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = app->renderPass;
        framebufferCreateInfo.attachmentCount = app->colorAttachCount+1;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = app->swapchainExtent.width;
        framebufferCreateInfo.height = app->swapchainExtent.height;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(app->logicalDevice, &framebufferCreateInfo, NULL, &app->swapchainFramebuffers[x]) == VK_SUCCESS) {
            printf("created swapchain framebuffer #%d\n", x);
        }
    }
}

void VkRayTracingApplication::createGraphicsPipeline(VkRayTracingApplication* app)
{

    Shader* vertex_shader=new Shader();
    //vertex_shader->load(app,"shaders/basic.vert.spv");
#ifdef _DEBUG
    //vertex_shader->load(app, "C:/Users/Rocki/source/repos/VulkanBasicRayTracing/LearnVulkan/shaders/basic.vert.spv");
    vertex_shader->load(app, "C:/Users/Vincent/source/repos/VulkanRayTracing/LearnVulkan/shaders/basic.vert.spv");

#else
    vertex_shader->load(app, "shaders/basic.vert.spv");
#endif // DEBUG

    Shader* frag_shader = new Shader();
    //frag_shader->load(app, "shaders/basic.frag.spv");
#ifdef _DEBUG
    frag_shader->load(app, "C:/Users/Vincent/source/repos/VulkanRayTracing/LearnVulkan/shaders/basic.frag.spv");
    //frag_shader->load(app, "C:/Users/Rocki/source/repos/VulkanBasicRayTracing/LearnVulkan/shaders/basic.frag.spv");

#else
    frag_shader->load(app, "shaders/basic.frag.spv");
#endif // DEBUG

    VkPipelineShaderStageCreateInfo shaderStages[2] = { vertex_shader->ShaderStageInfo, frag_shader->ShaderStageInfo };

    app->vertexBindingDescriptions = (VkVertexInputBindingDescription*)malloc(sizeof(VkVertexInputBindingDescription) * 1);
    app->vertexBindingDescriptions[0].binding = 0;
    app->vertexBindingDescriptions[0].stride = sizeof(float) * 3;
    app->vertexBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    app->vertexAttributeDescriptions = (VkVertexInputAttributeDescription*)malloc(sizeof(VkVertexInputAttributeDescription) * 1);
    app->vertexAttributeDescriptions[0].binding = 0;
    app->vertexAttributeDescriptions[0].location = 0;
    app->vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    app->vertexAttributeDescriptions[0].offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = app->vertexBindingDescriptions;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = app->vertexAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = (float)app->swapchainExtent.height;
    viewport.width = (float)app->swapchainExtent.width;
    viewport.height = -(float)app->swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    VkOffset2D scissorOffset = { 0, 0 };
    scissor.offset = scissorOffset;
    scissor.extent = app->swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    //VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    //colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    //colorBlendAttachmentState.blendEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState* pColorBlendAttachmentState = (VkPipelineColorBlendAttachmentState*)malloc(app->colorAttachCount * sizeof(VkPipelineColorBlendAttachmentState));
    for (auto i = 0; i < app->colorAttachCount; i++) {
        pColorBlendAttachmentState[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pColorBlendAttachmentState[i].blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount = app->colorAttachCount;                //Num of color attachment
    colorBlendStateCreateInfo.pAttachments = pColorBlendAttachmentState;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 2;
    pipelineLayoutCreateInfo.pSetLayouts = app->rayTraceDescriptorSetLayouts;

    if (vkCreatePipelineLayout(app->logicalDevice, &pipelineLayoutCreateInfo, NULL, &app->pipelineLayout) == VK_SUCCESS) {
        printf("created pipeline layout\n");
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.layout = app->pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = app->renderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult errorCode;
    try {
        errorCode = vkCreateGraphicsPipelines(app->logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &app->graphicsPipeline);
    }
    catch (...) {
        cout << "unknow e";
    }
    
    if ( errorCode == VK_SUCCESS) {
        printf("created graphics pipeline\n");
    }


    delete vertex_shader;
    delete frag_shader;
}

void VkRayTracingApplication::createGraphicsPipeline_indirectLgt(VkRayTracingApplication* app)
{

    Shader* vertex_shader = new Shader();
    //vertex_shader->load(app, "shaders/basic.vert.spv");
#ifdef _DEBUG
    //vertex_shader->load(app, "C:/Users/Rocki/source/repos/VulkanBasicRayTracing/LearnVulkan/shaders/basic.vert.spv");
    vertex_shader->load(app, "C:/Users/Vincent/source/repos/VulkanRayTracing/LearnVulkan/shaders/basic.vert.spv");

#else
    vertex_shader->load(app, "shaders/basic.vert.spv");
#endif // DEBUG


    Shader* frag_shader = new Shader();
    //frag_shader->load(app, "shaders/basic_indirectLgt.frag.spv");
#ifdef _DEBUG
    //frag_shader->load(app, "C:/Users/Rocki/source/repos/VulkanBasicRayTracing/LearnVulkan/shaders/basic_indirectLgt.frag.spv");
    frag_shader->load(app, "C:/Users/Vincent/source/repos/VulkanRayTracing/LearnVulkan/shaders/basic_indirectLgt.frag.spv");
#else
    frag_shader->load(app, "shaders/basic_indirectLgt.frag.spv");
#endif // DEBUG

    VkPipelineShaderStageCreateInfo shaderStages[2] = { vertex_shader->ShaderStageInfo, frag_shader->ShaderStageInfo };

    app->vertexBindingDescriptions = (VkVertexInputBindingDescription*)malloc(sizeof(VkVertexInputBindingDescription) * 1);
    app->vertexBindingDescriptions[0].binding = 0;
    app->vertexBindingDescriptions[0].stride = sizeof(float) * 3;
    app->vertexBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    app->vertexAttributeDescriptions = (VkVertexInputAttributeDescription*)malloc(sizeof(VkVertexInputAttributeDescription) * 1);
    app->vertexAttributeDescriptions[0].binding = 0;
    app->vertexAttributeDescriptions[0].location = 0;
    app->vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    app->vertexAttributeDescriptions[0].offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = app->vertexBindingDescriptions;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = app->vertexAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = (float)app->swapchainExtent.height;
    viewport.width = (float)app->swapchainExtent.width;
    viewport.height = -(float)app->swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    VkOffset2D scissorOffset = { 0, 0 };
    scissor.offset = scissorOffset;
    scissor.extent = app->swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState* pColorBlendAttachmentState = (VkPipelineColorBlendAttachmentState*)malloc(app->colorAttachCount * sizeof(VkPipelineColorBlendAttachmentState));
    for (auto i = 0; i < app->colorAttachCount; i++) {
        pColorBlendAttachmentState[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pColorBlendAttachmentState[i].blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount = app->colorAttachCount;
    colorBlendStateCreateInfo.pAttachments = pColorBlendAttachmentState;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 2;
    pipelineLayoutCreateInfo.pSetLayouts = app->rayTraceDescriptorSetLayouts;

    if (vkCreatePipelineLayout(app->logicalDevice, &pipelineLayoutCreateInfo, NULL, &app->pipelineLayout_indierctLgt) == VK_SUCCESS) {
        printf("created pipeline layout\n");
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.layout = app->pipelineLayout_indierctLgt;
    graphicsPipelineCreateInfo.renderPass = app->renderPass_indierctLgt;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult errorCode;
    try {
        errorCode = vkCreateGraphicsPipelines(app->logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &app->graphicsPipeline_indierctLgt);
    }
    catch (...) {
        cout << "unknow e";
    }

    if (errorCode == VK_SUCCESS) {
        printf("created graphics pipeline_indirectLgt\n");
    }


    delete vertex_shader;
    delete frag_shader;
}

void VkRayTracingApplication::createGraphicsPipeline_indirectLgt_2(VkRayTracingApplication* app)
{

    Shader* vertex_shader = new Shader();
    //vertex_shader->load(app, "shaders/basic.vert.spv");
#ifdef _DEBUG
    //vertex_shader->load(app, "C:/Users/Rocki/source/repos/VulkanBasicRayTracing/LearnVulkan/shaders/basic.vert.spv");
    vertex_shader->load(app, "C:/Users/Vincent/source/repos/VulkanRayTracing/LearnVulkan/shaders/basic.vert.spv");
#else
    vertex_shader->load(app, "shaders/basic.vert.spv");
#endif // DEBUG


    Shader* frag_shader = new Shader();
    //frag_shader->load(app, "shaders/basic_indirectLgt.frag.spv");
#ifdef _DEBUG
    //frag_shader->load(app, "C:/Users/Rocki/source/repos/VulkanBasicRayTracing/LearnVulkan/shaders/basic_filterIndirect.frag.spv");
    frag_shader->load(app, "C:/Users/Vincent/source/repos/VulkanRayTracing/LearnVulkan/shaders/basic_filterIndirect.frag.spv");
#else
    frag_shader->load(app, "shaders/basic_filterIndirect.frag.spv");
#endif // DEBUG

    VkPipelineShaderStageCreateInfo shaderStages[2] = { vertex_shader->ShaderStageInfo, frag_shader->ShaderStageInfo };

    app->vertexBindingDescriptions = (VkVertexInputBindingDescription*)malloc(sizeof(VkVertexInputBindingDescription) * 1);
    app->vertexBindingDescriptions[0].binding = 0;
    app->vertexBindingDescriptions[0].stride = sizeof(float) * 3;
    app->vertexBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    app->vertexAttributeDescriptions = (VkVertexInputAttributeDescription*)malloc(sizeof(VkVertexInputAttributeDescription) * 1);
    app->vertexAttributeDescriptions[0].binding = 0;
    app->vertexAttributeDescriptions[0].location = 0;
    app->vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    app->vertexAttributeDescriptions[0].offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = app->vertexBindingDescriptions;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = app->vertexAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = (float)app->swapchainExtent.height;
    viewport.width = (float)app->swapchainExtent.width;
    viewport.height = -(float)app->swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    VkOffset2D scissorOffset = { 0, 0 };
    scissor.offset = scissorOffset;
    scissor.extent = app->swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState* pColorBlendAttachmentState = (VkPipelineColorBlendAttachmentState*)malloc(app->colorAttachCount * sizeof(VkPipelineColorBlendAttachmentState));
    for (auto i = 0; i < app->colorAttachCount; i++) {
        pColorBlendAttachmentState[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pColorBlendAttachmentState[i].blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount = app->colorAttachCount;
    colorBlendStateCreateInfo.pAttachments = pColorBlendAttachmentState;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 2;
    pipelineLayoutCreateInfo.pSetLayouts = app->rayTraceDescriptorSetLayouts;

    if (vkCreatePipelineLayout(app->logicalDevice, &pipelineLayoutCreateInfo, NULL, &app->pipelineLayout_indierctLgt) == VK_SUCCESS) {
        printf("created pipeline layout\n");
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.layout = app->pipelineLayout_indierctLgt;
    graphicsPipelineCreateInfo.renderPass = app->renderPass_indierctLgt;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult errorCode;
    try {
        errorCode = vkCreateGraphicsPipelines(app->logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &app->graphicsPipeline_indierctLgt_2);
    }
    catch (...) {
        cout << "unknow e";
    }

    if (errorCode == VK_SUCCESS) {
        printf("created graphics pipeline_indirectLgt_2\n");
    }


    delete vertex_shader;
    delete frag_shader;
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
    //make viewport state from our stored viewport and scissor.
            //at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &_viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &_scissor;

    //setup dummy color blending. We aren't using transparent objects yet
    //the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &_colorBlendAttachment;

    //build the actual pipeline
    //we now use all of the info structs we have been writing into into this one to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;

    pipelineInfo.stageCount = _shaderStages.size();
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDepthStencilState = &_depthStencil;

    //it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        std::cout << "failed to create pipeline\n";
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    }
    else
    {
        return newPipeline;
    }
}

void VkRayTracingApplication::load_meshes(Scene* scene, std::string fileName) {

    //load the monkey
    //_monkeyMesh.load_from_obj("../../assets/monkey_smooth.obj");
    _monkeyMesh.load_from_obj(fileName);

//**********************trans***************************************************************
{
    scene->attributes.num_vertices = _monkeyMesh.attrib.vertices.size() / 3;
    scene->attributes.vertices = _monkeyMesh.attrib.vertices.data();
    scene->attributes.num_normals = _monkeyMesh.attrib.normals.size() / 3;
    scene->attributes.normals = _monkeyMesh.attrib.normals.data();
    scene->attributes.num_texcoords = _monkeyMesh.attrib.texcoords.size() / 2;
    scene->attributes.texcoords = _monkeyMesh.attrib.texcoords.data();

    scene->attributes.num_faces = 0;               //calculate total indices （not real face ,shit c-version api)
    scene->attributes.num_face_num_verts = 0;
    vector<unsigned int> indices;
    vector<unsigned int> face_num_verts;
    vector<unsigned int> materialId;
    for (auto i = _monkeyMesh.shapes.begin(); i != _monkeyMesh.shapes.end(); i++) {
        scene->attributes.num_faces += (*i).mesh.indices.size();
        scene->attributes.num_face_num_verts += (*i).mesh.num_face_vertices.size();
        for (auto j = 0; j < (*i).mesh.indices.size(); j++) {
            indices.push_back((*i).mesh.indices[j].vertex_index);
        }
        for (auto j = 0; j < (*i).mesh.num_face_vertices.size(); j++) {
            face_num_verts.push_back((*i).mesh.num_face_vertices[j]);
            materialId.push_back((*i).mesh.material_ids[j]);
        }
    }
    scene->attributes.faces = (tinyobj_vertex_index_t*)malloc(scene->attributes.num_faces*sizeof(tinyobj_vertex_index_t));
    scene->attributes.face_num_verts = (int*)malloc(scene->attributes.num_face_num_verts * sizeof(int));
    scene->attributes.material_ids = (int*)malloc(scene->attributes.num_face_num_verts * sizeof(int));
    for (auto i = 0; i < scene->attributes.num_faces; i++) {
        scene->attributes.faces[i].v_idx = indices[i];
    }
    for (auto i = 0; i < scene->attributes.num_face_num_verts; i++) {
        scene->attributes.face_num_verts[i] = face_num_verts[i];
        scene->attributes.material_ids[i] = materialId[i];
    }
    //for materials
    scene->numMaterials = _monkeyMesh.materials.size();
    scene->materials = (tinyobj_material_t*)malloc(scene->numMaterials*sizeof(tinyobj_material_t));
    for (auto i = 0; i < scene->numMaterials; i++) {
        memcpy(scene->materials[i].ambient, _monkeyMesh.materials[i].ambient, sizeof(float) * 3);
        memcpy(scene->materials[i].diffuse, _monkeyMesh.materials[i].diffuse, sizeof(float) * 3);
        memcpy(scene->materials[i].specular, _monkeyMesh.materials[i].specular, sizeof(float) * 3);
        memcpy(scene->materials[i].emission, _monkeyMesh.materials[i].emission, sizeof(float) * 3);
    }

}
//**********************trans***************************************************************

    //make sure both meshes are sent to the GPU
   upload_mesh(_monkeyMesh);

   std::unordered_map<Vertex, uint32_t> uniqueVertices{};
   for (const auto& shape : _monkeyMesh.shapes) {
       for (const auto& index : shape.mesh.indices) {
           Vertex vertex{};

           vertex.position = {
                _monkeyMesh.attrib.vertices[3 * index.vertex_index + 0],
                _monkeyMesh.attrib.vertices[3 * index.vertex_index + 1],
                _monkeyMesh.attrib.vertices[3 * index.vertex_index + 2]
           };
           vertex.normal = {
               _monkeyMesh.attrib.normals[3 * index.normal_index + 0],
               _monkeyMesh.attrib.normals[3 * index.normal_index + 1],
               _monkeyMesh.attrib.normals[3 * index.normal_index + 2]
           };
           vertex.texCoord = {   //翻转
               _monkeyMesh.attrib.texcoords[2 * index.texcoord_index + 0],
               1.0f - _monkeyMesh.attrib.texcoords[2 * index.texcoord_index + 1]
           };

           vertex.color = { 1.0f, 1.0f, 1.0f };

           if (uniqueVertices.count(vertex) == 0) {
               uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
               vertices.push_back(vertex);
           }

           indices.push_back(uniqueVertices[vertex]);
       }
   }

   for (const auto& mat : _monkeyMesh.materials) {
       Material m;
       memcpy(m.ambient, mat.ambient, sizeof(float) * 3);
       memcpy(m.diffuse, mat.diffuse, sizeof(float) * 3);
       memcpy(m.specular, mat.specular, sizeof(float) * 3);
       memcpy(m.emission, mat.emission, sizeof(float) * 3);
       materials.push_back(m);
   }
    //note that we are copying them. Eventually we will delete the hardcoded _monkey and _triangle meshes, so it's no problem now.
    _meshes["monkey"] = _monkeyMesh;
}

void VkRayTracingApplication::createTextureImage(VkRayTracingApplication* app)
{
    int texWidth, texHeight, texChannels;
    std::string str = GetExePath();
    str += "\\data\\viking_room.png";
    stbi_uc* pixels = stbi_load(str.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(app,imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
    void* data;
    vkMapMemory(app->logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(app->logicalDevice, stagingBufferMemory);
    stbi_image_free(pixels);

    createImage(app,texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &textureImage, &textureImageMemory);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void VkRayTracingApplication::createTextureImageView()
{
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}

VkCommandBuffer VkRayTracingApplication::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VkRayTracingApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void VkRayTracingApplication::upload_mesh(Mesh& mesh) {
    //allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    //this is the total size, in bytes, of the buffer we are allocating
    bufferInfo.size = mesh._vertices.size() * sizeof(Vertex);
    //this buffer is going to be used as a Vertex Buffer
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;


    //let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    //allocate the buffer
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
        &mesh._vertexBuffer._buffer,
        &mesh._vertexBuffer._allocation,
        nullptr));

    //add the destruction of triangle mesh buffer to the deletion queue
    _mainDeletionQueue.push_function([=]() {

        vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
        });

    //copy vertex data
    void* data;
    vmaMapMemory(_allocator, mesh._vertexBuffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(_allocator, mesh._vertexBuffer._allocation);
}

Material_pipeline* VkRayTracingApplication::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
    Material_pipeline mat;
    mat.pipeline = pipeline;
    mat.pipelineLayout = layout;
    _materials[name] = mat;
    return &_materials[name];
}

Material_pipeline* VkRayTracingApplication::get_material(const std::string& name)
{
    //search for the object, and return nullptr if not found
    auto it = _materials.find(name);
    if (it == _materials.end()) {
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}

Mesh* VkRayTracingApplication::get_mesh(const std::string& name) {
    auto it = _meshes.find(name);
    if (it == _meshes.end()) {
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}

void VkRayTracingApplication::draw_objects(VkCommandBuffer cmd, RenderObject* first, int count) {
    //make a model view matrix for rendering the object
    //camera view
    glm::vec3 camPos = { 0.f,-6.f,-10.f };

    glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
    //camera projection
    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    Mesh* lastMesh = nullptr;
    Material_pipeline* lastMaterial = nullptr;
    for (int i = 0; i < count; i++)
    {
        RenderObject& object = first[i];

        //only bind the pipeline if it doesn't match with the already bound one
        if (object.material != lastMaterial) {

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
        }


        glm::mat4 model = object.transformMatrix;
        //final render matrix, that we are calculating on the cpu
        glm::mat4 mesh_matrix = projection * view * model;

        MeshPushConstants constants;
        constants.render_matrix = mesh_matrix;

        //upload the mesh to the GPU via push constants
        vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        //only bind the mesh if it's a different one from last bind
        if (object.mesh != lastMesh) {
            //bind the mesh vertex buffer with offset 0
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
            lastMesh = object.mesh;
        }
        //we can now draw
        vkCmdDraw(cmd, object.mesh->_vertices.size(), 1, 0, 0);
    }
}

ShadingMode::ShadingMode()
{
    enable2thRay = 1;
    enableSVGF = 0;
    enableShadowMotion = 0;
    enableSVGF_withIndAlbedo = 0;
    enableShadowMotion = 0;
    enable2thRMotion = 0;
}
