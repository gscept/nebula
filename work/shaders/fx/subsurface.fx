//------------------------------------------------------------------------------
//  subsurface.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/defaultsampler.fxh"
#include "lib/geometrybase.fxh"

Texture2D AbsorptionMap;
Texture2D ScatterMap;

cbuffer SSSVars
{
	float SubsurfaceStrength  = {0.0f};
	float SubsurfaceWidth = {0.0f};
	float SubsurfaceCorrection = {0.0f};
}


DepthStencilState GeometryDepthStencil
{
	DepthFunc = 4;
};


//------------------------------------------------------------------------------
/**
	Renders Subsurface geometry to buffers.
	Writes absorption, scattering and mask
*/
void
psSkin(float3 ViewSpacePos : TEXCOORD0,
	float3 Tangent : TANGENT0,
	float3 Normal : NORMAL0,
	float4 Position : SV_POSITION0,
	float3 Binormal : BINORMAL0,
	float2 UV : TEXCOORD1,
	out float4 absorption : SV_TARGET0,
	out float4 scatter : SV_TARGET1,
	out float4 mask : SV_TARGET2)
{
	// sample textures
	absorption = AbsorptionMap.Sample(DefaultSampler, UV);
	scatter = ScatterMap.Sample(DefaultSampler, UV);
	mask.a = 255;
	mask.r = SubsurfaceStrength;
	mask.g = SubsurfaceWidth;
	mask.b = SubsurfaceCorrection;
}
//------------------------------------------------------------------------------
/**
*/
technique11 Skinned < string Mask = "Skinned"; >
{
	pass Main
	{
		SetBlendState(SolidBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(GeometryDepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(CompileShader(vs_5_0, vsSkinned()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psSkin()));
	}
}

technique11 Static < string Mask = "Static"; >
{
	pass Main
	{
		SetBlendState(SolidBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(GeometryDepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(CompileShader(vs_5_0, vsStatic()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psSkin()));
	}
}