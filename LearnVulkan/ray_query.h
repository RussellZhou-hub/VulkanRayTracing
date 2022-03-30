#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define SEEK_END 2
#define VK_ENABLE_BETA_EXTENSIONS
#define VKB_VALIDATION_LAYERS
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
//#define TINYOBJLOADER_IMPLEMENTATION

//const uint32_t WIDTH = 1920;
//const uint32_t HEIGHT = 1080;
const uint32_t WIDTH = 3840/2;
const uint32_t HEIGHT = 2160/2;



#define M_PI 3.141592658f

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include<vector>
#include<string>
#include<set>

//#include"App.h"
//extern "C" {
//#include "tinyobj_loader_c.h"
//}

#include "tinyobj_loader_c.h"

#define MAX_FRAMES_IN_FLIGHT      1
#define ENABLE_VALIDATION         1

//#include "tinyobj_loader_c.c"
//#include "tiny_obj_loader.h"

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_RAY_QUERY_EXTENSION_NAME
};

static char keyDownIndex[500];

static float cameraPosition[3] = {1.46f,3.20f,9.26f};
static float cameraYaw=0.0f;
static float cameraPitch=-0.0f;

/*
Vulkan expects the data in your structure to be aligned in memory in a specific way, for example:

Scalars have to be aligned by N (= 4 bytes given 32 bit floats).
A vec2 must be aligned by 2N (= 8 bytes)
A vec3 or vec4 must be aligned by 4N (= 16 bytes)
A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
A mat4 matrix must have the same alignment as a vec4.
You can find the full list of alignment requirements in https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout
*/

class Camera {
public:
	Camera();
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	float position[4];
	float right[4];
	float up[4];
	float forward[4];

	glm::vec4 lightA;
	glm::vec4 lightB;
	glm::vec4 lightC;
	glm::vec4 lightPad;  //for padding

	uint32_t frameCount;
	uint32_t ViewPortWidth;
	uint32_t ViewPortHeight;
};

class ShadingMode {
public:
	ShadingMode();
	//glm::mat4 invViewMatrix;
	//glm::mat4 invProjMatrix;
	glm::mat4 PrevViewMatrix;
	glm::mat4 PrevProjectionMatrix;
	uint32_t enable2thRay;
	uint32_t enableShadowMotion;
	uint32_t enableMeanDiff;
	uint32_t enable2thRMotion;
	uint32_t enable2thRayDierctionSpatialFilter;
};

struct Material {
	float ambient[3]; int padA;
	float diffuse[3]; int padB;
	float specular[3]; int padC;
	float emission[3]; int padD;
};



class Scene {
public:
	tinyobj_attrib_t attributes;
	tinyobj_shape_t* shapes;
	tinyobj_material_t* materials;

	uint64_t numShapes;
	uint64_t numMaterials;
};

class VkRayTracingApplication {
public:
	friend class Shader;
	VkRayTracingApplication();
	void run(Scene& scene, Camera& camera, ShadingMode& shadingMode);
private:
	//void loadModel(Scene* scene);
	void initializeScene(Scene* scene, const char* fileNameOBJ);
	void initVulkan(Scene* scene);
	void mainLoop(VkRayTracingApplication* app, Camera* camera, ShadingMode* shadingMode);
	void cleanup(VkRayTracingApplication* app, Scene* scene);
	void createBuffer(VkRayTracingApplication* app, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
	void copyBuffer(VkRayTracingApplication* app, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createImage(VkRayTracingApplication* app, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage* image, VkDeviceMemory* imageMemory);
	void initializeVulkanContext(VkRayTracingApplication* app);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(const VkPhysicalDevice device);
	void pickPhysicalDevice(VkRayTracingApplication* app);
	void createLogicalConnection(VkRayTracingApplication* app);
	void createSwapchain(VkRayTracingApplication* app);
	void createRenderPass(VkRayTracingApplication* app);
	void createCommandPool(VkRayTracingApplication* app);
	void createDepthResources(VkRayTracingApplication* app);
	void createFramebuffers(VkRayTracingApplication* app);  
	void createVertexBuffer(VkRayTracingApplication* app, Scene* scene);
	void createIndexBuffer(VkRayTracingApplication* app, Scene* scene);
	void createMaterialsBuffer(VkRayTracingApplication* app, Scene* scene);
	void createTextures(VkRayTracingApplication* app);
	void createBottomLevelAccelerationStructure(VkRayTracingApplication* app, Scene* scene);
	void createTopLevelAccelerationStructure(VkRayTracingApplication* app);
	//Descriptor Setup
	void createUniformBuffer(VkRayTracingApplication* app);
	void createDescriptorSets(VkRayTracingApplication* app);
	void createGraphicsPipeline(VkRayTracingApplication* app);//Rasterization pipeline
	void createGraphicsPipeline_indirectLgt(VkRayTracingApplication* app);//Rasterization pipeline_indirectLgt
	void createGraphicsPipeline_indirectLgt_2(VkRayTracingApplication* app);//Rasterization pipeline_indirectLgt_2
	void createRayTracePipeline(VkRayTracingApplication* app);
	void createShaderBindingTable(VkRayTracingApplication* app);
	void createCommandBuffers(VkRayTracingApplication* app, Scene* scene);
	void createCommandBuffers_2pass(VkRayTracingApplication* app, Scene* scene);
	void createCommandBuffers_3pass(VkRayTracingApplication* app, Scene* scene);
	void createSynchronizationObjects(VkRayTracingApplication* app);
	void updateUniformBuffer(VkRayTracingApplication* app, Camera* camera, ShadingMode* shadingMode);
	void drawFrame(VkRayTracingApplication* app, struct Camera* camera, ShadingMode* shadingMode);

	int isCameraMoved;

	GLFWwindow* window;
	VkSurfaceKHR surface;
	VkInstance instance;

	VkDebugUtilsMessengerEXT debugMessenger;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	uint32_t graphicsQueueIndex;
	uint32_t presentQueueIndex;
	uint32_t computeQueueIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;

	uint32_t imageCount;
	VkSwapchainKHR swapchain;
	VkImage* swapchainImages;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	VkImageView* swapchainImageViews;
	VkFramebuffer* swapchainFramebuffers;

	VkImage GnormalImage;
	VkImageView GnormalImageView;
	VkDeviceMemory GnormalImageMemory;
	VkImage GDirectImage;
	VkImageView GDirectImageView;
	VkDeviceMemory GDirectImageMemory;
	VkImage GDepthImage;
	VkImageView GDepthImageView;
	VkDeviceMemory GDepthImageMemory;
	VkFramebuffer* GFramebuffersLv0;   //for geometry buffer

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkRenderPass renderPass_indierctLgt;
	VkPipelineLayout pipelineLayout_indierctLgt;
	VkPipeline graphicsPipeline_indierctLgt;

	VkRenderPass renderPass_indierctLgt_2;
	VkPipelineLayout pipelineLayout_indierctLgt_2;
	VkPipeline graphicsPipeline_indierctLgt_2;

	

	VkCommandPool commandPool;
	VkCommandBuffer* commandBuffers;

	VkSemaphore* imageAvailableSemaphores;
	VkSemaphore* renderFinishedSemaphores;
	VkFence* inFlightFences;
	VkFence* imagesInFlight;
	uint32_t currentFrame;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	VkBuffer uniformBuffer_shadingMode;
	VkDeviceMemory uniformBufferMemory_shadingMode;

	VkVertexInputBindingDescription* vertexBindingDescriptions;
	VkVertexInputAttributeDescription* vertexAttributeDescriptions;

	VkPhysicalDeviceMemoryProperties memoryProperties;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkBuffer vertexPositionBuffer;
	VkDeviceMemory vertexPositionBufferMemory;

	VkBuffer materialIndexBuffer;
	VkDeviceMemory materialIndexBufferMemory;

	VkBuffer materialBuffer;
	VkDeviceMemory materialBufferMemory;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkAccelerationStructureKHR accelerationStructure;
	VkBuffer accelerationStructureBuffer;
	VkDeviceMemory accelerationStructureBufferMemory;

	VkAccelerationStructureKHR bottomLevelAccelerationStructure;
	VkBuffer bottomLevelAccelerationStructureBuffer;
	VkDeviceMemory bottomLevelAccelerationStructureBufferMemory;

	VkAccelerationStructureKHR topLevelAccelerationStructure;
	VkBuffer topLevelAccelerationStructureBuffer;
	VkDeviceMemory topLevelAccelerationStructureBufferMemory;

	VkImageView rayTraceImageView;
	VkImage rayTraceImage;
	VkDeviceMemory rayTraceImageMemory;

	VkImageView ImageView_indirectLgt;
	VkImage Image_indirectLgt;
	VkDeviceMemory ImageMemory_indirectLgt;

	VkImageView ImageView_indirectLgt_2;
	VkImage Image_indirectLgt_2;
	VkDeviceMemory ImageMemory_indirectLgt_2;
	

	VkDescriptorPool descriptorPool;
	VkDescriptorSet rayTraceDescriptorSet;
	VkDescriptorSet materialDescriptorSet;
	VkDescriptorSetLayout* rayTraceDescriptorSetLayouts;

	VkPipeline rayTracePipeline;
	VkPipelineLayout rayTracePipelineLayout;

	VkBuffer shaderBindingTableBuffer;
	VkDeviceMemory shaderBindingTableBufferMemory;
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT || messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ||
		messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT|| messageSeverity == VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		printf("\033[22;36mvalidation layer\033[0m: \033[22;33m%s\033[0m\n", pCallbackData->pMessage);
	}

	return VK_FALSE;
}

void readFile(const char* fileName, char** buffer, uint64_t* length);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

