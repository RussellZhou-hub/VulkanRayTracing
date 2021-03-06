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

struct Vertex{
  vec3 pos;
  vec3 normal;
  vec3 color;
  vec2 texCoord;
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
layout(binding = 3, set = 0) buffer VertexBuffer { Vertex data[]; } vertexBuffer;
layout(binding = 4, set = 0, rgba32f) uniform image2D image;
layout(binding = 6, set = 0, rgba32f) uniform image2D image_indirectLgt;
layout(binding = 7, set = 0, rgba32f) uniform image2D image_indirectLgt_2;  //ind albedo
layout(binding = 8, set = 0, rgba32f) uniform image2D image_directLgtIr;
layout(binding = 9, set = 0, rgba32f) uniform image2D image_directAlbedo;
layout(binding = 10, set = 0, rgba32f) uniform image2D image_normal;
layout(binding = 11, set = 0, rgba32f) uniform image2D image_worldPos;
layout(binding = 12, set = 0, r32f) uniform image2D image_Depth;
layout(binding = 13, set = 0, rgba32f) uniform image2D image_Var;  //historic Variance

layout(binding = 14) uniform sampler2D texSampler;

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
  uint groundTruth;
} shadingMode;

layout(binding = 0, set = 1) buffer MaterialIndexBuffer { uint data[]; } materialIndexBuffer;
layout(binding = 1, set = 1) buffer MaterialBuffer { Material data[]; } materialBuffer;

float random(vec2 uv, float seed);
float random(vec2 p);
float random_1(vec2 uv, float seed);
float avgBrightness(vec3 color);
vec2 getFragCoord(vec3 pos);
vec4 getWorldPos(vec3 fragPos);
bool isLight(vec3 emission);
vec3 getRadomLightPosition(int randomIndex);
vec3 getReflectedDierction(vec3 inRay,vec3 normal );
vec3 getSampledReflectedDirection(vec3 inRay,vec3 normal,vec2 uv,float seed);
vec3 getSpatial_SampledReflectedDirection(vec3 inPos,vec3 normal,vec2 uv,float seed);
vec3 uniformSampleHemisphere(vec2 uv);
vec3 alignHemisphereWithCoordinateSystem(vec3 hemisphere, vec3 up);





void main() {
  vec3 directColor = vec3(0.0, 0.0, 0.0);
  vec3 indirectColor = vec3(0.0, 0.0, 0.0);
  vec3 indirectIrrad = vec3(0.0, 0.0, 0.0);
  vec3 indirectalbedo = vec3(0.0, 0.0, 0.0);
  outColor.xyz=directColor;
  outIndIr=vec4(directColor,0.0);
  vec4 curClipPos;

  vec4 preShadow=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy));   //pre shadow

  ivec3 indices = ivec3(indexBuffer.data[3 * gl_PrimitiveID + 0], indexBuffer.data[3 * gl_PrimitiveID + 1], indexBuffer.data[3 * gl_PrimitiveID + 2]);

  vec3 vertexA =vertexBuffer.data[indices.x].pos;
  vec3 vertexB=vertexBuffer.data[indices.y].pos;
  vec3 vertexC=vertexBuffer.data[indices.z].pos;

  //vec3 vertexA = vec3(vertexBuffer.data[3 * indices.x + 0], vertexBuffer.data[3 * indices.x + 1], vertexBuffer.data[3 * indices.x + 2]);
  //vec3 vertexB = vec3(vertexBuffer.data[3 * indices.y + 0], vertexBuffer.data[3 * indices.y + 1], vertexBuffer.data[3 * indices.y + 2]);
  //vec3 vertexC = vec3(vertexBuffer.data[3 * indices.z + 0], vertexBuffer.data[3 * indices.z + 1], vertexBuffer.data[3 * indices.z + 2]);
  
  vec3 geometricNormal = normalize(cross(vertexB - vertexA, vertexC - vertexA));
  outNormal=vec4((geometricNormal+1)/2,1.0);  //[0???1]?????????

  //if(gl_FragCoord.x>1979){
  //  vec3 AB=vertexB - vertexA;
  //  vec3 AC=vertexC - vertexA;
  //  debugPrintfEXT("geometricNormal.x is %f  geometricNormal.y is %f \n AB.x is %f   AB.y is %f  AC.x is %f AC.y is %f AC.z is %f\n\n",geometricNormal.x,geometricNormal.y, AB.x,AB.y,AC.x,AC.y,AC.z);
  //}

  curClipPos=camera.projMatrix*camera.viewMatrix*vec4(interpolatedPosition,1.0);
  curClipPos.xyz/=curClipPos.w;
  curClipPos.y=-curClipPos.y;
  outWorldPos=curClipPos;

  vec3 surfaceColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].diffuse;

  // 40 & 41 == light
  if (isLight(materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission) || materialIndexBuffer.data[gl_PrimitiveID]==-1) {
    directColor = materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].emission;
    outColor.xyz=directColor;
    outDirectIr=vec4(0.6,0.6,0.6,1.0);
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

    outColor=vec4(surfaceColor,1.0);  //direct albedo
    
    //shadow ray
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, shadowRayOrigin, 0.001f, shadowRayDirection, shadowRayDistance);
  
    while (rayQueryProceedEXT(rayQuery));

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
      
      directColor = surfaceColor * lightColor * dot(geometricNormal, positionToLightDirection);
      vec4 irrad=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy));
      float weight=length(irrad);
      if(shadingMode.enableShadowMotion==1){
        if(preShadow.w==0.0){  //pre in shadow
            directColor=preShadow.xyz;
            outDirectIr=preShadow;
        }
        else{//pre not in shadow
            outDirectIr.xyz=lightColor * dot(geometricNormal, positionToLightDirection);
            outDirectIr.w=1.0;
        }
      }
      else{  //unenable shadowmotion
        //directColor=surfaceColor * lightColor * dot(geometricNormal, positionToLightDirection); 
        directColor = surfaceColor * lightColor * dot(geometricNormal, positionToLightDirection);    //not in shadow
        outDirectIr.xyz=lightColor * dot(geometricNormal, positionToLightDirection);  //irradiance of the direct light
        outDirectIr.w=1.0;
      }
      
    }
    else {
      directColor = vec3(0.0, 0.0, 0.0);                     //in shadow
      outDirectIr=vec4(directColor,0.0);
      isShadow=true;

        if(shadingMode.enableShadowMotion==1 && preShadow.w==0.0){   //must in shadow
            vec3 irrad=imageLoad(image_directLgtIr,ivec2(gl_FragCoord.xy)).xyz;
            outDirectIr.xyz=directColor;
            outDirectIr.w=0.0;
        }
        if(shadingMode.enableShadowMotion==1 && preShadow.w==1.0){  //contrast , believe pre ,so not in shadow
            directColor = surfaceColor * preShadow.xyz;
            outDirectIr.xyz=lightColor * dot(geometricNormal, positionToLightDirection);
            outDirectIr.w=1.0;
        }
        else{
            directColor=vec3(0.0,0.0,0.0);
        }
    }
    fragPos.xy=getFragCoord(interpolatedPosition.xyz);
    //if( fragPos.y>1079){
    //    debugPrintfEXT("gl_FragCoord.x is %f  gl_FragCoord.y is %f \n fragPos.x is %f   fragPos.y is %f  clipPos.x is %f interpolatedPos.x is %f interpolatedPos.y is %f   clipPos.w is %f   \n\n",
    //    gl_FragCoord.x,gl_FragCoord.y, fragPos.x,fragPos.y,clipPos.x,interpolatedPosition.x,interpolatedPosition.y,clipPos.w);
     // }

    if(shadingMode.enableShadowMotion==1 && isShadow==true){
      //Implement shadow motion vector algorithm
      //vec4 prevFramePos=shadingMode.PrevProjectionMatrix*shadingMode.PrevViewMatrix*vec4(interpolatedPosition, 1.0);

      //fragPos=clipPos.xyz;
      
      
     // fragPos.xy/=clipPos.w;
      //fragPos.y=-fragPos.y;
      //fragPos.xy+=1;
      //fragPos.xy/=2;
      //fragPos.x*=camera.ViewPortWidth;
      //fragPos.y*=camera.ViewPortHeight;
      //fragPos.x-=500;
      //fragPos.xy=floor(fragPos.xy)+0.5;
      
      vec4 previousColor = clamp(imageLoad(image_directLgtIr, ivec2(fragPos.xy)),0.0,0.2);
      //vec4 previousColor=vec4(0.0,0.0,0.0,1.0);

      vec4 worldPos=getWorldPos(gl_FragCoord.xyz);
      

      float alpha=0.9;
      
      int d=10;
      if(shadingMode.enableSVGF==1 && isShadow==true){  //spatial filter
        
        // bilinear filter  ??????????????? ????????????
        
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
        
        //if((variance.x+variance.y+variance.z)/3>0){  //?????? noisy
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
        outDirectIr.xyz=directColor;
      }
    }
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
      
      vec3 extensionVertexA =vertexBuffer.data[extensionIndices.x].pos;
      vec3 extensionVertexB=vertexBuffer.data[extensionIndices.y].pos;
      vec3 extensionVertexC=vertexBuffer.data[extensionIndices.z].pos;

      //vec3 extensionVertexA = vec3(vertexBuffer.data[3 * extensionIndices.x + 0], vertexBuffer.data[3 * extensionIndices.x + 1], vertexBuffer.data[3 * extensionIndices.x + 2]);
      //vec3 extensionVertexB = vec3(vertexBuffer.data[3 * extensionIndices.y + 0], vertexBuffer.data[3 * extensionIndices.y + 1], vertexBuffer.data[3 * extensionIndices.y + 2]);
      //vec3 extensionVertexC = vec3(vertexBuffer.data[3 * extensionIndices.z + 0], vertexBuffer.data[3 * extensionIndices.z + 1], vertexBuffer.data[3 * extensionIndices.z + 2]);
    
      vec3 extensionPosition = extensionVertexA * extensionBarycentric.x + extensionVertexB * extensionBarycentric.y + extensionVertexC * extensionBarycentric.z;
      vec3 extensionNormal = normalize(cross(extensionVertexB - extensionVertexA, extensionVertexC - extensionVertexA));

      vec3 extensionSurfaceColor = materialBuffer.data[materialIndexBuffer.data[extensionPrimitiveIndex]].diffuse;

      if (isLight(materialBuffer.data[materialIndexBuffer.data[extensionPrimitiveIndex]].emission) ||materialIndexBuffer.data[gl_PrimitiveID]==-1) {
        indirectColor += (1.0 / (rayDepth + 1)) * materialBuffer.data[materialIndexBuffer.data[extensionPrimitiveIndex]].emission * dot(previousNormal, rayDirection);
        indirectIrrad+=(1.0 / (rayDepth + 1)) *  dot(previousNormal, rayDirection);
        indirectalbedo+=(1.0 / (rayDepth + 1)) * materialBuffer.data[materialIndexBuffer.data[extensionPrimitiveIndex]].emission;
        outIndIr=vec4(indirectColor,1.0);
        outIndAlbedo==vec4(indirectalbedo,1.0);
      }
      else {
        float beta_indirect=0;

        

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
            //vec4 total=imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));;
            //outIndIr=vec4(beta_indirect*total.xyz+(1-beta_indirect)*rayDirection,1.0); //direction of the 2thRay
        }

        RayHitPointFragCoord=getFragCoord(extensionPosition.xyz);


        int randomIndex = int(random(gl_FragCoord.xy, camera.frameCount + rayDepth) * 2 + 40);
        vec3 lightColor = vec3(0.6, 0.6, 0.6);

        ivec3 lightIndices = ivec3(indexBuffer.data[3 * randomIndex + 0], indexBuffer.data[3 * randomIndex + 1], indexBuffer.data[3 * randomIndex + 2]);

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
          indirectIrrad+=(1.0 / (rayDepth + 1))*lightColor  * dot(previousNormal, rayDirection) * dot(extensionNormal, positionToLightDirection);
          indirectalbedo+=extensionSurfaceColor;
          outIndIr.w=1.0f;
        }
        else {
          rayActive = false;
        }

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

  if(camera.frameCount==0){
      //outColor=vec4(rayDirection,1.0);
  }

  if(shadingMode.enable2thRMotion==1){
    vec3 preIndIr=imageLoad(image_indirectLgt, ivec2(fragPos.xy)).xyz;
    vec3 preIndAlbedo=imageLoad(image_indirectLgt_2, ivec2(fragPos.xy)).xyz;
    vec3 black=vec3(0.0,0.0,0.0);
    float beta=0.9;
    indirectIrrad=beta*preIndIr+(1-beta)*indirectIrrad;
    indirectalbedo=beta*preIndAlbedo+(1-beta)*indirectalbedo;
  }

  outIndIr.xyz=indirectIrrad;
  outIndAlbedo.xyz=indirectalbedo;

  //outColor = color;
}


float random(vec2 p) 
{ 
    vec2 K1 = vec2(
     23.14069263277926, // e^pi (Gelfond's constant) 
     2.665144142690225 // 2^sqrt(2) (Gelfond????????Schneider constant) 
    ); 
    return fract(cos(dot(p,K1)) * 12345.6789); 
} 

float random(vec2 uv, float seed) {     // 0???1????????????
  return fract(sin(mod(dot(uv, vec2(12.9898, 78.233)) + 1113.1 * seed, M_PI)) * 43758.5453);
}

float random_1(vec2 uv, float seed) {     // -1???1????????????
  return pow(-1,mod(seed,1))*fract(sin(mod(dot(uv, vec2(12.9898, 78.233)) + 1113.1 * seed, M_PI)) * 43758.5453);
}

float avgBrightness(vec3 color){
    float avg=color.x+color.y+color.z;
    return avg/3;
}

vec2 getFragCoord(vec3 pos){          //?????????????????????????????????????????????????????????
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

bool isLight(vec3 emission){
    if(emission.x>0||emission.y>0||emission.z>0) return true;
    else return false;
}

vec3 getRadomLightPosition(int randomIndex){

    vec3 lightVertexA = camera.lightA.xyz;
    vec3 lightVertexB = camera.lightB.xyz;
    vec3 lightVertexC = camera.lightC.xyz;

    vec2 uv = vec2(random(gl_FragCoord.xy, camera.frameCount), random(vec2(gl_FragCoord.y,gl_FragCoord.x), camera.frameCount + 1));
    if (uv.x + uv.y > 1.0f) {
      uv.x = 1.0f - uv.x;
      uv.y = 1.0f - uv.y;
    }

    vec3 lightBarycentric = vec3(1.0 - uv.x - uv.y, uv.x, uv.y);
    vec3 lightPosition = lightVertexA * lightBarycentric.x + lightVertexB * lightBarycentric.y + lightVertexC * lightBarycentric.z;
    return lightPosition;
}

vec3 getReflectedDierction(vec3 inRay,vec3 normal ){     //????????????
    inRay=normalize(inRay);
    normal=normalize(normal);
    vec3 outRay = inRay - 2*dot(inRay,normal)*normal;
    return normalize(outRay);
}

vec3 getSampledReflectedDirection(vec3 inRay,vec3 normal,vec2 uv,float seed){
    inRay=inRay-camera.position.xyz;
    //vec3 Ray=getReflectedDierction(inRay,normal);
    vec3 Ray=reflect(inRay,normal);
    //float theta=0.5*M_PI*random(uv);  //[0,pi/2]
    float theta=acos(1-random(uv));  //[0,pi/2]
    float phi=2*M_PI*random(vec2(uv.y,uv.x));
    //theta*=sin(theta);
    vec3 RandomRay=vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));
    //vec3 RandomRay=uniformSampleHemisphere(uv);
    float weight=0.5;  //reflection rate
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

