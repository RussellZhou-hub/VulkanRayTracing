#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_debug_printf : enable

#define M_PI 3.1415926535897932384626433832795

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 emission;
};

layout(location = 0) in vec3 interpolatedPosition;
layout(location = 1) in vec4 clipPos;
layout(location = 2) in mat4 PVMatrix;
layout(location = 6) in mat4 invProjMatrix;
layout(location = 10) in mat4 invViewMatrix;

layout(location = 0) out vec4 outColor;

vec3 fragPos;
bool isShadow=false;
vec2 RayHitPointFragCoord;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0) uniform Camera {
  mat4 viewMatrix;
  mat4 projMatrix;
  vec4 position;
  vec4 right;
  vec4 up;
  vec4 forward;

  vec4 lightA;
  vec4 lightB;
  vec4 lightC;
  vec4 lightPad;  //for padding

  uint frameCount;
  uint ViewPortWidth;
  uint ViewPortHeight;
} camera;

layout(binding = 2, set = 0) buffer IndexBuffer { uint data[]; } indexBuffer;
layout(binding = 3, set = 0) buffer VertexBuffer { float data[]; } vertexBuffer;
layout(binding = 4, set = 0, rgba32f) uniform image2D image;
layout(binding = 6, set = 0, rgba32f) uniform image2D image_indirectLgt;
layout(binding = 7, set = 0, rgba32f) uniform image2D image_indirectLgt_2;

layout(binding = 5, set = 0) uniform ShadingMode {
  //mat4 invViewMatrix;
  //mat4 invProjMatrix;
  mat4 PrevViewMatrix;
  mat4 PrevProjectionMatrix;
  uint enable2Ray;
  uint enableShadowMotion;
  uint enableMeanDiff;
  uint enable2thRMotion;
  uint enable2thRayDierctionSpatialFilter;
} shadingMode;

layout(binding = 0, set = 1) buffer MaterialIndexBuffer { uint data[]; } materialIndexBuffer;
layout(binding = 1, set = 1) buffer MaterialBuffer { Material data[]; } materialBuffer;




void main() {

        float level=4;
        vec4 preIndirectDirection_00 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
        vec4 preIndirectDirection_01 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
        vec4 preIndirectDirection_02 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
        vec4 preIndirectDirection_10 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
        vec4 preIndirectDirection_11 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));
        vec4 preIndirectDirection_12 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
        vec4 preIndirectDirection_20 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
        vec4 preIndirectDirection_21 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
        vec4 preIndirectDirection_22 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
        //vec4 preIndirectDirection=(1/4.0)*preIndirectDirection_11+(1/8.0)*(preIndirectDirection_01+preIndirectDirection_10+preIndirectDirection_12+preIndirectDirection_21)+(1/16.0)*(preIndirectDirection_00+preIndirectDirection_02+preIndirectDirection_20+preIndirectDirection_22);
        vec4 preIndirectDirection=preIndirectDirection_11+preIndirectDirection_01+preIndirectDirection_10+preIndirectDirection_12+preIndirectDirection_21+preIndirectDirection_00+preIndirectDirection_02+preIndirectDirection_20+preIndirectDirection_22;
        preIndirectDirection/=9;

        preIndirectDirection_11 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.xy));
        outColor=0.1*preIndirectDirection+0.9*preIndirectDirection_11; //direction of the 2thRay

        /*
        //5*5 guassian
            vec4 preIndirectDirection_00 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-2,gl_FragCoord.y-2));
            vec4 preIndirectDirection_01 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-1,gl_FragCoord.y-2));
            vec4 preIndirectDirection_02 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y-2));
            vec4 preIndirectDirection_03 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+1,gl_FragCoord.y-2));
            vec4 preIndirectDirection_04 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+2,gl_FragCoord.y-2));
            vec4 r0=1*preIndirectDirection_00+4*preIndirectDirection_01+7*preIndirectDirection_02+4*preIndirectDirection_03+1*preIndirectDirection_04;

            vec4 preIndirectDirection_10 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-2,gl_FragCoord.y-1));
            vec4 preIndirectDirection_11 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-1,gl_FragCoord.y-1));
            vec4 preIndirectDirection_12 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y-1));
            vec4 preIndirectDirection_13 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+1,gl_FragCoord.y-1));
            vec4 preIndirectDirection_14 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+2,gl_FragCoord.y-1));
            vec4 r1=4*preIndirectDirection_10+16*preIndirectDirection_11+26*preIndirectDirection_12+16*preIndirectDirection_13+4*preIndirectDirection_14;

            vec4 preIndirectDirection_20 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-2,gl_FragCoord.y));
            vec4 preIndirectDirection_21 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-1,gl_FragCoord.y));
            vec4 preIndirectDirection_22 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y));
            vec4 preIndirectDirection_23 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+1,gl_FragCoord.y));
            vec4 preIndirectDirection_24 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+2,gl_FragCoord.y));
            vec4 r2=7*preIndirectDirection_20+26*preIndirectDirection_21+41*preIndirectDirection_22+26*preIndirectDirection_23+7*preIndirectDirection_24;

            vec4 preIndirectDirection_30 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-2,gl_FragCoord.y+1));
            vec4 preIndirectDirection_31 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-1,gl_FragCoord.y+1));
            vec4 preIndirectDirection_32 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y+1));
            vec4 preIndirectDirection_33 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+1,gl_FragCoord.y+1));
            vec4 preIndirectDirection_34 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+2,gl_FragCoord.y+1));
            vec4 r3=4*preIndirectDirection_30+16*preIndirectDirection_31+26*preIndirectDirection_32+16*preIndirectDirection_33+4*preIndirectDirection_34;

            vec4 preIndirectDirection_40 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-2,gl_FragCoord.y+2));
            vec4 preIndirectDirection_41 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-1,gl_FragCoord.y+2));
            vec4 preIndirectDirection_42 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y+2));
            vec4 preIndirectDirection_43 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+1,gl_FragCoord.y+2));
            vec4 preIndirectDirection_44 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+2,gl_FragCoord.y+2));
            vec4 r4=1*preIndirectDirection_40+4*preIndirectDirection_41+7*preIndirectDirection_42+4*preIndirectDirection_43+1*preIndirectDirection_44;

            vec4 total=(r0+r1+r2+r3+r4)/273;
            outColor=total; //direction of the 2thRay
        */
    
            //outColor=imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y));;
}