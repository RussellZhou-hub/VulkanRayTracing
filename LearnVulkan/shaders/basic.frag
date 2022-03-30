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
  vec3 indirectColor_2 = vec3(0.0, 0.0, 0.0);

  ivec3 indices = ivec3(indexBuffer.data[3 * gl_PrimitiveID + 0], indexBuffer.data[3 * gl_PrimitiveID + 1], indexBuffer.data[3 * gl_PrimitiveID + 2]);

  vec3 vertexA = vec3(vertexBuffer.data[3 * indices.x + 0], vertexBuffer.data[3 * indices.x + 1], vertexBuffer.data[3 * indices.x + 2]);
  vec3 vertexB = vec3(vertexBuffer.data[3 * indices.y + 0], vertexBuffer.data[3 * indices.y + 1], vertexBuffer.data[3 * indices.y + 2]);
  vec3 vertexC = vec3(vertexBuffer.data[3 * indices.z + 0], vertexBuffer.data[3 * indices.z + 1], vertexBuffer.data[3 * indices.z + 2]);
  
  vec3 geometricNormal = normalize(cross(vertexB - vertexA, vertexC - vertexA));

  vec3 surfaceColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].diffuse;

  // 40 & 41 == light
  if (gl_PrimitiveID == 40 || gl_PrimitiveID == 41) {
    directColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission;
     debugPrintfEXT("lightVertexA.x is %f  lightVertexA.y is %f lightVertexA.z is %f \n lightVertexB.x is %f  lightVertexB.y is %f lightVertexB.z is %f \n lightVertexC.x is %f  lightVertexC.y is %f lightVertexC.z is %f \n",vertexA.x,vertexA.y,vertexA.z,vertexB.x,vertexB.y,vertexB.z,vertexC.x,vertexC.y,vertexC.z);
  }
  else {
    int randomIndex = int(random(gl_FragCoord.xy) * 2 + 40);//40 is the light area
    vec3 lightColor = vec3(0.6, 0.6, 0.6);
    //vec3 lightPosition = lightVertexA * lightBarycentric.x + lightVertexB * lightBarycentric.y + lightVertexC * lightBarycentric.z;
    vec3 lightPosition=getRadomLightPosition(randomIndex);

    vec3 positionToLightDirection = normalize(lightPosition - interpolatedPosition);

    vec3 shadowRayOrigin = interpolatedPosition;
    vec3 shadowRayDirection = positionToLightDirection;
    float shadowRayDistance = length(lightPosition - interpolatedPosition) - 0.001f;

    
    
    //shadow ray
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, shadowRayOrigin, 0.001f, shadowRayDirection, shadowRayDistance);
  
    while (rayQueryProceedEXT(rayQuery));

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
      directColor = surfaceColor * lightColor * dot(geometricNormal, positionToLightDirection);    //not in shadow
    }
    else {
      directColor = vec3(0.0, 0.0, 0.0);                     //in shadow
      isShadow=true;
    }

    if(shadingMode.enableShadowMotion==1 && isShadow==true && camera.frameCount > 0){
      //Implement shadow motion vector algorithm
      //vec4 prevFramePos=shadingMode.PrevProjectionMatrix*shadingMode.PrevViewMatrix*vec4(interpolatedPosition, 1.0);

      fragPos=clipPos.xyz;
      
      //fragPos.xy=clipPos.xy;
      fragPos.xy/=clipPos.w;
      fragPos.y=-fragPos.y;
      fragPos.xy+=1;
      fragPos.xy/=2;
      fragPos.x*=camera.ViewPortWidth;
      fragPos.y*=camera.ViewPortHeight;
      //fragPos.x-=500;
      fragPos.xy=floor(fragPos.xy)+0.5;
      fragPos.xy=getFragCoord(interpolatedPosition.xyz);
      vec4 previousColor = imageLoad(image, ivec2(fragPos.xy));

      vec4 worldPos=getWorldPos(gl_FragCoord.xyz);
      ///if( fragPos.y>1079){
      //  debugPrintfEXT("gl_FragCoord.x is %f  gl_FragCoord.y is %f \n worldPos.x is %f   worldPos.y is %f  clipPos.x is %f interpolatedPos.x is %f interpolatedPos.y is %f   clipPos.w is %f   \n\n",
      //  gl_FragCoord.x,gl_FragCoord.y, worldPos.x,worldPos.y,clipPos.x,interpolatedPosition.x,interpolatedPosition.y,clipPos.w);
      //}

      float alpha=0.9;
      
      int d=10;
      if(shadingMode.enableShadowMotion==1 && shadingMode.enableMeanDiff==1 && isShadow==true && camera.frameCount > 0){  //spatial filter
        
        // bilinear filter  双线性过滤 提升质量
        
        float level=4;
        vec4 preShadow_01 = imageLoad(image, ivec2(fragPos.x,fragPos.y-level));
        vec4 preShadow_10 = imageLoad(image, ivec2(fragPos.x-level,fragPos.y));
        vec4 preShadow_12 = imageLoad(image, ivec2(fragPos.x+level,fragPos.y));
        vec4 preShadow_21 = imageLoad(image, ivec2(fragPos.x,fragPos.y+level));
        vec4 PreShadow=preShadow_01+preShadow_10+preShadow_12+preShadow_21;
        //vec4 PreShadow=preShadow_11+preShadow_01+preShadow_10+preShadow_12+preShadow_21+preShadow_00+preShadow_02+preShadow_20+preShadow_22;
        PreShadow/=4;

        float beta=0.25;
        float avgInt;

        //if(avg.x>0.6){
        //    debugPrintfEXT("previousColorR.x is %f\n avg.x is %f",previousColorR.x,avg.x);
        //    debugPrintfEXT("gl_FragCoord.x is %f  gl_FragCoord.y is %f \n fragPos.x is %f   fragPos.y is %f  clipPos.x is %f interpolatedPos.x is %f   clipPos.w is %f   \n\n",
        //    gl_FragCoord.x,gl_FragCoord.y, fragPos.x,fragPos.y,clipPos.x,interpolatedPosition.x,clipPos.w);
        //}
        
        //if((variance.x+variance.y+variance.z)/3>0){  //估计 noisy
        avgInt=PreShadow.x+PreShadow.y+PreShadow.z;
        avgInt/=3;
        if(avgInt>0.35){
            if(isShadow){
                directColor+=vec3(0.25,0.25,0.25);
            }
        }
        directColor=alpha*PreShadow.xyz+(1-alpha)*directColor.xyz;
      }
      else{
        directColor=alpha*previousColor.xyz+(1-alpha)*directColor.xyz;
      }
    }
  }

  vec3 hemisphere = uniformSampleHemisphere(vec2(random(gl_FragCoord.xy, camera.frameCount), random(gl_FragCoord.xy, camera.frameCount + 1)));
  vec3 alignedHemisphere = alignHemisphereWithCoordinateSystem(hemisphere, geometricNormal);

  vec3 rayOrigin = interpolatedPosition;
  //vec3 rayDirection = alignedHemisphere;
  
  //direction map
  vec3 rayDirection;
  if(shadingMode.enable2thRayDierctionSpatialFilter==1){
      rayDirection = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.xy)).xyz;;
  }
  else{
    rayDirection = getSampledReflectedDirection(interpolatedPosition.xyz,geometricNormal,gl_FragCoord.xy,camera.frameCount);
  }
  
  
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
      }
      else {
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
          /*
          if(shadingMode.enable2thRayDierctionSpatialFilter==1){
          float level=4;
            vec4 preIndirectColor_00 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
            vec4 preIndirectColor_01 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
            vec4 preIndirectColor_02 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
            vec4 preIndirectColor_10 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
            vec4 preIndirectColor_11 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));
            vec4 preIndirectColor_12 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
            vec4 preIndirectColor_20 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
            vec4 preIndirectColor_21 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
            vec4 preIndirectColor_22 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
            vec4 preIndirectColor=(1/4.0)*preIndirectColor_11+(1/8.0)*(preIndirectColor_01+preIndirectColor_10+preIndirectColor_12+preIndirectColor_21)+(1/16.0)*(preIndirectColor_00+preIndirectColor_02+preIndirectColor_20+preIndirectColor_22);
            indirectColor=preIndirectColor.xyz;
          }
          */

          
        }
        else {
          rayActive = false;
        }

        //if(abs(indirectColor.x-directColor.x)>0.3){
        //    vec3 avgColor=(indirectColor+directColor)/2;
        //    directColor=avgColor;
         //   indirectColor=avgColor;
        //}
        if(shadingMode.enableShadowMotion==1 && gl_PrimitiveID != 40 && gl_PrimitiveID != 41){
             float subFactor;
        if(directColor.x<0.1){
            subFactor=0.3;
        }
        else if(directColor.x>0.1 && directColor.x<0.3){
            subFactor=0.25;
        }
        else if(directColor.x>0.3 && directColor.x<0.6){
            subFactor=0.1;
        }
        indirectColor-=vec3(subFactor,subFactor,subFactor);
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

  if(!isShadow && shadingMode.enable2thRMotion==1 && gl_PrimitiveID != 40 && gl_PrimitiveID != 41 && camera.frameCount > 0){
    vec4 previousColor = imageLoad(image, ivec2(gl_FragCoord.xy));
    color.xyz=0.8*previousColor.xyz+0.2*color.xyz;
  }

  /*
  if (camera.frameCount > 0) {              //静止画面下使用前一帧像素降噪
    vec4 previousColor = imageLoad(image, ivec2(gl_FragCoord.xy));
    previousColor *= camera.frameCount;

    color += previousColor;
    color /= (camera.frameCount + 1);
  }
  */
  if(shadingMode.enableShadowMotion==1 && gl_PrimitiveID != 40 && gl_PrimitiveID != 41 && camera.frameCount > 0){
    float addBrightness=0.2;
    float addIndirect=0.2;
    if(indirectColor.x>0.0 && color.x<0.5){     //clamp 阴影中的反射颜色
        color.xyz=vec3(addBrightness)+addIndirect*indirectColor;
     }
    else if(indirectColor.y>0.0 && color.y<0.5){
        color.xyz=vec3(addBrightness)+addIndirect*indirectColor;
    }
    else if(indirectColor.z>0.0 && color.z<0.5){
        color.xyz=vec3(addBrightness)+addIndirect*indirectColor;
    }
    else if(avgBrightness(indirectColor.xyz)>0.0 && avgBrightness(color.xyz)<0.5){
        color.xyz=vec3(addBrightness)+addIndirect*indirectColor;
    }
  }

  outColor = color;
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

vec3 getSpatial_SampledReflectedDirection(vec3 inPos,vec3 normal,vec2 uv,float seed){
    vec3 cameraPos=camera.position.xyz;
    vec3 inRay=inPos-camera.position.xyz;
    vec3 right=cross(inRay,normal);
    vec3 front=cross(right,normal);
    right=normalize(right);
    front=normalize(front);
    float step=2;
    vec3 rPos=inPos+step*right;
    vec3 fPos=inPos+step*front;
    vec3 bPos=inPos-step*front;
    vec3 lPos=inPos-step*right;

    float weight=0.2;  //reflection rate

    vec3 rRay=getReflectedDierction(rPos-camera.position.xyz,normal);
    float theta_r=M_PI*random(vec2(rRay.xy));
    float phi_r=2*M_PI*random(vec2(rRay.y,rRay.x+0.2));
    vec3 RandomRay_r=vec3(sin(theta_r)*cos(phi_r),sin(theta_r)*sin(phi_r),cos(theta_r));
    rRay=normalize(weight*rRay+(1-weight)*normalize(RandomRay_r));

    vec3 lRay=getReflectedDierction(lPos-camera.position.xyz,normal);
    float theta_l=M_PI*random(vec2(lRay.xy));
    float phi_l=2*M_PI*random(vec2(lRay.y,lRay.x));
    vec3 RandomRay_l=vec3(sin(theta_l)*cos(phi_l),sin(theta_l)*sin(phi_l),cos(theta_l));
    lRay=normalize(weight*lRay+(1-weight)*normalize(RandomRay_l));

    vec3 bRay=getReflectedDierction(bPos-camera.position.xyz,normal);
    float theta_b=M_PI*random(vec2(bRay.xy));
    float phi_b=2*M_PI*random(vec2(bRay.y,bRay.x));
    vec3 RandomRay_b=vec3(sin(theta_b)*cos(phi_b),sin(theta_b)*sin(phi_b),cos(theta_b));
    bRay=normalize(weight*bRay+(1-weight)*normalize(RandomRay_b));

    vec3 fRay=getReflectedDierction(fPos-camera.position.xyz,normal);
    float theta_f=M_PI*random(vec2(fRay.xy));
    float phi_f=2*M_PI*random(vec2(fRay.y,fRay.x));
    vec3 RandomRay_f=vec3(sin(theta_f)*cos(phi_f),sin(theta_f)*sin(phi_f),cos(theta_f));
    bRay=normalize(weight*bRay+(1-weight)*normalize(RandomRay_b));


    vec3 Ray=getSampledReflectedDirection(inPos,normal,uv,seed);
    Ray+=rRay+lRay+bRay+fRay;
    Ray/=5;
    
    return normalize(Ray);
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