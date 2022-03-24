#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 interpolatedPosition;
layout(location = 1) out vec4 clipPos;
layout(location = 2) out mat4 PVMatrix;

layout(binding = 1, set = 0) uniform Camera {
  vec4 position;
  vec4 right;
  vec4 up;
  vec4 forward;

  uint frameCount;
  uint ViewPortWidth;
  uint ViewPortHeight;
} camera;

layout(binding = 5, set = 0) uniform ShadingMode {
  //mat4 invViewMatrix;
  //mat4 invProjMatrix;
  mat4 PrevViewMatrix;
  mat4 PrevProjectionMatrix;
  uint enable2Ray;
  uint enableShadowMotion;
  uint enable2SR;
} shadingMode;

void main() {
  vec4 positionVector = camera.position - vec4(0.0, 0.0, 0.0, 1.0);
  mat4 viewMatrix = {
    vec4(camera.right.x, camera.up.x, camera.forward.x, 0),
    vec4(camera.right.y, camera.up.y, camera.forward.y, 0),
    vec4(camera.right.z, camera.up.z, camera.forward.z, 0),
    vec4(-dot(camera.right, positionVector), -dot(camera.up, positionVector), -dot(camera.forward, positionVector), 1)
  };

  float farDist = 1000.0;
  float nearDist = 0.0001;
  float frustumDepth = farDist - nearDist;
  float oneOverDepth = 1.0 / frustumDepth;
  float fov = 1.0472;
  float aspect = 3840 / 2160;

  mat4 projectionMatrix = {
    vec4(1.0 / tan(0.5f * fov) / aspect, 0, 0, 0),
    vec4(0, 1.0 / tan(0.5f * fov), 0, 0),
    vec4(0, 0, farDist * oneOverDepth, 1),
    vec4(0, 0, (-farDist * nearDist) * oneOverDepth, 0)
  };

  PVMatrix=shadingMode.PrevProjectionMatrix*shadingMode.PrevViewMatrix;
  //PVMatrix=projectionMatrix*viewMatrix;

  //debugPrintfEXT("\n PrevViewMatrix[0][i] is               ViewMatrix[0][i] is\n  %f %f %f %f             %f %f %f %f ",
  //               shadingMode.PrevViewMatrix[3][0],shadingMode.PrevViewMatrix[3][1],shadingMode.PrevViewMatrix[3][2],shadingMode.PrevViewMatrix[3][3], 
  //               viewMatrix[3][0],viewMatrix[3][1],viewMatrix[3][2],viewMatrix[3][3] );

 // PVMatrix=projectionMatrix*viewMatrix;  //current frame
  gl_Position = projectionMatrix * viewMatrix * vec4(inPosition, 1.0);
 // gl_Position = PVMatrix* vec4(inPosition, 1.0);
  clipPos=PVMatrix* vec4(inPosition, 1.0);
  
  interpolatedPosition = inPosition;
}
