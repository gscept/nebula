//------------------------------------------------------------------------------
//  gather.fx
//
//	Composes light with color
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"
#include "lib/shared.fxh"

/// Declaring used textures
Texture2D AlbedoTexture;
Texture2D SpecularTexture;
Texture2D LightTexture;
Texture2D SSSTexture;
Texture2D SSAOTexture;
Texture2D EmissiveTexture;

/// Declaring used samplers
SamplerState DefaultSampler;

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
	float2 normalMapPixelSize = GetPixelSize(LightTexture);
	float2 screenUv = psComputeScreenCoord(Position.xy, normalMapPixelSize.xy);
	float4 sssLight = DecodeHDR(SSSTexture.Sample(DefaultSampler, screenUv));
	float4 light = DecodeHDR(LightTexture.Sample(DefaultSampler, screenUv));
	
	// blend non-blurred light with SSS light
	light.rgb = lerp(light.rgb, sssLight.rgb, sssLight.a);
	
	float4 albedoColor = AlbedoTexture.Sample(DefaultSampler, screenUv);
	float3 specularColor = SpecularTexture.Sample(DefaultSampler, screenUv);
	float3 emissiveColor = EmissiveTexture.Sample(DefaultSampler, screenUv);
	float4 color = albedoColor;
	
	float3 normedColor = normalize(light.xyz);
	float maxColor = max(max(normedColor.x, normedColor.y), normedColor.z);
	
	color.xyz *= light.rgb;
	float spec = light.a;
	color.xyz += saturate(specularColor * spec * normedColor) + emissiveColor;

	float4 ssao = SSAOTexture.Sample(DefaultSampler, screenUv);
	
	color.rgb *= 1 - saturate(ssao.rgb);
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