//------------------------------------------------------------------------------
//  picking.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/skinning.fxh"


BlendState Blend 
{
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 2;
};

DepthStencilState DepthStencil
{
};

//------------------------------------------------------------------------------
/**
*/
void
vsStatic(float4 position : POSITION,
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
vsSkinned(float4 position : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 tangent : TANGENT,
	float4 binormal : BINORMAL,
	float4 weights : BLENDWEIGHT,
	uint4 indices : BLENDINDICES,
	out float4 Position : SV_POSITION0,
	out float2 UV : TEXCOORD0) 
{
	float4 skinnedPos = SkinnedPosition(position, weights, indices);
	Position = mul(skinnedPos, mul(Model, ViewProjection));
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsStaticInst(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	uint ID : SV_InstanceID,
	out float4 Position : SV_POSITION0,
	out float2 UV : TEXCOORD0) 
{
	Position = mul(position, mul(ModelArray[ID], ViewProjection));
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
psStatic(in float4 Position : SV_POSITION0,
		out float Id : SV_TARGET0) 
{
	Id = (float)ObjectId;
}

//------------------------------------------------------------------------------
/**
*/
VertexShader staticVs = CompileShader(vs_5_0, vsStatic());
VertexShader skinnedVs = CompileShader(vs_5_0, vsSkinned());
VertexShader staticInstVs = CompileShader(vs_5_0, vsStaticInst());
PixelShader id = CompileShader(ps_5_0, psStatic());

technique11 Static < string Mask = "Static|Picking"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(staticVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(id);
	}
}


technique11 Skinned < string Mask = "Skinned|Picking"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(skinnedVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(id);
	}
}

technique11 StaticInstanced < string Mask = "Static|Instanced|Picking"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(staticInstVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(id);
	}
}

