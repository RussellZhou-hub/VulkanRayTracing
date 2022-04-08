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
layout(location = 1) out vec4 outDirectIr;
layout(location = 2) out vec4 outIndAlbedo;
layout(location = 3) out vec4 outIndIr;
layout(location = 4) out vec4 outNormal;
layout(location = 5) out vec4 outWorldPos;
layout(location = 6) out vec4 outDepth;

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
layout(binding = 8, set = 0, rgba32f) uniform image2D image_directLgtIr;
layout(binding = 9, set = 0, rgba32f) uniform image2D image_directAlbedo;
layout(binding = 10, set = 0, rgba32f) uniform image2D image_normal;
layout(binding = 11, set = 0, rgba32f) uniform image2D image_worldPos;
layout(binding = 12, set = 0, r32f) uniform image2D image_Depth;
layout(binding = 13, set = 0, rgba32f) uniform image2D image_Var;  //historic Variance

layout(binding = 5, set = 0) uniform ShadingMode {
  //mat4 invViewMatrix;
  //mat4 invProjMatrix;
  mat4 PrevViewMatrix;
  mat4 PrevProjectionMatrix;
  uint enable2Ray;
  uint enableShadowMotion;
  uint enableSVGF;
  uint enable2thRMotion;
  uint enableSVGF_withIndAlbedo;
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

float weight(vec2 p,vec2 q);
float w_depth(vec2 p,vec2 q);
float w_normal(vec2 p,vec2 q);
float w_lumin(vec2 p,vec2 q);
vec4 variance(vec2 p);
vec4 aTrous_indirectIr(vec2 p);
vec4 aTrous_indirectAlbedo(vec2 p);
vec4 aTrous_directIr(vec2 p);




void main() {
  vec3 directColor = vec3(0.0, 0.0, 0.0);
  vec3 indirectColor = vec3(0.0, 0.0, 0.0);

  //outDirectIr=vec4(1.0,0.0,0.0,1.0);
  outIndAlbedo=vec4(0.5,0.5,0.5,1.0);
  //outIndIr=vec4(0.5,0.5,0.0,1.0);
  outNormal=vec4(0.0,0.5,0.0,1.0);
  outWorldPos=vec4(0.0,0.0,1.0,1.0);

  vec4 preShadow;
  vec4 directAlbedo;
  vec4 indirectIr;
  vec4 indirectAlbedo;
  float avgShadow;
  fragPos.xy=getFragCoord(interpolatedPosition.xyz);

  ivec3 indices = ivec3(indexBuffer.data[3 * gl_PrimitiveID + 0], indexBuffer.data[3 * gl_PrimitiveID + 1], indexBuffer.data[3 * gl_PrimitiveID + 2]);

  vec3 vertexA = vec3(vertexBuffer.data[3 * indices.x + 0], vertexBuffer.data[3 * indices.x + 1], vertexBuffer.data[3 * indices.x + 2]);
  vec3 vertexB = vec3(vertexBuffer.data[3 * indices.y + 0], vertexBuffer.data[3 * indices.y + 1], vertexBuffer.data[3 * indices.y + 2]);
  vec3 vertexC = vec3(vertexBuffer.data[3 * indices.z + 0], vertexBuffer.data[3 * indices.z + 1], vertexBuffer.data[3 * indices.z + 2]);
  
  vec3 geometricNormal = normalize(cross(vertexB - vertexA, vertexC - vertexA));

  vec3 surfaceColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].diffuse;

  if(shadingMode.enable2thRMotion ==1){
            // 40 & 41 == light
  if (gl_PrimitiveID == 40 || gl_PrimitiveID == 41) {
    directColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission;
     //debugPrintfEXT("lightVertexA.x is %f  lightVertexA.y is %f lightVertexA.z is %f \n lightVertexB.x is %f  lightVertexB.y is %f lightVertexB.z is %f \n lightVertexC.x is %f  lightVertexC.y is %f lightVertexC.z is %f \n",vertexA.x,vertexA.y,vertexA.z,vertexB.x,vertexB.y,vertexB.z,vertexC.x,vertexC.y,vertexC.z);
  }
  else {
    preShadow=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy));   //pre shadow
    outDirectIr=preShadow;
    directAlbedo=imageLoad(image_directAlbedo,ivec2(gl_FragCoord.xy));
    indirectIr=imageLoad(image_indirectLgt,ivec2(gl_FragCoord.xy));
    outIndIr=indirectIr;
    indirectAlbedo=imageLoad(image_indirectLgt_2,ivec2(gl_FragCoord.xy));

    //int level=3;
    //avgShadow=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.x-level,gl_FragCoord.y)).w+imageLoad(image_directLgtIr,ivec2(gl_FragCoord.x+level,gl_FragCoord.y)).w+imageLoad(image_directLgtIr,ivec2(gl_FragCoord.x,gl_FragCoord.y-level)).w+imageLoad(image_directLgtIr,ivec2(gl_FragCoord.x,gl_FragCoord.y+level)).w;

    float alpha=0.9;
    if(preShadow.w==0.0){  // in shadow
        directColor=preShadow.xyz;
        vec4 previousShadowColor = imageLoad(image, ivec2(fragPos.xy));
        directColor=alpha*previousShadowColor.xyz+(1-alpha)*directColor.xyz;
    }
    else{
        directColor = directAlbedo.xyz * preShadow.xyz;    //not in shadow
    }
    //outIndAlbedo=vec4(directColor,1.0);

    
    if( preShadow.w==0.0 ){ //inshadow,weaken indirect light reflection
          indirectAlbedo.xyz=directAlbedo.xyz*0.5+indirectAlbedo.xyz*0.2;
          indirectIr.xyz*=0.25;
     }
     else if(avgShadow<4){  //around shadow
         indirectAlbedo.xyz=directAlbedo.xyz*0.5+indirectAlbedo.xyz*0.0;
         indirectIr.xyz*=0.3;
     }
     indirectColor=indirectIr.xyz*indirectAlbedo.xyz;
     
     //outIndIr=vec4(indirectColor,1.0);
  }
     vec4 color = vec4(directColor + indirectColor, 1.0);
     vec3 preFinalColor=imageLoad(image, ivec2(fragPos.xy)).xyz;
     if(length(materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission)==0){
         color.xyz=0.8*preFinalColor+0.2*color.xyz;
     }
     outColor = color;
 }
  else if(shadingMode.enableSVGF==1){
              // 40 & 41 == light
  if (gl_PrimitiveID == 40 || gl_PrimitiveID == 41) {
    directColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission;
     //debugPrintfEXT("lightVertexA.x is %f  lightVertexA.y is %f lightVertexA.z is %f \n lightVertexB.x is %f  lightVertexB.y is %f lightVertexB.z is %f \n lightVertexC.x is %f  lightVertexC.y is %f lightVertexC.z is %f \n",vertexA.x,vertexA.y,vertexA.z,vertexB.x,vertexB.y,vertexB.z,vertexC.x,vertexC.y,vertexC.z);
  }
  else {
    preShadow=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy));   //pre shadow
    preShadow.xyz=aTrous_directIr(gl_FragCoord.xy).xyz;
    outDirectIr=preShadow;
    directAlbedo=imageLoad(image_directAlbedo,ivec2(gl_FragCoord.xy));
    indirectIr=imageLoad(image_indirectLgt,ivec2(gl_FragCoord.xy));
    indirectIr.xyz=aTrous_indirectIr(gl_FragCoord.xy).xyz;
    outIndIr=indirectIr;
    indirectAlbedo=imageLoad(image_indirectLgt_2,ivec2(gl_FragCoord.xy));
    if(shadingMode.enableSVGF_withIndAlbedo==1){
        indirectAlbedo=aTrous_indirectAlbedo(gl_FragCoord.xy);
        outIndAlbedo=indirectAlbedo;
    }

    //int level=3;
    //avgShadow=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.x-level,gl_FragCoord.y)).w+imageLoad(image_directLgtIr,ivec2(gl_FragCoord.x+level,gl_FragCoord.y)).w+imageLoad(image_directLgtIr,ivec2(gl_FragCoord.x,gl_FragCoord.y-level)).w+imageLoad(image_directLgtIr,ivec2(gl_FragCoord.x,gl_FragCoord.y+level)).w;

    directColor = directAlbedo.xyz * preShadow.xyz;
    //outIndAlbedo=vec4(directColor,1.0);
    
     indirectColor=indirectIr.xyz*indirectAlbedo.xyz;
     
     //outIndIr=vec4(indirectColor,1.0);
  }
     vec4 color = vec4(directColor + indirectColor, 1.0);
     vec3 preFinalColor=imageLoad(image, ivec2(fragPos.xy)).xyz;
     if(length(materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission)==0){
         color.xyz=0.8*preFinalColor+0.2*color.xyz;
     }
     outColor = color;
  }
  //not enable2thray motion vector
  else{
      // 40 & 41 == light
  if (gl_PrimitiveID == 40 || gl_PrimitiveID == 41) {
    directColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission;
     //debugPrintfEXT("lightVertexA.x is %f  lightVertexA.y is %f lightVertexA.z is %f \n lightVertexB.x is %f  lightVertexB.y is %f lightVertexB.z is %f \n lightVertexC.x is %f  lightVertexC.y is %f lightVertexC.z is %f \n",vertexA.x,vertexA.y,vertexA.z,vertexB.x,vertexB.y,vertexB.z,vertexC.x,vertexC.y,vertexC.z);
  }
  else {
    preShadow=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy));   //pre shadow
    outDirectIr=preShadow;
    indirectIr=imageLoad(image_indirectLgt,ivec2(gl_FragCoord.xy));
    outIndIr=indirectIr;
    indirectAlbedo=imageLoad(image_indirectLgt_2,ivec2(gl_FragCoord.xy));

    int randomIndex = int(random(gl_FragCoord.xy) * 2 + 40);//40 is the light area
    vec3 lightColor = vec3(0.6, 0.6, 0.6);
    //vec3 lightPosition = lightVertexA * lightBarycentric.x + lightVertexB * lightBarycentric.y + lightVertexC * lightBarycentric.z;
    vec3 lightPosition=getRadomLightPosition(randomIndex);

    vec3 positionToLightDirection = normalize(lightPosition - interpolatedPosition);

    vec3 shadowRayOrigin = interpolatedPosition;
    vec3 shadowRayDirection = positionToLightDirection;
    float shadowRayDistance = length(lightPosition - interpolatedPosition) - 0.001f;

    preShadow=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy));   //pre shadow
    
    //shadow ray
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, shadowRayOrigin, 0.001f, shadowRayDirection, shadowRayDistance);
  
    while (rayQueryProceedEXT(rayQuery));

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
      //directColor = surfaceColor * lightColor * dot(geometricNormal, positionToLightDirection);    //not in shadow
      vec4 irrad=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy));
      float weight=length(irrad);
      if(shadingMode.enableShadowMotion==1){
         if(preShadow.w==0.0){  //pre in shadow
            directColor=preShadow.xyz;
        }
        else{
            directColor = surfaceColor * irrad.xyz;    //not in shadow
        }
      }
      else{  //unenable shadowmotion
        directColor=surfaceColor * lightColor * dot(geometricNormal, positionToLightDirection); 
      }
    }
    else {  //now in shadow
        if(shadingMode.enableShadowMotion==1 && preShadow.w==0.0){   //must in shadow
            vec3 irrad=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy)).xyz;
            directColor=irrad;
        }
        if(shadingMode.enableShadowMotion==1 && preShadow.w==1.0){  //contrast , believe pre ,so not in shadow
            directColor = surfaceColor * preShadow.xyz;
        }
        else{
            directColor=vec3(0.0,0.0,0.0);
        }
      
      isShadow=true;
    }
    fragPos.xy=getFragCoord(interpolatedPosition.xyz);
    if(shadingMode.enableShadowMotion==1 && isShadow==true && camera.frameCount > 0){
      //Implement shadow motion vector algorithm
      //vec4 prevFramePos=shadingMode.PrevProjectionMatrix*shadingMode.PrevViewMatrix*vec4(interpolatedPosition, 1.0);

      //fragPos=clipPos.xyz;
      
      
      //fragPos.xy/=clipPos.w;
      //fragPos.y=-fragPos.y;
      //fragPos.xy+=1;
      //fragPos.xy/=2;
      //fragPos.x*=camera.ViewPortWidth;
      //fragPos.y*=camera.ViewPortHeight;
      //fragPos.xy=floor(fragPos.xy)+0.5;
      
      vec4 previousColor = imageLoad(image, ivec2(fragPos.xy));
      //vec4 previousColor=vec4(0.0,0.0,0.0,1.0);

      vec4 worldPos=getWorldPos(gl_FragCoord.xyz);
      ///if( fragPos.y>1079){
      //  debugPrintfEXT("gl_FragCoord.x is %f  gl_FragCoord.y is %f \n worldPos.x is %f   worldPos.y is %f  clipPos.x is %f interpolatedPos.x is %f interpolatedPos.y is %f   clipPos.w is %f   \n\n",
      //  gl_FragCoord.x,gl_FragCoord.y, worldPos.x,worldPos.y,clipPos.x,interpolatedPosition.x,interpolatedPosition.y,clipPos.w);
      //}

      float alpha=0.9;
      
      int d=10;
      if(shadingMode.enableShadowMotion==1 && shadingMode.enableSVGF==1 && isShadow==true && camera.frameCount > 0){  //spatial filter
        
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
  //if(shadingMode.enableSVGF_withIndAlbedo==1){
     // rayDirection = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.xy)).xyz;;
 // }
  //else{
    rayDirection = getSampledReflectedDirection(interpolatedPosition.xyz,geometricNormal,gl_FragCoord.xy,camera.frameCount);
  //}
  
  
  vec3 previousNormal = geometricNormal;

  

  bool rayActive = true;
  int maxRayDepth = 1;
  if(shadingMode.enable2Ray==1){
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

      

        if(shadingMode.enable2thRMotion==0){
            rayQueryEXT rayQuery;
            rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, shadowRayOrigin, 0.001f, shadowRayDirection, shadowRayDistance);
      
             while (rayQueryProceedEXT(rayQuery));

        
            //secondary shadow ray
            if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
            indirectColor += (1.0 / (rayDepth + 1)) * extensionSurfaceColor * lightColor  * dot(previousNormal, rayDirection) * dot(extensionNormal, positionToLightDirection);
            }
            else {
                rayActive = false;
            }
        }
        else{
            indirectColor=imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy)).xyz*imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.xy)).xyz;
            if( preShadow.w==0.0 ){ //inshadow
                indirectColor*=0.125;
            }
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

  /*
  if (camera.frameCount > 0) {              //静止画面下使用前一帧像素降噪
    vec4 previousColor = imageLoad(image, ivec2(gl_FragCoord.xy));
    previousColor *= camera.frameCount;

    color += previousColor;
    color /= (camera.frameCount + 1);
  }
  */

  vec3 preFinalColor=imageLoad(image, ivec2(fragPos.xy)).xyz;
 if(shadingMode.enable2thRMotion==1 && preShadow.w==1 && length(materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission)==0){
    color.xyz=0.8*preFinalColor+0.2*color.xyz;
 }
  
  outColor = color;
  }


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
    float weight=0.7;  //reflection rate
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

float w_depth(vec2 p,vec2 q){   //weight of depth in the edge stop function in SVGF
    float sigma_z=1.0;  //1.0 in the SVGF paper
    float epsil=0.01;
    //calculate gradient
    vec2 grad_p;
    float right=imageLoad(image_Depth,ivec2(p.x+1,p.y)).x;
    float left=imageLoad(image_Depth,ivec2(p.x-1,p.y)).x;
    float up=imageLoad(image_Depth,ivec2(p.x,p.y+1)).x;
    float down=imageLoad(image_Depth,ivec2(p.x,p.y-1)).x;
    grad_p.x=0.5*right-0.5*left;
    grad_p.y=0.5*up-0.5*down;
    //load p,q
    float depth_p=imageLoad(image_Depth,ivec2(p.x,p.y)).x;
    float depth_q=imageLoad(image_Depth,ivec2(q.x,q.y)).x;
    //caculate weight
    float weight=exp(-(abs(depth_p-depth_q)/sigma_z*dot(grad_p,(p-q))+epsil));
    return weight;
}

float w_normal(vec2 p,vec2 q){   //weight of normal in the edge stop function in SVGF
    float sigma_n=128;
    vec3 n_p=2*imageLoad(image_normal,ivec2(p.xy)).xyz-1;
    vec3 n_q=2*imageLoad(image_normal,ivec2(q.xy)).xyz-1;
    float weight=pow(max(0,dot(n_p,n_q)),sigma_n);
    return weight;
}

float w_lumin(vec2 p,vec2 q){//weight of Luminance in the edge stop function in SVGF
    float sigma_l=4;
    float epsil=0.01;
    float lumin_p=length(imageLoad(image_directLgtIr,ivec2(p.xy)).xyz);
    float lumin_q=length(imageLoad(image_directLgtIr,ivec2(q.xy)).xyz);
    float weight=exp(-abs(lumin_p-lumin_q)/(sigma_l*variance(p).z+epsil));
    return weight;
}

vec4 variance(vec2 p){
    vec2 prevMoments=vec2(0.0f);
    float prevHistoryLen=0.0f;
    float lumin;
    float factor=0.001; //缩小到0~1.0的范围
    int cnt=0;  //num of the history moment
    float variance_out;
    vec3 normal_p=2*imageLoad(image_normal,ivec2(p.xy)).xyz-1;
    if(camera.frameCount==0){
        vec3 curDirectIr=imageLoad(image_directLgtIr,ivec2(p.xy)).xyz;
        lumin=0.2126*curDirectIr.x+0.7152*curDirectIr.y+0.0722*curDirectIr.z;
        prevMoments=vec2(lumin,lumin*lumin);
        variance_out=100.0f;
    }
    else{ //get fragPos
        vec2 prevPos=getFragCoord(interpolatedPosition);
        lumin=length(imageLoad(image_directLgtIr,ivec2(p.xy)).xyz);
        
        vec3 normal_tmp=2*imageLoad(image_normal,ivec2(p.x+1,p.y)).xyz-1;
        if(length(normal_p-normal_tmp)<0.3){  //valid history moment
            prevMoments+=imageLoad(image_Var,ivec2(prevPos.x+1,prevPos.y)).zw/factor;
            cnt++;
        }
        normal_tmp=2*imageLoad(image_normal,ivec2(p.x-1,p.y)).xyz-1;
        if(length(normal_p-normal_tmp)<0.3){  //valid history moment
            prevMoments+=imageLoad(image_Var,ivec2(prevPos.x-1,prevPos.y)).zw/factor;
            cnt++;
        }
        normal_tmp=2*imageLoad(image_normal,ivec2(p.x,p.y+1)).xyz-1;
        if(length(normal_p-normal_tmp)<0.3){  //valid history moment
            prevMoments+=imageLoad(image_Var,ivec2(prevPos.x,prevPos.y+1)).zw/factor;
            cnt++;
        }
        normal_tmp=2*imageLoad(image_normal,ivec2(p.x,p.y-1)).xyz-1;
        if(length(normal_p-normal_tmp)<0.3){  //valid history moment
            prevMoments+=imageLoad(image_Var,ivec2(prevPos.x,prevPos.y-1)).zw/factor;
            cnt++;
        }

        
    }
    if(cnt>0){
        prevMoments/=cnt;
    }
    float moment_alpha =max(1.0f/(cnt+1),0.5);
    //calculate accumulated moments
    float first_Moment=moment_alpha*prevMoments.x+(1-moment_alpha)*lumin;
    float second_Moment=moment_alpha*prevMoments.y+(1-moment_alpha)*lumin*lumin;
    outColor.zw=vec2(first_Moment*factor,second_Moment*factor);
    outColor.xy=outColor.zw;
    variance_out=second_Moment-first_Moment*first_Moment;
    variance_out=variance_out>0.0f?100.0f*variance_out:0.0f;

    return vec4(first_Moment,second_Moment,variance_out,1.0);
}

float weight(vec2 p,vec2 q){
     return  w_normal(p,q)*w_lumin(p,q);//w_depth(p,q)*
}

vec4 aTrous_indirectIr(vec2 p){
    vec4 Numerator=vec4(0.0,0.0,0.0,1.0);
    vec4 Denominator=vec4(0.0,0.0,0.0,1.0);

    float level=16;
    vec4 Ir_00 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y-level))*Ir_00;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y-level));

    vec4 Ir_01 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y-level))*Ir_01;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y-level));

    vec4 Ir_02 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y-level))*Ir_02;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y-level));

    vec4 Ir_10 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y))*Ir_10;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y));

    vec4 Ir_11 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));
    Numerator+=(1.0/4.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.xy))*Ir_11;
    Denominator+=(1.0/4.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.xy));

    vec4 Ir_12 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y))*Ir_12;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y));

    vec4 Ir_20 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y+level))*Ir_20;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y+level));

    vec4 Ir_21 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y+level))*Ir_21;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y+level));

    vec4 Ir_22 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y+level))*Ir_22;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y+level));

    //vec4 Ir=(1/4.0)*Ir_11+(1/8.0)*(Ir_01+Ir_10+Ir_12+Ir_21)+(1/16.0)*(Ir_00+Ir_02+Ir_20+Ir_22);

    return Numerator/Denominator;
}

vec4 aTrous_indirectAlbedo(vec2 p){
    vec4 Numerator=vec4(0.0,0.0,0.0,1.0);
    vec4 Denominator=vec4(0.0,0.0,0.0,1.0);

    float level=4;
    vec4 Ir_00 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y-level))*Ir_00;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y-level));

    vec4 Ir_01 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y-level))*Ir_01;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y-level));

    vec4 Ir_02 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y-level))*Ir_02;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y-level));

    vec4 Ir_10 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y))*Ir_10;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y));

    vec4 Ir_11 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.xy));
    Numerator+=(1.0/4.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.xy))*Ir_11;
    Denominator+=(1.0/4.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.xy));

    vec4 Ir_12 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y))*Ir_12;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y));

    vec4 Ir_20 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y+level))*Ir_20;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y+level));

    vec4 Ir_21 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y+level))*Ir_21;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y+level));

    vec4 Ir_22 = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y+level))*Ir_22;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y+level));

    //vec4 Ir=(1/4.0)*Ir_11+(1/8.0)*(Ir_01+Ir_10+Ir_12+Ir_21)+(1/16.0)*(Ir_00+Ir_02+Ir_20+Ir_22);

    return Numerator/Denominator;
}

vec4 aTrous_directIr(vec2 p){
    vec4 Numerator=vec4(0.0,0.0,0.0,1.0);
    vec4 Denominator=vec4(0.0,0.0,0.0,1.0);

    float level=16;
    vec4 Ir_00 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y-level))*Ir_00;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y-level));

    vec4 Ir_01 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y-level))*Ir_01;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y-level));

    vec4 Ir_02 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y-level))*Ir_02;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y-level));

    vec4 Ir_10 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y))*Ir_10;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y));

    vec4 Ir_11 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.xy));
    Numerator+=(1.0/4.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.xy))*Ir_11;
    Denominator+=(1.0/4.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.xy));

    vec4 Ir_12 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y))*Ir_12;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y));

    vec4 Ir_20 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y+level))*Ir_20;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x-level,gl_FragCoord.y+level));

    vec4 Ir_21 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
    Numerator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y+level))*Ir_21;
    Denominator+=(1.0/8.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x,gl_FragCoord.y+level));

    vec4 Ir_22 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
    Numerator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y+level))*Ir_22;
    Denominator+=(1.0/16.0)*weight(gl_FragCoord.xy,vec2(gl_FragCoord.x+level,gl_FragCoord.y+level));

    //vec4 Ir=(1/4.0)*Ir_11+(1/8.0)*(Ir_01+Ir_10+Ir_12+Ir_21)+(1/16.0)*(Ir_00+Ir_02+Ir_20+Ir_22);

    return Numerator/Denominator;
}