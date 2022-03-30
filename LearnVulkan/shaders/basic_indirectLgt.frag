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

float random(vec2 uv, float seed);
float random(vec2 p);
float random_1(vec2 uv, float seed);
float avgBrightness(vec3 color);
vec2 getFragCoord(vec3 pos);
vec4 getWorldPos(vec3 fragPos);
vec3 getRadomLightPosition(int randomIndex);
vec3 getReflectedDierction(vec3 inRay,vec3 normal );
vec3 getSampledReflectedDirection(vec3 inRay,vec3 normal,vec2 uv,float seed);
vec3 getSpatial_SampledReflectedDirection(vec3 inPos,vec3 normal,vec2 uv,float seed);
vec3 uniformSampleHemisphere(vec2 uv);
vec3 alignHemisphereWithCoordinateSystem(vec3 hemisphere, vec3 up);




void main() {
  vec3 directColor = vec3(0.0, 0.0, 0.0);
  vec3 indirectColor = vec3(0.0, 0.0, 0.0);
  outColor.xyz=directColor;

  ivec3 indices = ivec3(indexBuffer.data[3 * gl_PrimitiveID + 0], indexBuffer.data[3 * gl_PrimitiveID + 1], indexBuffer.data[3 * gl_PrimitiveID + 2]);

  vec3 vertexA = vec3(vertexBuffer.data[3 * indices.x + 0], vertexBuffer.data[3 * indices.x + 1], vertexBuffer.data[3 * indices.x + 2]);
  vec3 vertexB = vec3(vertexBuffer.data[3 * indices.y + 0], vertexBuffer.data[3 * indices.y + 1], vertexBuffer.data[3 * indices.y + 2]);
  vec3 vertexC = vec3(vertexBuffer.data[3 * indices.z + 0], vertexBuffer.data[3 * indices.z + 1], vertexBuffer.data[3 * indices.z + 2]);
  
  vec3 geometricNormal = normalize(cross(vertexB - vertexA, vertexC - vertexA));

  vec3 surfaceColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].diffuse;

  // 40 & 41 == light
  if (gl_PrimitiveID == 40 || gl_PrimitiveID == 41) {
    directColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission;
    outColor.xyz=directColor;
  }
  else {

  }

  vec3 hemisphere = uniformSampleHemisphere(vec2(random(gl_FragCoord.xy, camera.frameCount), random(gl_FragCoord.xy, camera.frameCount + 1)));
  vec3 alignedHemisphere = alignHemisphereWithCoordinateSystem(hemisphere, geometricNormal);

  vec3 rayOrigin = interpolatedPosition;
  //vec3 rayDirection = alignedHemisphere;
  
  //direction map
  vec3 rayDirection;
  vec3 rayDirection_2thRay;
  rayDirection = getSampledReflectedDirection(interpolatedPosition.xyz,geometricNormal,gl_FragCoord.xy,camera.frameCount);
  rayDirection_2thRay=rayDirection;
  vec3 previousNormal = geometricNormal;

  bool rayActive = true;
  int maxRayDepth = 1;
  if(shadingMode.enable2Ray==1 && camera.frameCount > 0){
    for (int rayDepth = 0; rayDepth < maxRayDepth && rayActive; rayDepth++) {
  //secondary ray (or more ray)
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, rayOrigin, 0.001f, rayDirection, 1000.0f);

    while (rayQueryProceedEXT(rayQuery));

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
      int extensionPrimitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
      vec2 extensionIntersectionBarycentric = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);

      ivec3 extensionIndices = ivec3(indexBuffer.data[3 * extensionPrimitiveIndex + 0], indexBuffer.data[3 * extensionPrimitiveIndex + 1], indexBuffer.data[3 * extensionPrimitiveIndex + 2]);
      vec3 extensionBarycentric = vec3(1.0 - extensionIntersectionBarycentric.x - extensionIntersectionBarycentric.y, extensionIntersectionBarycentric.x, extensionIntersectionBarycentric.y);
      
      vec3 extensionVertexA = vec3(vertexBuffer.data[3 * extensionIndices.x + 0], vertexBuffer.data[3 * extensionIndices.x + 1], vertexBuffer.data[3 * extensionIndices.x + 2]);
      vec3 extensionVertexB = vec3(vertexBuffer.data[3 * extensionIndices.y + 0], vertexBuffer.data[3 * extensionIndices.y + 1], vertexBuffer.data[3 * extensionIndices.y + 2]);
      vec3 extensionVertexC = vec3(vertexBuffer.data[3 * extensionIndices.z + 0], vertexBuffer.data[3 * extensionIndices.z + 1], vertexBuffer.data[3 * extensionIndices.z + 2]);
    
      vec3 extensionPosition = extensionVertexA * extensionBarycentric.x + extensionVertexB * extensionBarycentric.y + extensionVertexC * extensionBarycentric.z;
      vec3 extensionNormal = normalize(cross(extensionVertexB - extensionVertexA, extensionVertexC - extensionVertexA));

      vec3 extensionSurfaceColor = materialBuffer.data[materialIndexBuffer.data[extensionPrimitiveIndex]].diffuse;

      if (extensionPrimitiveIndex == 40 || extensionPrimitiveIndex == 41) {
        indirectColor += (1.0 / (rayDepth + 1)) * materialBuffer.data[materialIndexBuffer.data[extensionPrimitiveIndex]].emission * dot(previousNormal, rayDirection);
        outColor=vec4(indirectColor,1.0);
      }
      else {

      /*
      float level=8;
        vec4 preIndirectDirection_00 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
        vec4 preIndirectDirection_01 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
        vec4 preIndirectDirection_02 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
        vec4 preIndirectDirection_10 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
        vec4 preIndirectDirection_11 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));
        vec4 preIndirectDirection_12 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
        vec4 preIndirectDirection_20 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
        vec4 preIndirectDirection_21 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
        vec4 preIndirectDirection_22 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
        vec4 preIndirectDirection=(1/4.0)*preIndirectDirection_11+(1/8.0)*(preIndirectDirection_01+preIndirectDirection_10+preIndirectDirection_12+preIndirectDirection_21)+(1/16.0)*(preIndirectDirection_00+preIndirectDirection_02+preIndirectDirection_20+preIndirectDirection_22);

        outColor=vec4(beta_indirect*preIndirectDirection.xyz+(1-beta_indirect)*rayDirection,1.0); //direction of the 2thRay
      */
      float beta_indirect=0.8;

        

        { //5*5 guassian
        /*
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
            vec4 r4=1*preIndirectDirection_40+4*preIndirectDirection_41+7*preIndirectDirection_42+4*preIndirectDirection_43+1*preIndirectDirection_44;  */

            //vec4 total=(r0+r1+r2+r3+r4)/273;
            vec4 total=imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));;
            outColor=vec4(beta_indirect*total.xyz+(1-beta_indirect)*rayDirection,1.0); //direction of the 2thRay
        }


        RayHitPointFragCoord=getFragCoord(extensionPosition.xyz);


        int randomIndex = int(random(gl_FragCoord.xy, camera.frameCount + rayDepth) * 2 + 40);
        vec3 lightColor = vec3(0.6, 0.6, 0.6);

        ivec3 lightIndices = ivec3(indexBuffer.data[3 * randomIndex + 0], indexBuffer.data[3 * randomIndex + 1], indexBuffer.data[3 * randomIndex + 2]);

        //vec3 lightVertexA = vec3(vertexBuffer.data[3 * lightIndices.x + 0], vertexBuffer.data[3 * lightIndices.x + 1], vertexBuffer.data[3 * lightIndices.x + 2]);
        //vec3 lightVertexB = vec3(vertexBuffer.data[3 * lightIndices.y + 0], vertexBuffer.data[3 * lightIndices.y + 1], vertexBuffer.data[3 * lightIndices.y + 2]);
        //vec3 lightVertexC = vec3(vertexBuffer.data[3 * lightIndices.z + 0], vertexBuffer.data[3 * lightIndices.z + 1], vertexBuffer.data[3 * lightIndices.z + 2]);

        vec3 lightVertexA = camera.lightA.xyz;
        vec3 lightVertexB = camera.lightB.xyz;
        vec3 lightVertexC = camera.lightC.xyz;

        vec2 uv = vec2(random(gl_FragCoord.xy, camera.frameCount + rayDepth), random(gl_FragCoord.xy, camera.frameCount + rayDepth + 1));
        if (uv.x + uv.y > 1.0f) {
          uv.x = 1.0f - uv.x;
          uv.y = 1.0f - uv.y;
        }

        vec3 lightBarycentric = vec3(1.0 - uv.x - uv.y, uv.x, uv.y);
        vec3 lightPosition = lightVertexA * lightBarycentric.x + lightVertexB * lightBarycentric.y + lightVertexC * lightBarycentric.z;

        vec3 positionToLightDirection = normalize(lightPosition - extensionPosition);

        vec3 shadowRayOrigin = extensionPosition;
        vec3 shadowRayDirection = positionToLightDirection;
        float shadowRayDistance = length(lightPosition - extensionPosition) - 0.001f;

        rayQueryEXT rayQuery;
        rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, shadowRayOrigin, 0.001f, shadowRayDirection, shadowRayDistance);
      
        while (rayQueryProceedEXT(rayQuery));

        //secondary shadow ray
        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
          indirectColor += (1.0 / (rayDepth + 1)) * extensionSurfaceColor * lightColor  * dot(previousNormal, rayDirection) * dot(extensionNormal, positionToLightDirection);
        }
        else {
          rayActive = false;

          //outColor=vec4(0.0,0.0,0.0,1.0);
        }
      }

      vec3 hemisphere = uniformSampleHemisphere(vec2(random(gl_FragCoord.xy, camera.frameCount + rayDepth), random(gl_FragCoord.xy, camera.frameCount + rayDepth + 1)));
      vec3 alignedHemisphere = alignHemisphereWithCoordinateSystem(hemisphere, extensionNormal);

      //reset rayOrigin...
      rayOrigin = extensionPosition;
      rayDirection = alignedHemisphere;
      previousNormal = extensionNormal;

      //RayHitPointFragCoord=getFragCoord(interpolatedPosition.xyz);
      RayHitPointFragCoord=getFragCoord(extensionPosition.xyz);
    }
    else {
      rayActive = false;
    }
  }
  }


  vec4 color = vec4(directColor + indirectColor, 1.0);

  if(camera.frameCount==0){
      outColor=vec4(rayDirection,1.0);
  }

  //outColor = color;
}


float random(vec2 p) 
{ 
    vec2 K1 = vec2(
     23.14069263277926, // e^pi (Gelfond's constant) 
     2.665144142690225 // 2^sqrt(2) (Gelfondâ€“Schneider constant) 
    ); 
    return fract(cos(dot(p,K1)) * 12345.6789); 
} 

float random(vec2 uv, float seed) {     // 0到1的随机数
  return fract(sin(mod(dot(uv, vec2(12.9898, 78.233)) + 1113.1 * seed, M_PI)) * 43758.5453);
}

float random_1(vec2 uv, float seed) {     // -1到1的随机数
  return pow(-1,mod(seed,1))*fract(sin(mod(dot(uv, vec2(12.9898, 78.233)) + 1113.1 * seed, M_PI)) * 43758.5453);
}

float avgBrightness(vec3 color){
    float avg=color.x+color.y+color.z;
    return avg/3;
}

vec2 getFragCoord(vec3 pos){          //从世界坐标获取对应的上一帧里的屏幕坐标
    vec4 clipPos=PVMatrix*vec4(pos,1.0);
      
    clipPos/=clipPos.w;
    clipPos.y=-clipPos.y;
    clipPos.xy+=1;
    clipPos.xy/=2;
    clipPos.x*=camera.ViewPortWidth;
    clipPos.y*=camera.ViewPortHeight;
    return floor(clipPos.xy)+0.5;
}

vec4 getWorldPos(vec3 fragPos){
    vec4 worldPos;
    worldPos=vec4(fragPos.xy/vec2(camera.ViewPortWidth,camera.ViewPortHeight)*2.0-1.0,fragPos.z,1.0);
    worldPos.y=-worldPos.y;
    worldPos=invProjMatrix*worldPos;
    worldPos.xyz/=worldPos.w;
    worldPos=invViewMatrix*worldPos;
    return worldPos;
}

vec3 getRadomLightPosition(int randomIndex){
    ivec3 lightIndices = ivec3(indexBuffer.data[3 * randomIndex + 0], indexBuffer.data[3 * randomIndex + 1], indexBuffer.data[3 * randomIndex + 2]);

    //vec3 lightVertexA = vec3(vertexBuffer.data[3 * lightIndices.x + 0], vertexBuffer.data[3 * lightIndices.x + 1], vertexBuffer.data[3 * lightIndices.x + 2]);
    //vec3 lightVertexB = vec3(vertexBuffer.data[3 * lightIndices.y + 0], vertexBuffer.data[3 * lightIndices.y + 1], vertexBuffer.data[3 * lightIndices.y + 2]);
    //vec3 lightVertexC = vec3(vertexBuffer.data[3 * lightIndices.z + 0], vertexBuffer.data[3 * lightIndices.z + 1], vertexBuffer.data[3 * lightIndices.z + 2]);

    vec3 lightVertexA = camera.lightA.xyz;
    vec3 lightVertexB = camera.lightB.xyz;
    vec3 lightVertexC = camera.lightC.xyz;

    vec2 uv = vec2(random(gl_FragCoord.xy, camera.frameCount), random(vec2(gl_FragCoord.y,gl_FragCoord.x), camera.frameCount + 1));
    if (uv.x + uv.y > 1.0f) {
      uv.x = 1.0f - uv.x;
      uv.y = 1.0f - uv.y;
    }

    //if( shadingMode.enable2SR==1){
    //    uv.x=0.3;
    //    uv.y=0.3;
    //}

    vec3 lightBarycentric = vec3(1.0 - uv.x - uv.y, uv.x, uv.y);
    vec3 lightPosition = lightVertexA * lightBarycentric.x + lightVertexB * lightBarycentric.y + lightVertexC * lightBarycentric.z;
    return lightPosition;
}

vec3 getReflectedDierction(vec3 inRay,vec3 normal ){     //反射角度
    inRay=normalize(inRay);
    normal=normalize(normal);
    vec3 outRay = inRay - 2*dot(inRay,normal)*normal;
    return normalize(outRay);
}

vec3 getSampledReflectedDirection(vec3 inRay,vec3 normal,vec2 uv,float seed){
    inRay=inRay-camera.position.xyz;
    vec3 Ray=getReflectedDierction(inRay,normal);
    float theta=M_PI*random(uv);
    float phi=2*M_PI*random(vec2(uv.y,uv.x));
    vec3 RandomRay=vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));
    float weight=0.8;  //reflection rate
    return normalize(weight*Ray+(1-weight)*normalize(RandomRay));
}

vec3 uniformSampleHemisphere(vec2 uv) {
  float z = uv.x;
  float r = sqrt(max(0, 1.0 - z * z));
  float phi = 2.0 * M_PI * uv.y;

  return vec3(r * cos(phi), z, r * sin(phi));
}

vec3 alignHemisphereWithCoordinateSystem(vec3 hemisphere, vec3 up) {
  vec3 right = normalize(cross(up, vec3(0.0072f, 1.0f, 0.0034f)));
  vec3 forward = cross(right, up);

  return hemisphere.x * right + hemisphere.y * up + hemisphere.z * forward;
}