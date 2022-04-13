#pragma once

#include "vk_types.h"
#include <vector>
#include<vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include"tiny_obj_loader.h"

struct VertexInputDescription {

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    static VertexInputDescription get_vertex_description();
};

class Mesh {
public:
    std::vector<Vertex> _vertices;

    AllocatedBuffer _vertexBuffer;

    //attrib will contain the vertex arrays of the file
    tinyobj::attrib_t attrib;
    //shapes contains the info for each separate object in the file
    std::vector<tinyobj::shape_t> shapes;
    //materials contains the information about the material of each shape, but we won't use it.
    std::vector<tinyobj::material_t> materials;

    bool load_from_obj(const char* filename);
};