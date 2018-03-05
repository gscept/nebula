#ifndef LIGHTUTILS_FXH
#define LIGHTUTILS_FXH


float4 ShadowOffsetScale       : ShadowOffsetScale;
float4 ShadowConstants         : ShadowConstants = {100.0f, 100.0f, 0.003f, 512.0f};
float  ShadowIntensity         : ShadowIntensity = 5.0f;

Texture2D  LightBuffer         		: LightBuffer;
Texture2D  ShadowProjMap       		: ShadowProjMap;

float4 FocalLength 			   : FocalLength;

const SamplerState lightProjMapSampler
{
    //Texture     = <lightProjMap>;
    AddressU    = Border;
    AddressV    = Border;
    BorderColor = float4(0,0,0,0);
	Filter = MIN_MAG_MIP_LINEAR;
};					

SamplerState shadowProjMapSampler
{
    //Texture     = <shadowProjMap>;
    AddressU    = Border;
    AddressV    = Border;
    BorderColor = float4(1,1,1,1);
	Filter = MIN_MAG_MIP_LINEAR;
};
//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float 
GetInvertedOcclusion(float receiverDepthInLightSpace,
                     float2 lightSpaceUv)
{
    // offset and scale shadow lookup tex coordinates
    lightSpaceUv.xy *= ShadowOffsetScale.zw;
    lightSpaceUv.xy += ShadowOffsetScale.xy;
    
    // apply bias
    const float shadowBias = ShadowConstants.z;
    float receiverDepth = ShadowConstants.x * receiverDepthInLightSpace - shadowBias;


	float occluder = ShadowProjMap.Sample(ShadowProjMapSampler, lightSpaceUv);

    float occluderReceiverDistance = occluder - receiverDepth;
    const float darkeningFactor = ShadowConstants.y;
    float occlusion = saturate(exp(darkeningFactor * occluderReceiverDistance));  
                 
    return occlusion;
}

#endif