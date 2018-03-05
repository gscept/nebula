//------------------------------------------------------------------------------
//  gatheralpha.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

/// Declaring used textures
sampler2D AlphaAlbedoTexture;
sampler2D AlphaLightTexture;
//sampler2D AlphaUnlitTexture;
sampler2D AlphaEmissiveTexture;

samplerstate TextureSampler
{
	Samplers = { AlphaAlbedoTexture, AlphaLightTexture };
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
vsMain(in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	gl_Position = vec4(position, 1);
	UV = FlipY(uv);
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
	vec4 light = DecodeHDR(textureLod(AlphaLightTexture, UV, 0));
	vec4 albedoColor = textureLod(AlphaAlbedoTexture, UV, 0);
	vec4 emissiveColor = DecodeHDR(textureLod(AlphaEmissiveTexture, UV, 0));
	
	vec4 color = vec4((light.rgb + emissiveColor.rgb * light.a), 1.0f);
	
	color.a = albedoColor.a;
	MergedColor = EncodeHDR(color);
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), AlphaGatherState);