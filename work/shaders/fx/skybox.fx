//------------------------------------------------------------------------------
//  skybox.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/util.fxh"

// sky stuff
cbuffer SkyVars
{
	float Contrast = 1.0f;
	float Brightness = 1.0f;
	float SkyBlendFactor = 0.0f;
}


// declare two textures, one main texture and one blend texture together with a wrapping sampler
TextureCube SkyLayer1;
TextureCube SkyLayer2;
SamplerState SkySampler
{
	AddressU = Wrap;
	AddressV = Wrap;
	Filter = MIN_MAG_LINEAR_MIP_POINT;
};

BlendState Blend 
{
};

// invert cull mode to get front faces!
RasterizerState Rasterizer
{
	CullMode = 3;
};


DepthStencilState DepthStencil
{
	DepthFunc = 2;
	DepthWriteMask = 0;
};

//------------------------------------------------------------------------------
/**
    Sky box vertex shader
*/
void
vsMain(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float3 UV : TEXCOORD)
{
	float3 tempPos = normalize(position.xyz);
	Position = mul(float4(tempPos, 1), Projection);	
	UV = mul(View, tempPos);
	float animationSpeed = Time * 0.03f;
	float3x3 rotMat = { cos(animationSpeed), 0, sin(animationSpeed),
						0, 1, 0,
						-sin(animationSpeed), 0, cos(animationSpeed)};
	UV = mul(rotMat, UV);
}

//------------------------------------------------------------------------------
/**
    Sky box pixel shader
*/
void
psMain(float4 Position : SV_POSITION,
	float3 UV : TEXCOORD,
	out float4 Color : SV_TARGET0)
{
	// rotate uvs around center with constant speed
	float4 baseColor = SkyLayer1.Sample(SkySampler, UV);
	float4 blendColor = SkyLayer2.Sample(SkySampler, UV);
	float4 color = lerp(baseColor, blendColor, SkyBlendFactor);
	color.rgb = ((color.rgb - 0.5f) * Contrast) + 0.5f;
	color.rgb *= Brightness;
		
	Color = color;
}

//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsMain());
PixelShader ps = CompileShader(ps_5_0, psMain());

technique11 Default < string Mask = "Static"; >
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