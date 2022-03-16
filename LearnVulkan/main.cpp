#define GLFW_INCLUDE_VULKAN
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "glfw3dll.lib")
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define TINYOBJ_LOADER_C_IMPLEMENTATION
//#include "tinyobj_loader_c.h"

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <iostream>
//#include"App.h"
//#include"ray_pipeline.h"
#include"ray_query.h"

int main() {
    //rasterization_pipeline
    //App app;
    //app.run();

    //uint32_t extensionCount = 0;
    //vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    //std::cout << extensionCount << " extensions supported\n";

    //glm::mat4 matrix;
   // glm::vec4 vec;
    //auto test = matrix * vec;

    //ray_pipeline

    Scene* scene = new Scene();

    Camera* camera = new Camera();

    ShadingMode* shadingMode = new ShadingMode();

    VkRayTracingApplication *app = new VkRayTracingApplication();
    app->run(*scene,*camera,*shadingMode);

    delete scene;
    delete camera;
    delete shadingMode;
    delete app;

    return 0;
}