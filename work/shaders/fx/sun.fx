//------------------------------------------------------------------------------
//  sun.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/util.fxh"
#include "lib/shared.fxh"

cbuffer SunVars
{
	float4 SunColor = {1,1,1,1};
	float GodrayScale = 25.0f;
}


/// Declaring used textures
Texture2D SunTexture;

/// Declaring used samplers
SamplerState DefaultSampler;

BlendState Blend 
{
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 1;
};

DepthStencilState DepthStencil
{
	DepthEnable = FALSE;
	DepthWriteMask = 0;
};

//------------------------------------------------------------------------------
/**
*/
void
vsGeometry(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION0,
	out float2 UV : TEXCOORD0) 
{
	Position = mul(position, mul(Model, ViewProjection));
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsGodrays(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION0,
	out float3 ViewSpacePos : VIEWSPACEPOS,
	out float3 Tangent : TANGENT,
	out float3 Normal : NORMAL,
	out float3 Binormal : BINORMAL,
	out float2 UV : TEXCOORD0) 
{
	float4x4 modelView = mul(Model, View);
    Tangent  = mul(tangent, modelView).xyz;
    Normal   = mul(normal, modelView).xyz;
    Binormal = mul(binormal, modelView).xyz;
	ViewSpacePos = mul(position, modelView).xyz;    
	float4 pos = float4(position.xyz * GodrayScale, 1);
	Position = mul(pos, mul(Model, ViewProjection));
	UV = uv;
}


//------------------------------------------------------------------------------
/**
	Render the actual godray stuff, we do this by simply rendering the color and texture
*/
void
psGodray(in float4 Position : SV_POSITION0,
	in float3 ViewSpacePos : VIEWSPACEPOS,
	in float3 Tangent : TANGENT,
	in float3 Normal : NORMAL,
	in float3 Binormal : BINORMAL,
	in float2 UV : TEXCOORD0,
	out float4 Color : SV_TARGET0) 
{
	// calculate freznel effect to give a smooth linear falloff of the godray effect
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent), normalize(Binormal), normalize(Normal));   
	float3 tNormal = float3(0,0,0);
	tNormal.xy = float2(0,0);
	tNormal.z = 1;
	float3 worldSpaceNormal = mul(tNormal, tangentViewMatrix).xyz;
	float rim = pow(abs(abs(dot(normalize(ViewSpacePos.xyz), worldSpaceNormal))), 3);
	
	float4 sunSample = SunTexture.Sample(DefaultSampler, UV);
	Color = sunSample * SunColor * rim * 0.14f;
	Color.a = 1.0f;
	Color = EncodeHDR(Color);
}

//------------------------------------------------------------------------------
/**
	Render the sun geometry, this needs to be here because we don't want to see the skybox
	We intentionally render it completely white which gives a nice bloom effect
*/
void
psGeometry(in float4 Position : SV_POSITION0,
	in float2 UV : TEXCOORD0,
	out float4 Color : SV_TARGET0) 
{
	float4 sunSample = SunTexture.Sample(DefaultSampler, UV);
	Color = float4(1,1,1,0) * sunSample * 10.0f;
	Color.a = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
technique11 FirstPass < string Mask = "Godrays"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(CompileShader(vs_5_0, vsGodrays()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psGodray()));
	}
}

technique11 SecondPass < string Mask = "Sun"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(CompileShader(vs_5_0, vsGeometry()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psGeometry()));
	}
}