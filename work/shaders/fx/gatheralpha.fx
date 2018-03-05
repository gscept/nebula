//------------------------------------------------------------------------------
//  gatheralpha.fx
//
//	Composes light with color for alpha
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"
#include "lib/shared.fxh"

/// Declaring used textures
Texture2D AlphaAlbedoTexture;
Texture2D AlphaSpecularTexture;
Texture2D AlphaLightTexture;
Texture2D AlphaEmissiveTexture;
//Texture2D AlphaUnlitTexture;

/// Declaring used samplers
SamplerState DefaultSampler;

BlendState Blend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = 5;
	DestBlend[0] = 6;
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 2;
};

DepthStencilState DepthStencil
{
	DepthWriteMask = 0;
};

//------------------------------------------------------------------------------
/**
*/
void
vsMain(float4 position : POSITION,
	float2 uv0 : TEXCOORD0,
	out float4 Position : SV_POSITION0,
	out float2 UV : TEXCOORD0) 
{
	Position = position;
	UV = uv0;
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(in float4 Position : SV_POSITION0,
	in float2 UV : TEXCOORD0,
	out float4 MergedColor : SV_TARGET0) 
{

	float2 normalMapPixelSize = GetPixelSize(AlphaLightTexture);
	float2 screenUv = psComputeScreenCoord(Position.xy, normalMapPixelSize.xy);
	float4 light = DecodeHDR(AlphaLightTexture.Sample(DefaultSampler, screenUv));
	float4 albedoColor = AlphaAlbedoTexture.Sample(DefaultSampler, screenUv);
	float3 specularColor = AlphaSpecularTexture.Sample(DefaultSampler, screenUv);
	float3 emissiveColor = AlphaEmissiveTexture.Sample(DefaultSampler, screenUv);
	//float3 unlitColor = AlphaUnlitTexture.Sample(DefaultSampler, screenUv) * 2;
	float4 color = albedoColor;
	float3 normedColor = normalize(light.xyz);
	float maxColor = max(max(normedColor.x, normedColor.y), normedColor.z);
	normedColor /= maxColor;
	color.xyz *= light.xyz;
	float spec = light.w;
	
	color.xyz += saturate(specularColor * spec * normedColor) + emissiveColor;

	//color.rgb = lerp(unlitColor.rgb, color.rgb, color.a);
	MergedColor = EncodeHDR(color);
}

//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsMain());
PixelShader ps = CompileShader(ps_5_0, psMain());

technique11 Default
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