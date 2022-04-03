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
        /*
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
        */
        

        
        { /*
        //5*5 guassian
        vec4 preDirect_00 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-2,gl_FragCoord.y-2));
        vec4 preDirect_01 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-1,gl_FragCoord.y-2));
        vec4 preDirect_02 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y-2));
        vec4 preDirect_03 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+1,gl_FragCoord.y-2));
        vec4 preDirect_04 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+2,gl_FragCoord.y-2));
        vec4 r0=1*preDirect_00+4*preDirect_01+7*preDirect_02+4*preDirect_03+1*preDirect_04;

        vec4 preDirect_10 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-2,gl_FragCoord.y-1));
        vec4 preDirect_11 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-1,gl_FragCoord.y-1));
        vec4 preDirect_12 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y-1));
        vec4 preDirect_13 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+1,gl_FragCoord.y-1));
        vec4 preDirect_14 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+2,gl_FragCoord.y-1));
        vec4 r1=4*preDirect_10+16*preDirect_11+26*preDirect_12+16*preDirect_13+4*preDirect_14;

        vec4 preDirect_20 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-2,gl_FragCoord.y));
        vec4 preDirect_21 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-1,gl_FragCoord.y));
        vec4 preDirect_22 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y));
        vec4 preDirect_23 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+1,gl_FragCoord.y));
        vec4 preDirect_24 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+2,gl_FragCoord.y));
        vec4 r2=7*preDirect_20+26*preDirect_21+41*preDirect_22+26*preDirect_23+7*preDirect_24;

        vec4 preDirect_30 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-2,gl_FragCoord.y+1));
        vec4 preDirect_31 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-1,gl_FragCoord.y+1));
        vec4 preDirect_32 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y+1));
        vec4 preDirect_33 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+1,gl_FragCoord.y+1));
        vec4 preDirect_34 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+2,gl_FragCoord.y+1));
        vec4 r3=4*preDirect_30+16*preDirect_31+26*preDirect_32+16*preDirect_33+4*preDirect_34;

        vec4 preDirect_40 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-2,gl_FragCoord.y+2));
        vec4 preDirect_41 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-1,gl_FragCoord.y+2));
        vec4 preDirect_42 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y+2));
        vec4 preDirect_43 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+1,gl_FragCoord.y+2));
        vec4 preDirect_44 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+2,gl_FragCoord.y+2));
        vec4 r4=1*preDirect_40+4*preDirect_41+7*preDirect_42+4*preDirect_43+1*preDirect_44;

        vec4 total=(r0+r1+r2+r3+r4)/273;
        outDirectIr=total; //smoothed direct light
        */}


        int count=9;//同一平面上的数量
        mat3 isShadowSamePlane={
            vec3(1.0,1.0,1.0),
            vec3(1.0,1.0,1.0),
            vec3(1.0,1.0,1.0)
        };
        vec4 shadowNormal_11 = imageLoad(image_normal, ivec2(gl_FragCoord.xy));
        float level=10;
        //normal
         vec4 shadowNormal_00 = imageLoad(image_normal, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
         if(length(shadowNormal_11-shadowNormal_00)>0.7){ count--;   isShadowSamePlane[0][0]=0.0; }
        vec4 shadowNormal_01 = imageLoad(image_normal, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
        if(length(shadowNormal_11-shadowNormal_01)>0.7){ count--;   isShadowSamePlane[0][1]=0.0; }
        vec4 shadowNormal_02 = imageLoad(image_normal, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
        if(length(shadowNormal_11-shadowNormal_02)>0.7){ count--;   isShadowSamePlane[0][2]=0.0; }
        vec4 shadowNormal_10 = imageLoad(image_normal, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
        if(length(shadowNormal_11-shadowNormal_10)>0.7){ count--;   isShadowSamePlane[1][0]=0.0; }
        //vec4 shadowNormal_11 = imageLoad(image_normal, ivec2(gl_FragCoord.xy));
        vec4 shadowNormal_12 = imageLoad(image_normal, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
        if(length(shadowNormal_11-shadowNormal_12)>0.7){ count--;   isShadowSamePlane[1][2]=0.0; }
        vec4 shadowNormal_20 = imageLoad(image_normal, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
        if(length(shadowNormal_11-shadowNormal_20)>0.7){ count--;   isShadowSamePlane[2][0]=0.0; }
        vec4 shadowNormal_21 = imageLoad(image_normal, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
        if(length(shadowNormal_11-shadowNormal_21)>0.7){ count--;   isShadowSamePlane[2][1]=0.0; }
        vec4 shadowNormal_22 = imageLoad(image_normal, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
        if(length(shadowNormal_11-shadowNormal_22)>0.7){ count--;   isShadowSamePlane[2][2]=0.0; }

        if(count>=6&&count<9){
            level=4;
        }
        else if(count>=4&&count<6){
            level=2;
        }
        else{
            level=1;
        }

        vec4 preDirect_11 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.xy));
        float inShadow=0.0;
        if(preDirect_11.w==1.0){   //not in shadow point
            level=1;
            inShadow=1.0;
        }
        vec4 preDirect_00 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
        vec4 preDirect_01 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
        vec4 preDirect_02 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
        vec4 preDirect_10 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
        //vec4 preDirect_11 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.xy));
        vec4 preDirect_12 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
        vec4 preDirect_20 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
        vec4 preDirect_21 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
        vec4 preDirect_22 = imageLoad(image_directLgtIr, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
        //vec4 preDirect=preDirect_11+preDirect_01+preDirect_10+preDirect_12+preDirect_21+preDirect_00+preDirect_02+preDirect_20+preDirect_22;
        //preDirect/=9;
        vec4 preDirect;

        if(count==9){  preDirect=(1/4.0)*preDirect_11+(1/8.0)*(preDirect_01+preDirect_10+preDirect_12+preDirect_21)+(1/16.0)*(preDirect_00+preDirect_02+preDirect_20+preDirect_22);  }
        else{
            preDirect=preDirect_00*isShadowSamePlane[0][0]+preDirect_01*isShadowSamePlane[0][1]+preDirect_02*isShadowSamePlane[0][2]+
                    preDirect_10*isShadowSamePlane[1][0]+preDirect_11*isShadowSamePlane[1][1]+preDirect_12*isShadowSamePlane[1][2]+
                    preDirect_20*isShadowSamePlane[2][0]+preDirect_21*isShadowSamePlane[2][1]+preDirect_22*isShadowSamePlane[2][2];
            preDirect/=count;
        }

        outDirectIr=preDirect;   //direct shadow
        outDirectIr.w=inShadow;

        //outIndAlbedo=vec4(1.0,0.0,0.0,1.0);


        //filter indirect irradiance
        
        //

        count=9;//同一平面上的数量
        mat3 isSamePlane={
            vec3(1.0,1.0,1.0),
            vec3(1.0,1.0,1.0),
            vec3(1.0,1.0,1.0)
        };
        vec4 preInd_11 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));
        vec4 normal_11 = imageLoad(image_normal, ivec2(gl_FragCoord.xy));
        level=32;
        //normal
         vec4 normal_00 = imageLoad(image_normal, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
         if(length(normal_11-normal_00)>0.7){ count--;   isSamePlane[0][0]=0.0; }
        vec4 normal_01 = imageLoad(image_normal, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
        if(length(normal_11-normal_01)>0.7){ count--;   isSamePlane[0][1]=0.0; }
        vec4 normal_02 = imageLoad(image_normal, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
        if(length(normal_11-normal_02)>0.7){ count--;   isSamePlane[0][2]=0.0; }
        vec4 normal_10 = imageLoad(image_normal, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
        if(length(normal_11-normal_10)>0.7){ count--;   isSamePlane[1][0]=0.0; }
        //vec4 normal_11 = imageLoad(image_normal, ivec2(gl_FragCoord.xy));
        vec4 normal_12 = imageLoad(image_normal, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
        if(length(normal_11-normal_12)>0.7){ count--;   isSamePlane[1][2]=0.0; }
        vec4 normal_20 = imageLoad(image_normal, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
        if(length(normal_11-normal_20)>0.7){ count--;   isSamePlane[2][0]=0.0; }
        vec4 normal_21 = imageLoad(image_normal, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
        if(length(normal_11-normal_21)>0.7){ count--;   isSamePlane[2][1]=0.0; }
        vec4 normal_22 = imageLoad(image_normal, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
        if(length(normal_11-normal_22)>0.7){ count--;   isSamePlane[2][2]=0.0; }

        if(count>=6&&count<9){
            level=16;
        }
        else if(count>=4&&count<6){
            level=8;
        }
        else{
            level=4;
        }

        //float ShadowHit=1.0f;
        //if(preInd_11.w==0.0){   //2thShadow ray not Hit
        //    level=2;
        //    ShadowHit=0.0;
        //}
        vec4 preInd_00 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y-level));
        vec4 preInd_01 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y-level));
        vec4 preInd_02 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y-level));
        vec4 preInd_10 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y));
        //vec4 preInd_11 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));
        vec4 preInd_12 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y));
        vec4 preInd_20 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x-level,gl_FragCoord.y+level));
        vec4 preInd_21 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x,gl_FragCoord.y+level));
        vec4 preInd_22 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.x+level,gl_FragCoord.y+level));
        vec4 preInd;
        if(count==9){  preInd=(1/4.0)*preInd_11+(1/8.0)*(preInd_01+preInd_10+preInd_12+preInd_21)+(1/16.0)*(preInd_00+preInd_02+preInd_20+preInd_22);  }
        else{
            preInd=preInd_00*isSamePlane[0][0]+preInd_01*isSamePlane[0][1]+preInd_02*isSamePlane[0][2]+
                    preInd_10*isSamePlane[1][0]+preInd_11*isSamePlane[1][1]+preInd_12*isSamePlane[1][2]+
                    preInd_20*isSamePlane[2][0]+preInd_21*isSamePlane[2][1]+preInd_22*isSamePlane[2][2];
            preInd/=count;
        }
        //vec4 preInd=preInd_11+preInd_01+preInd_10+preInd_12+preInd_21+preInd_00+preInd_02+preInd_20+preInd_22;
        //preInd/=9;
        

        //preInd-=0.05*preDirect_11;
        //if(pre.x!=0.0){
        //    debugPrintfEXT("preDirect_11.x is %f  1-preDirect_11 .x is %f \n",pre.x,preDirect_11.x);
        //}

        outIndIr=preInd;   //Ind irradiance
        //outIndIr.w=ShadowHit;
}