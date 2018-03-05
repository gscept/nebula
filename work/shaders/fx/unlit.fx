//------------------------------------------------------------------------------
//  unlit.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/util.fxh"

cbuffer UnlitVars
{
	float Brightness = {0.0f};
}


/// Declaring used textures
Texture2D DiffuseMap;

/// Declaring used samplers
SamplerState DefaultSampler;

BlendState AlphaBlend
{
	BlendEnable[0] = true;
	SrcBlend[0] = 5;
	DestBlend[0] = 6;
};

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
vsMain(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float2 UV : TEXCOORD0,
	out float4 Position : SV_POSITION0) 
{
    Position = mul(position, mul(Model, ViewProjection));
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(in float2 UV : TEXCOORD0,
	in float4 Position : SV_POSITION0,
	out float4 Albedo : SV_TARGET0,
	out float4 Unshaded : SV_TARGET1) 
{
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV.xy) * Brightness;
	float alpha = diffColor.a;
	clip( alpha < AlphaSensitivity ? -1:1 );
	Albedo = EncodeHDR(diffColor);
	Unshaded = float4(0,0,0,1);    
}

//------------------------------------------------------------------------------
/**
*/
void
psMainAlpha(in float2 UV : TEXCOORD0,
	in float4 Position : SV_POSITION0,
	out float4 Albedo : SV_TARGET0,
	out float4 Unshaded : SV_TARGET1) 
{
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV.xy) * Brightness;
	float alpha = diffColor.a;
	clip( alpha < AlphaSensitivity ? -1:1 );
	Albedo = EncodeHDR(diffColor * AlphaBlendFactor);
	Unshaded = float4(0,0,0,1);
}


//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsMain());
PixelShader ps = CompileShader(ps_5_0, psMain());
PixelShader psAlpha = CompileShader(ps_5_0, psMainAlpha());

technique11 Solid < string Mask = "Static|Unlit"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}

technique11 Alpha < string Mask = "Alpha|Unlit"; >
{
	pass Main
	{
		SetBlendState(AlphaBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(psAlpha);
	}
}