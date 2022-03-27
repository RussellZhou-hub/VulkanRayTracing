#include"Shader.h"
#include<iostream>
#include "ray_query.h"

using namespace std;

Shader::Shader()
{
    isUsed = false;
    shaderFileBuffer = nullptr;
}

Shader::~Shader()
{
    if (isUsed) {
        vkDestroyShaderModule(app->logicalDevice, shaderModule, NULL);
        free(shaderFileBuffer);
    }
}

void Shader::load(VkRayTracingApplication* app,std::string path)
{
    this->app = app;
    string type=judgeTypeByName(path);
    if (type == "vert") {
        
    }
   
    FILE* shaderFile = fopen(path.c_str(), "rb");
    fseek(shaderFile, 0, SEEK_END);
    uint32_t shaderFileSize = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    char* shaderFileBuffer = (char*)malloc(sizeof(char*) * shaderFileSize);
    fread(shaderFileBuffer, 1, shaderFileSize, shaderFile);
    fclose(shaderFile);

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderFileSize;
    shaderModuleCreateInfo.pCode = (uint32_t*)shaderFileBuffer;

    //VkShaderModule shaderModule;
    VkResult result;
    try {
        result = vkCreateShaderModule(app->logicalDevice, &shaderModuleCreateInfo, NULL, &shaderModule);
    }
    catch (...) {
        cout << "unknown exception\n";
    }
    if (result == VK_SUCCESS) {
        string type = judgeTypeByName(path);
        if (type == "vert") {
            printf("created vertex shader module\n");
            ShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (type == "frag") {
            printf("created fragment shader module\n");
            ShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
    }
    ShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStageInfo.module = shaderModule;
    ShaderStageInfo.pName = "main";
    isUsed = true;
}

string Shader::judgeTypeByName(string& path)
{
    int index = path.find('.');  //取得第一个后缀 如basic.frag.spv
    std::string postfix = path.substr(++index,path.size());
    int index_2 = postfix.find('.');
    std::string type = postfix.substr(0, index_2);
    return type;
}
