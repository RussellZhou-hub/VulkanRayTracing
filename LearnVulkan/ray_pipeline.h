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
const uint32_t WIDTH = 3840;
const uint32_t HEIGHT = 2160;



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



//#include "tinyobj_loader_c.c"
//#include "tiny_obj_loader.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
};

/*
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_NV_RAY_TRACING_EXTENSION_NAME
};
     */


#define MAX_FRAMES_IN_FLIGHT      1
#define ENABLE_VALIDATION         0

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
    void run(Scene& scene,Camera& camera);
private:
  //void loadModel(Scene* scene);
  void initializeScene(Scene* scene, const char* fileNameOBJ);
  void initVulkan(Scene* scene);
  void mainLoop(VkRayTracingApplication* app,Camera* camera);
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
  void createCommandPool(VkRayTracingApplication* app);
  void createVertexBuffer(VkRayTracingApplication* app, Scene* scene);
  void createIndexBuffer(VkRayTracingApplication* app, Scene* scene);
  void createMaterialsBuffer(VkRayTracingApplication* app, Scene* scene);
  void createTextures(VkRayTracingApplication* app);
  void createBottomLevelAccelerationStructure(VkRayTracingApplication* app, Scene* scene);
  void createTopLevelAccelerationStructure(VkRayTracingApplication* app);
  //Descriptor Setup
  void createUniformBuffer(VkRayTracingApplication* app);
  void createDescriptorSets(VkRayTracingApplication* app);
  void createRayTracePipeline(VkRayTracingApplication* app);
  void createShaderBindingTable(VkRayTracingApplication* app);
  void createCommandBuffers(VkRayTracingApplication* app);
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

  VkCommandPool commandPool;

  VkSemaphore* imageAvailableSemaphores;
  VkSemaphore* renderFinishedSemaphores;
  VkFence* inFlightFences;
  VkFence* imagesInFlight;
  uint32_t currentFrame;

  VkPhysicalDeviceMemoryProperties memoryProperties;

  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;

  VkBuffer vertexPositionBuffer;
  VkDeviceMemory vertexPositionBufferMemory;

  VkBuffer materialIndexBuffer;
  VkDeviceMemory materialIndexBufferMemory;

  VkBuffer materialBuffer;
  VkDeviceMemory materialBufferMemory;

  VkAccelerationStructureKHR bottomLevelAccelerationStructure;
  VkBuffer bottomLevelAccelerationStructureBuffer;
  VkDeviceMemory bottomLevelAccelerationStructureBufferMemory;

  VkAccelerationStructureKHR topLevelAccelerationStructure;
  VkBuffer topLevelAccelerationStructureBuffer;
  VkDeviceMemory topLevelAccelerationStructureBufferMemory;

  VkImageView rayTraceImageView;
  VkImage rayTraceImage;
  VkDeviceMemory rayTraceImageMemory;

  VkBuffer uniformBuffer;
  VkDeviceMemory uniformBufferMemory;

  VkDescriptorPool descriptorPool;
  VkDescriptorSet rayTraceDescriptorSet;
  VkDescriptorSet materialDescriptorSet;
  VkDescriptorSetLayout* rayTraceDescriptorSetLayouts;

  VkPipeline rayTracePipeline;
  VkPipelineLayout rayTracePipelineLayout;

  VkBuffer shaderBindingTableBuffer;
  VkDeviceMemory shaderBindingTableBufferMemory;

  VkCommandBuffer* commandBuffers;
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT || messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    printf("\033[22;36mvalidation layer\033[0m: \033[22;33m%s\033[0m\n", pCallbackData->pMessage);
  }

  return VK_FALSE;
}

void readFile(const char* fileName, char** buffer, uint64_t* length);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);


/*
bool make_mesh_and_material_by_obj(const char* filename, const char* basepath, bool triangulate) {

	std::cout << "Loading " << filename << std::endl;

	tinyobj::attrib_t attrib; // 所有的数据放在这里
	std::vector<tinyobj::shape_t> shapes;
	// 一个shape,表示一个部分,
	// 其中主要存的是索引坐标 mesh_t类,
	// 放在indices中
	/*
	// -1 means not used.
	typedef struct {
	  int vertex_index;
	  int normal_index;
	  int texcoord_index;
	} index_t;
	/*
std::vector<tinyobj::material_t> materials;

std::string warn;
std::string err;

bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename,
	basepath, triangulate);

// 接下里就是从上面的属性中取值了
if (!warn.empty()) {
	std::cout << "WARN: " << warn << std::endl;
}

if (!err.empty()) {
	std::cerr << "ERR: " << err << std::endl;
}

if (!ret) {
	printf("Failed to load/parse .obj.\n");
	return false;
}

// ========================== 将读入的模型数据存入自己定义的数据结构中 ========================

std::cout << "# of vertices  : " << (attrib.vertices.size() / 3) << std::endl;
std::cout << "# of normals   : " << (attrib.normals.size() / 3) << std::endl;
std::cout << "# of texcoords : " << (attrib.texcoords.size() / 2)
<< std::endl;

std::cout << "# of shapes    : " << shapes.size() << std::endl;
std::cout << "# of materials : " << materials.size() << std::endl;

///1. 获取各种材质和纹理
{
	for (int i = 0; i < materials.size(); i++) {
		Material* m = new Material();
		tinyobj::material_t tm = materials[i];
		string name = tm.name;
		if (name.size()) {
			m->name = name;
		}
		m->ambient.r = tm.ambient[0];
		m->ambient.g = tm.ambient[1];
		m->ambient.b = tm.ambient[2];

		m->diffuse.r = tm.diffuse[0];
		m->diffuse.g = tm.diffuse[1];
		m->diffuse.b = tm.diffuse[2];

		m->specular.r = tm.specular[0];
		m->specular.g = tm.specular[1];
		m->specular.b = tm.specular[2];

		m->transmittance.r = tm.transmittance[0];
		m->transmittance.g = tm.transmittance[1];
		m->transmittance.b = tm.transmittance[2];

		m->emission.r = tm.emission[0];
		m->emission.g = tm.emission[1];
		m->emission.b = tm.emission[2];

		m->shininess = tm.shininess;
		m->ior = tm.ior;
		m->dissolve = tm.dissolve;
		m->illum = tm.illum;
		m->pad0 = tm.pad0;

		m->ambient_tex_id = -1;
		m->diffuse_tex_id = -1;
		m->specular_tex_id = -1;
		m->specular_highlight_tex_id = -1;
		m->bump_tex_id = -1;
		m->displacement_tex_id = -1;
		m->alpha_tex_id = -1;

		m->ambient_texname = "";
		m->diffuse_texname = "";
		m->specular_texname = "";
		m->specular_highlight_texname = "";
		m->bump_texname = "";
		m->displacement_texname = "";
		m->alpha_texname = "";

		if (tm.ambient_texname.size()) {

		}
		if (tm.diffuse_texname.size()) {

		}
		if (tm.specular_texname.size()) {

		}
		if (tm.specular_highlight_texname.size()) {

		}
		if (tm.bump_texname.size()) {

		}
		if (tm.displacement_texname.size()) {
		}
		if (tm.alpha_texname.size()) {

		}

		this->materials.push_back(m);
	}


}

/// 2.顶点数据
{
	// For each shape 遍历每一个部分
	for (size_t i = 0; i < shapes.size(); i++) {
		// 这部分的名称
		printf("shape[%ld].name = %s\n", static_cast<long>(i),
			shapes[i].name.c_str());
		// 网格的点数
		printf("Size of shape[%ld].mesh.indices: %lu\n", static_cast<long>(i),
			static_cast<unsigned long>(shapes[i].mesh.indices.size()));
		//printf("Size of shape[%ld].path.indices: %lu\n", static_cast<long>(i),static_cast<unsigned long>(shapes[i].path.indices.size()));

		//assert(shapes[i].mesh.num_face_vertices.size() == shapes[i].mesh.material_ids.size());
		//assert(shapes[i].mesh.num_face_vertices.size() == shapes[i].mesh.smoothing_group_ids.size());

		printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
			static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

		Model* model = new Model(); // 每一部分的模型数据
		// 顶点数量  = face的数量x3 
		model->mesh_num = shapes[i].mesh.num_face_vertices.size() * 3;
		// 开辟空间
		Vertex* mesh_data = new Vertex[model->mesh_num];
		size_t index_offset = 0;

		// For each face
		for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
			size_t fnum = shapes[i].mesh.num_face_vertices[f];

			// 获得所索引下标
			tinyobj::index_t idx;
			int vertex_index[3];
			int normal_index[3];
			int texcoord_index[3];
			for (size_t v = 0; v < fnum; v++) {
				idx = shapes[i].mesh.indices[index_offset + v];
				vertex_index[v] = idx.vertex_index;
				texcoord_index[v] = idx.texcoord_index;
				normal_index[v] = idx.normal_index;
			}
			for (size_t v = 0; v < fnum; v++) {
				// v
				mesh_data[index_offset + v].pos.x = attrib.vertices[(vertex_index[v]) * 3 + 0];
				mesh_data[index_offset + v].pos.y = attrib.vertices[(vertex_index[v]) * 3 + 1];
				mesh_data[index_offset + v].pos.z = attrib.vertices[(vertex_index[v]) * 3 + 2];
				mesh_data[index_offset + v].pos.w = 1.0f;

				// vt
				mesh_data[index_offset + v].tc.u = attrib.texcoords[texcoord_index[v] * 2 + 0];
				mesh_data[index_offset + v].tc.v = attrib.texcoords[texcoord_index[v] * 2 + 1];

				// vn
				mesh_data[index_offset + v].normal.x = attrib.normals[normal_index[v] * 3 + 0];
				mesh_data[index_offset + v].normal.y = attrib.normals[normal_index[v] * 3 + 1];
				mesh_data[index_offset + v].normal.z = attrib.normals[normal_index[v] * 3 + 2];
				mesh_data[index_offset + v].normal.w = 1.0f;

				// color
				mesh_data[index_offset + v].color.r = 1.0f;
				mesh_data[index_offset + v].color.g = 1.0f;
				mesh_data[index_offset + v].color.b = 1.0f;
				mesh_data[index_offset + v].color.a = 1.0f;
			}

			// 偏移
			index_offset += fnum;
		}
		model->mesh = mesh_data;
		models.push_back(model);
	}
}

std::cout << "# Loading Complete #" << std::endl;
//PrintInfo(attrib, shapes, materials);
return true;
}
*/

