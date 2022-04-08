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

layout(location = 0) out vec4 outColor;    //this renderpass: variance
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


float weight(vec2 p,vec2 q);
float w_depth(vec2 p,vec2 q);
float w_normal(vec2 p,vec2 q);
float w_lumin(vec2 p,vec2 q);
vec4 variance(vec2 p);
vec4 aTrous_indirectIr(vec2 p);
vec4 aTrous_indirectAlbedo(vec2 p);
vec4 aTrous_directIr(vec2 p);

vec2 getFragCoord(vec3 pos);

void main() {
        
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

outIndAlbedo = imageLoad(image_indirectLgt_2, ivec2(gl_FragCoord.xy));

if(shadingMode.enableSVGF==1){
            outDirectIr=aTrous_directIr(gl_FragCoord.xy);
            outIndIr=aTrous_indirectIr(gl_FragCoord.xy);
            if(shadingMode.enableSVGF_withIndAlbedo==1){
                outIndAlbedo = aTrous_indirectAlbedo(gl_FragCoord.xy);
            }
        }
else if(shadingMode.enableShadowMotion==1){
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
}
        
if(shadingMode.enable2thRMotion==1){
            //filter indirect irradiance
            int count =9;//同一平面上的数量
            float level=32;
            mat3 isSamePlane={
                vec3(1.0,1.0,1.0),
                vec3(1.0,1.0,1.0),
                vec3(1.0,1.0,1.0)
            };
            vec4 preInd_11 = imageLoad(image_indirectLgt, ivec2(gl_FragCoord.xy));
            vec4 normal_11 = imageLoad(image_normal, ivec2(gl_FragCoord.xy));
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
}
            //outIndAlbedo=vec4(1.0,0.0,0.0,1.0);
            //outIndIr.w=ShadowHit;
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

    float level=4;
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

    float level=4;
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
