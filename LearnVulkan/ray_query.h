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
#include<deque>
#include<unordered_map>
#include<string>
#include<set>

//#include"App.h"
//extern "C" {
//#include "tinyobj_loader_c.h"
//}

#include "tinyobj_loader_c.h"
#include<vma/vk_mem_alloc.h>
#include <functional>
#include "Mesh.h"

#define MAX_FRAMES_IN_FLIGHT      1
#define ENABLE_VALIDATION         1

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
	uint32_t enableSVGF;
	uint32_t enable2thRMotion;
	uint32_t enableSVGF_withIndAlbedo;
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

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call the function
		}

		deletors.clear();
	}
};

//note that we store the VkPipeline and layout by value, not pointer.
//They are 64 bit handles to internal driver structures anyway so storing pointers to them isn't very useful
struct Material_pipeline {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh* mesh;
	Material_pipeline* material;
	glm::mat4 transformMatrix = {1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f
	};
};

class VkRayTracingApplication {
public:
	friend class Shader;
	VkRayTracingApplication();
	void run(Scene& scene, Camera& camera, ShadingMode& shadingMode);
private:
	//void loadModel(Scene* scene);
	void initializeScene(Scene* scene, std::string fileName);
	void initVulkan(Scene* scene);
	void mainLoop(VkRayTracingApplication* app, Camera* camera, ShadingMode* shadingMode);
	void cleanup(VkRayTracingApplication* app, Scene* scene);
	void createBuffer(VkRayTracingApplication* app, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
	void copyBuffer(VkRayTracingApplication* app, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createImage(VkRayTracingApplication* app, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage* image, VkDeviceMemory* imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format);
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
	void createTextureImage(VkRayTracingApplication* app);
	void createTextureImageView();
	void createTextureSampler();
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

	std::vector<const char*> enabledDeviceExtensions;

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void load_meshes(Scene* scene, std::string fileName);
	void upload_mesh(Mesh& mesh);
	//create material and add it to the map
	Material_pipeline* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	//returns nullptr if it can't be found
	Material_pipeline* get_material(const std::string& name);
	//returns nullptr if it can't be found
	Mesh* get_mesh(const std::string& name);
	//our draw function
	void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Material> materials;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	int isCameraMoved;
	bool isRenderCornellBox;

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

	VmaAllocator _allocator; //vma lib allocator

	//other code ....
	Mesh _monkeyMesh;

	//default array of renderable objects
	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string, Material_pipeline> _materials;
	std::unordered_map<std::string, Mesh> _meshes;

	int _selectedShader{ 0 };

	DeletionQueue _mainDeletionQueue;

	//for texture
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	//output attachment in shader
	uint32_t colorAttachCount;
	VkImage GnormalImage;  //normal map
	VkImageView GnormalImageView;
	VkDeviceMemory GnormalImageMemory;
	VkImage GDirectImage;   //directlight albedo
	VkImageView GDirectImageView;
	VkDeviceMemory GDirectImageMemory;
	VkImage GDirectIrradImage;   //directlight irradiance
	VkImageView GDirectIrradImageView;
	VkDeviceMemory GDirectIrradImageMemory;
	VkImage G_albedoImage;  //indirectlight albedo
	VkImageView G_albedoImageView;
	VkDeviceMemory G_albedoImageMemory;
	VkImage GIrradImage;  //indirectlight Irrad
	VkImageView GIrradImageView;
	VkDeviceMemory GIrradImageMemory;
	VkImage GWorldPosImage;  //world position Irrad
	VkImageView GWorldPosImageView;
	VkDeviceMemory GWorldPosImageMemory;

	//input image ,Historicl information
	VkImage HDirectIrradImage;   //directlight irradiance
	VkImageView HDirectIrradImageView;
	VkDeviceMemory HDirectIrradImageMemory;

	//input image ,Historicl information
	VkImage HDirectAlbedoImage;   //directlight albedo
	VkImageView HDirectAlbedoImageView;
	VkDeviceMemory HDirectAlbedoImageMemory;

	//input image ,Historicl information
	VkImage HNormalImage;   //normal
	VkImageView HNormalImageView;
	VkDeviceMemory HNormalImageMemory;

	//input image ,Historicl information
	VkImage HWorldPosImage;   //world
	VkImageView HWorldPosImageView;
	VkDeviceMemory HWorldPosImageMemory;

	//input image ,Historicl information
	VkImage HDepthImage;   //Depth
	VkImageView HDepthImageView;
	VkDeviceMemory HDepthImageMemory;

	//input image ,Historicl information
	VkImage HVarImage;   //Historic Variance
	VkImageView HVarImageView;
	VkDeviceMemory HVarImageMemory;

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

class PipelineBuilder {
public:

	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo _depthStencil;

	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
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

