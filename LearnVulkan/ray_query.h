#define _CRT_SECURE_NO_WARNINGS
#define SEEK_END 2
#define VK_ENABLE_BETA_EXTENSIONS
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


#define MAX_FRAMES_IN_FLIGHT      1
#define ENABLE_VALIDATION         1

static char keyDownIndex[500];

static float cameraPosition[3];
static float cameraYaw;
static float cameraPitch;

class Camera {
public:
	Camera();
	float position[4];
	float right[4];
	float up[4];
	float forward[4];

	uint32_t frameCount;
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
	VkRayTracingApplication();
	void run(Scene& scene, Camera& camera);
private:
	//void loadModel(Scene* scene);
	void initializeScene(Scene* scene, const char* fileNameOBJ);
	void initVulkan(Scene* scene);
	void mainLoop(VkRayTracingApplication* app, Camera* camera);
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
	void createRayTracePipeline(VkRayTracingApplication* app);
	void createShaderBindingTable(VkRayTracingApplication* app);
	void createCommandBuffers(VkRayTracingApplication* app, Scene* scene);
	void createSynchronizationObjects(VkRayTracingApplication* app);
	void updateUniformBuffer(VkRayTracingApplication* app, struct Camera* camera);
	void drawFrame(VkRayTracingApplication* app, struct Camera* camera);

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

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;
	VkCommandBuffer* commandBuffers;

	VkSemaphore* imageAvailableSemaphores;
	VkSemaphore* renderFinishedSemaphores;
	VkFence* inFlightFences;
	VkFence* imagesInFlight;
	uint32_t currentFrame;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

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

	VkDescriptorPool descriptorPool;
	VkDescriptorSet rayTraceDescriptorSet;
	VkDescriptorSet materialDescriptorSet;
	VkDescriptorSetLayout* rayTraceDescriptorSetLayouts;

	VkPipeline rayTracePipeline;
	VkPipelineLayout rayTracePipelineLayout;

	VkBuffer shaderBindingTableBuffer;
	VkDeviceMemory shaderBindingTableBufferMemory;

	int SecondaryRay;
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT || messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		printf("\033[22;36mvalidation layer\033[0m: \033[22;33m%s\033[0m\n", pCallbackData->pMessage);
	}

	return VK_FALSE;
}

void readFile(const char* fileName, char** buffer, uint64_t* length);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

