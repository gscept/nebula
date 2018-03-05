//------------------------------------------------------------------------------
//  billboard.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/util.fxh"

Texture2D DiffuseMap;

// simple sampler
SamplerState DefaultSampler
{
};

BlendState Blend 
{
	AlphaToCoverageEnable = true;
};

RasterizerState Rasterizer
{
	CullMode = 1;
};

DepthStencilState DepthStencil
{
};

//------------------------------------------------------------------------------
/**
*/
void
vsDefault(float2 position : POSITION,
	float2 uv : TEXCOORD,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD) 
{
    Position = mul(float4(position, 0, 1), mul(Model, ViewProjection));
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
psDefault(in float4 Position : SV_POSITION,
			in float2 UV : TEXCOORD,
			out float4 Albedo : SV_TARGET0,
			out float4 Unshaded : SV_TARGET1)
{
	// get diffcolor
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV.xy);
	
	//float alpha = diffColor.a;
	//clip( alpha < 0.05f ? -1:1 );
	
	Albedo = EncodeHDR(diffColor);
	Unshaded = float4(0,0,0,1);
}

//------------------------------------------------------------------------------
/**
*/
void
psPicking(in float4 Position : SV_POSITION0,
		out float Id : SV_TARGET0) 
{
	Id = (float)ObjectId;
}

//------------------------------------------------------------------------------
/**
*/
technique11 Default < string Mask = "Static|Unlit"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(CompileShader(vs_5_0, vsDefault()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psDefault()));
	}
}

technique11 Picking < string Mask = "Static|Picking"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(CompileShader(vs_5_0, vsDefault()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psPicking()));
	}
}