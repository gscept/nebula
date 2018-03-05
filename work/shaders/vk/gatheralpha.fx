//------------------------------------------------------------------------------
//  gatheralpha.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

/// Declaring used textures
textureHandle AlphaAlbedoTexture;
textureHandle AlphaLightTexture;
textureHandle AlphaEmissiveTexture;
//sampler2D AlphaUnlitTexture;


samplerstate TextureSampler
{
	//Samplers = { AlphaAlbedoTexture, AlphaLightTexture };
	Filter = Point;
};

state AlphaGatherState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	gl_Position = vec4(position, 1);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psMain(in vec2 UV,
	[color0] out vec4 MergedColor) 
{	
	vec4 light = DecodeHDR(textureLod(sampler2D(Textures2D[AlphaLightTexture], TextureSampler), UV, 0));
	vec4 albedoColor = textureLod(sampler2D(Textures2D[AlphaAlbedoTexture], TextureSampler), UV, 0);
	vec4 emissiveColor = DecodeHDR(texture(sampler2D(Textures2D[AlphaEmissiveTexture], TextureSampler), UV));
	
	vec4 color = light + emissiveColor;
	
	color.a = albedoColor.a;
	MergedColor = EncodeHDR(color);
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), AlphaGatherState);