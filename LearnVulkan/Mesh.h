#pragma once

#include "vk_types.h"
#include <vector>
#include<array>
#include<vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include"tiny_obj_loader.h"
#include <glm/ext/vector_float2.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct VertexInputDescription {

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {

    glm::vec3 position; int padA;
    glm::vec3 normal; int padB;
    glm::vec3 color; int padC;
    glm::vec2 texCoord; int padD;int padE;

    static VertexInputDescription get_vertex_description();
    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();

    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal && color == other.color && texCoord == other.texCoord;
    }
};

struct Primitive {
    int vertices_count = 3;//hardcorded 3
    int material_id = 0;
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

class Mesh {
public:
    std::vector<Vertex> _vertices;
    std::vector<Primitive> _primitives;
    AllocatedBuffer _vertexBuffer;
    std::vector<Texture> _textures;

    //attrib will contain the vertex arrays of the file
    tinyobj::attrib_t attrib;
    //shapes contains the info for each separate object in the file
    std::vector<tinyobj::shape_t> shapes;
    //materials contains the information about the material of each shape, but we won't use it.
    std::vector<tinyobj::material_t> materials;

    bool load_from_obj(std::string filename);
};

class VulkanObjModel {
public:
    // Single vertex buffer for all primitives
    struct {
        VkBuffer buffer;
        VkDeviceMemory memory;
    } vertices;

    // Single index buffer for all primitives
    struct {
        int count;
        VkBuffer buffer;
        VkDeviceMemory memory;
    } indices;

    // A node represents an object in the obj scene graph
    struct Node {
        Node* parent;
        std::vector<Node> children;
        Mesh mesh;
        glm::mat4 matrix;
    };

    std::vector<Node> nodes;

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
};