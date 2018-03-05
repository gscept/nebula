//------------------------------------------------------------------------------
//  lights.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"
#include "lib/defaultsampler.fxh"
#include "lib/shared.fxh"
#include "lib/CSM.fxh"

static const float specPower = float(32.0f);
static const float rimLighting = float(0.2f);
static const float exaggerateSpec = float(1.8f);
static const float3 luminanceValue = float3(0.299f, 0.587f, 0.114f);

cbuffer GlobalLightVars
{
	float4 GlobalBackLightColor;
	float4 GlobalLightColor;
	float4 GlobalAmbientLightColor;
	float4 GlobalLightDir;
	float GlobalBackLightOffset;
}

cbuffer LocalLightVars
{
	float4 LightColor;
	float4 LightPosRange;
	float LightShadowBias;
	float4x4 LightProjTransform;
}

cbuffer ShadowVars
{
	float4 CameraPosition;
	float4 ShadowOffsetScale = {0.0f, 0.0f, 0.0f, 0.0f};
	float4 ShadowConstants = {100.0f, 100.0f, 0.003f, 1024.0f};
	float4x4 ShadowProjTransform;
	float ShadowIntensity = {1.0f};
}

/// Declaring used textures
Texture2D NormalBuffer;
Texture2D DepthBuffer;
Texture2D LightProjMap;

SamplerState DefaultSampler;
SamplerState LightProjMapSampler
{
	Filter = 21;
	AddressU = 4;
	AddressV = 4;
	AddressW = 3;
	BorderColor = float4(0,0,0,0);
};

//---------------------------------------------------------------------------------------------------------------------------
//											GLOBAL LIGHT
//---------------------------------------------------------------------------------------------------------------------------

BlendState GlobBlend 
{
};

// we use the same rasterizer for both states
RasterizerState GlobRast
{
	CullMode = 2;
};

DepthStencilState GlobDepth
{
	DepthEnable = FALSE;
	DepthWriteMask = 0;
};


//------------------------------------------------------------------------------
/**
*/
void
vsGlob(float4 position : POSITION,
	float2 uv : TEXCOORD,
	out float3 ViewSpacePosition : TEXCOORD0,
	out float2 UV : TEXCOORD1,
	out float4 Position : SV_POSITION0) 
{
    Position = position;
    UV = uv; 
    ViewSpacePosition = float3(position.xy * FocalLength.xy, -1);
}

//------------------------------------------------------------------------------
/**
*/
void
psGlob(in float3 ViewSpacePosition : TEXCOORD0,
	in float2 UV : TEXCOORD1,
	in float4 Position : SV_POSITION0,
	uniform bool CastShadow,
	out float4 Color : SV_TARGET0) 
{
	float2 normalMapPixelSize = GetPixelSize(NormalBuffer);
	float2 screenUv = psComputeScreenCoord(Position.xy, normalMapPixelSize.xy);
	float3 ViewSpaceNormal = UnpackViewSpaceNormal(NormalBuffer.Sample(DefaultSampler, screenUv));
	float Depth = DepthBuffer.Sample(DefaultSampler, screenUv);
	
	clip(Depth);
	float NL = dot(GlobalLightDir.xyz, ViewSpaceNormal);
	float shadowFactor = 1.0f;
	if (CastShadow)
	{		
		if (NL > 0.1f)
		{
			float3 viewRay = normalize(ViewSpacePosition);
			float4 worldPos = float4(viewRay * Depth, 1);
			float4 texShadow;		
			CSMConvert(worldPos, texShadow);
			shadowFactor = CSMPS(texShadow,
								screenUv); 
			
			shadowFactor = lerp(1.0f, shadowFactor, ShadowIntensity);
		}		
	}

	float3 diff = GlobalAmbientLightColor.xyz;
	diff += GlobalLightColor.xyz * saturate(NL) * saturate(shadowFactor);
	diff += GlobalBackLightColor.xyz * saturate(-NL + GlobalBackLightOffset);   
	
	float3 H = normalize(GlobalLightDir.xyz - ViewSpacePosition);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float diffLuminance = dot(diff, luminanceValue) * exaggerateSpec;
	float spec = pow(NH, specPower) * diffLuminance;    
	
	diff += (float3(rimLighting, rimLighting, rimLighting) * saturate(NL - GlobalBackLightOffset));    
	
	Color = EncodeHDR(float4(diff, spec) );
}

//---------------------------------------------------------------------------------------------------------------------------
//											SPOT LIGHT
//---------------------------------------------------------------------------------------------------------------------------


BlendState SpotBlend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = ONE;
	DestBlend[0] = ONE;
	SrcBlendAlpha[0] = ONE;
	DestBlendAlpha[0] = ONE;
};

// we use the same rasterizer for both states
RasterizerState SpotRast
{
	CullMode = 3;
};

DepthStencilState SpotDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = 0;
	DepthFunc = 5;
};

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float 
GetInvertedOcclusionLocal(float receiverDepthInLightSpace,
                     float2 lightSpaceUv)
{

    // offset and scale shadow lookup tex coordinates
    lightSpaceUv.xy *= ShadowOffsetScale.zw;
    lightSpaceUv.xy += ShadowOffsetScale.xy;
	
    // apply bias
    const float shadowBias = LightShadowBias;
    float receiverDepth = ShadowConstants.x * receiverDepthInLightSpace - shadowBias;
	float occluder = ShadowProjMap.Sample(ShadowProjMapSampler, lightSpaceUv).r;
    float occluderReceiverDistance = occluder - receiverDepth;
    const float darkeningFactor = ShadowConstants.y;
    float occlusion = saturate(exp(darkeningFactor * occluderReceiverDistance));  
    return occlusion;
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
void
vsSpot(float4 position : POSITION,
	out float3 ViewSpacePosition : TEXCOORD0,
	out float4 Position : SV_POSITION0) 
{
	Position = mul(position, mul(Model, ViewProjection));
	ViewSpacePosition = mul(position, mul(Model, View)).xyz;
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
void
psSpot(in float3 ViewSpacePosition : TEXCOORD0,
	in float4 Position : SV_POSITION0,
	uniform bool CastShadow,
	out float4 Color : SV_TARGET0) 
{
	float2 pixelSize = GetPixelSize(DepthBuffer);
	float2 ScreenUV = psComputeScreenCoord(Position.xy, pixelSize.xy);
	float3 ViewSpaceNormal = UnpackViewSpaceNormal(NormalBuffer.Sample(DefaultSampler, ScreenUV));
	float Depth = DepthBuffer.Sample(DefaultSampler, ScreenUV);

	// remove pixels with negative depth
	clip(Depth);
	
	float3 viewVec = normalize(ViewSpacePosition);
	float3 surfacePos = viewVec * Depth;    
	float3 lightDir = (LightPosRange.xyz - surfacePos);
	
	float att = saturate(1.0 - length(lightDir) * LightPosRange.w);    
	clip(att - 0.004);    
	lightDir = normalize(lightDir);
	
	float4 projLightPos = mul(float4(surfacePos, 1.0f), LightProjTransform);
	clip(projLightPos.z - 0.001);
	float mipSelect = 0;
	float2 lightSpaceUv = float2(((projLightPos.xy / projLightPos.ww) * float2(0.5f, -0.5f)) + 0.5f);
	
	float3 lightModColor = LightProjMap.SampleLevel(LightProjMapSampler, lightSpaceUv, mipSelect);
	
	float NL = saturate(dot(lightDir, ViewSpaceNormal));
	float3 diff = LightColor.xyz * NL * att;
	
	float3 H = normalize(lightDir - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float diffLuminance = dot(diff, luminanceValue) * exaggerateSpec;
	float spec = pow(NH, specPower)* diffLuminance;
	
	float4 oColor = float4(lightModColor * diff, lightModColor.r * spec);
	
	float shadowFactor = 1.0f;    
	if (CastShadow)
	{
		float4 shadowProjLightPos = mul(float4(surfacePos, 1.0f), ShadowProjTransform);
		float2 shadowLookup = (shadowProjLightPos.xy / shadowProjLightPos.ww) * float2(0.5f, -0.5f) + 0.5f; 
		float receiverDepth = projLightPos.z / projLightPos.w;
		shadowFactor = GetInvertedOcclusionLocal(receiverDepth, shadowLookup);
		
		shadowFactor = lerp(1.0f, shadowFactor, ShadowIntensity);
	}
	Color = EncodeHDR(oColor * shadowFactor);
}

//---------------------------------------------------------------------------------------------------------------------------
//											POINT LIGHT
//---------------------------------------------------------------------------------------------------------------------------


BlendState PointBlend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = 2;
	DestBlend[0] = 2;
	SrcBlendAlpha[0] = 2;
	DestBlendAlpha[0] = 2;
};

// we use the same rasterizer for both states
RasterizerState PointRast
{
	CullMode = 3;
};

DepthStencilState PointDepthNoShadow
{
	DepthEnable = FALSE;
	DepthWriteMask = 0;
	DepthFunc = 5;
};

DepthStencilState PointDepthShadow
{
	DepthEnable = TRUE;
	DepthWriteMask = 0;
	DepthFunc = 5;
};

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
void
vsPoint(float4 position : POSITION,
	out float3 ViewSpacePosition : TEXCOORD0,
	out float4 Position : SV_POSITION0) 
{
	Position = mul(position, mul(Model, ViewProjection));
	ViewSpacePosition = mul(position, mul(Model, View)).xyz;
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
void
psPoint(in float3 ViewSpacePosition : TEXCOORD0,
	in float4 Position : SV_POSITION0,
	uniform bool CastShadow,
	out float4 Color : SV_TARGET0) 
{
	float2 pixelSize = GetPixelSize(DepthBuffer);
	float2 ScreenUV = psComputeScreenCoord(Position.xy, pixelSize.xy);
	float3 ViewSpaceNormal = UnpackViewSpaceNormal(NormalBuffer.Sample(DefaultSampler, ScreenUV));
	float Depth = DepthBuffer.Sample(DefaultSampler, ScreenUV);

	// remove pixels with negative depth
	clip(Depth);
	
	float3 viewVec = normalize(ViewSpacePosition);
	float3 surfacePos = viewVec * Depth;
	float3 lightDir = (LightPosRange.xyz - surfacePos);
	float att = saturate(1.0 - length(lightDir) * LightPosRange.w);
	att *= att;
	clip(att - 0.004);    
	lightDir = normalize(lightDir);
	
	float NL = saturate(dot(lightDir, ViewSpaceNormal));
	float3 diff = LightColor.xyz * NL * att;
	
	float3 H = normalize(lightDir - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float diffLuminance = dot(diff, luminanceValue) * exaggerateSpec;
	float spec = pow(NH, specPower) * diffLuminance;
	float4 output = float4(diff, spec);
	float shadowFactor = 1.0f;
	
	if (CastShadow)
	{
		float4 shadowProjLightPos = mul(float4(surfacePos, 1.0f), ShadowProjTransform);
		float2 shadowLookup = (shadowProjLightPos.xy / shadowProjLightPos.ww) * float2(0.5f, -0.5f) + 0.5f; 

		float receiverDepth = shadowProjLightPos.z / shadowProjLightPos.w;
		shadowFactor = GetInvertedOcclusionLocal(receiverDepth,
						    shadowLookup);
		
		shadowFactor = lerp(1.0f, shadowFactor, ShadowIntensity);
	}               
	Color = EncodeHDR(output * shadowFactor);
}
//------------------------------------------------------------------------------
/**
*/
VertexShader globVs = CompileShader(vs_5_0, vsGlob());
PixelShader globPs = CompileShader(ps_5_0, psGlob(false));
PixelShader globPsCSM = CompileShader(ps_5_0, psGlob(true));
VertexShader spotVs = CompileShader(vs_5_0, vsSpot());
PixelShader spotPsNoShadow = CompileShader(ps_5_0, psSpot(false));
PixelShader spotPsShadow = CompileShader(ps_5_0, psSpot(true));
VertexShader pointVs = CompileShader(vs_5_0, vsPoint());
PixelShader pointPsNoShadow = CompileShader(ps_5_0, psPoint(false));
PixelShader pointPsShadow = CompileShader(ps_5_0, psPoint(true));

technique11 GlobalLight < string Mask = "Light|Global"; >
{
	pass Main
	{
		SetBlendState(GlobBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(GlobDepth, 0);
		SetRasterizerState(GlobRast);
		SetVertexShader(globVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(globPs);
	}
}

technique11 GlobalLightCSM < string Mask = "Light|Global|Shadow"; >
{
	pass Main
	{
		SetBlendState(GlobBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(GlobDepth, 0);
		SetRasterizerState(GlobRast);
		SetVertexShader(globVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(globPsCSM);
	}
}

technique11 SpotNoShadow < string Mask = "Light|Spot"; >
{
	pass Main
	{
		SetBlendState(SpotBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(SpotDepth, 0);
		SetRasterizerState(SpotRast);
		SetVertexShader(spotVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(spotPsNoShadow);
	}
}

technique11 SpotShadow < string Mask = "Light|Spot|Shadow"; >
{
	pass Main
	{
		SetBlendState(SpotBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(SpotDepth, 0);
		SetRasterizerState(SpotRast);
		SetVertexShader(spotVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(spotPsShadow);
	}
}

technique11 PointNoShadow < string Mask = "Light|Point"; >
{
	pass Main
	{
		SetBlendState(PointBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(PointDepthNoShadow, 0);
		SetRasterizerState(PointRast);
		SetVertexShader(pointVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(pointPsNoShadow);
	}
}

technique11 PointShadow < string Mask = "Light|Point|Shadow"; >
{
	pass Main
	{
		SetBlendState(PointBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(PointDepthShadow, 0);
		SetRasterizerState(PointRast);
		SetVertexShader(pointVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(pointPsShadow);
	}
}