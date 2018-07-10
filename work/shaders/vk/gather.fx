//------------------------------------------------------------------------------
//  gather.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

varblock GatherBlock
{
	/// Declaring used textures
	textureHandle LightTexture;
	textureHandle DepthTexture;
	textureHandle EmissiveTexture;
	textureHandle SSSTexture;
	textureHandle SSAOTexture;
};

samplerstate GatherSampler
{
	//Samplers = { LightTexture, SSSTexture, SSAOTexture, DepthTexture };
	Filter = Point;
	AddressU = Border;
	AddressV = Border;
	BorderColor = { 0, 0, 0, 0 };
};

state GatherState
{
	CullMode = None;
	DepthEnabled = false;
	DepthWrite = false;
};

//------------------------------------------------------------------------------
/**
    Compute fogging given a sampled fog intensity value from the depth
    pass and a fog color.
*/
vec4 
psFog(float fogDepth, vec4 color)
{
    float fogIntensity = clamp((FogDistances.y - fogDepth) / (FogDistances.y - FogDistances.x), FogColor.a, 1.0);
    return vec4(lerp(FogColor.rgb, color.rgb, fogIntensity), color.a);
}

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
	vec4 sssLight = DecodeHDR(sample2DLod(SSSTexture, GatherSampler, UV, 0));
	vec4 light = DecodeHDR(sample2DLod(LightTexture, GatherSampler, UV, 0));
	vec4 emissiveColor = sample2DLod(EmissiveTexture, GatherSampler, UV, 0);
	float ssao = sample2DLod(SSAOTexture, GatherSampler, UV, 0).r;
	
	// blend non-blurred light with SSS light
	light.rgb = lerp(light.rgb + emissiveColor.rgb, sssLight.rgb, sssLight.a) * (1.0f - ssao);	
	vec4 color = light;
	
	float depth = sample2DLod(DepthTexture, GatherSampler, UV, 0).r;
	color = psFog(depth, color);
	MergedColor = EncodeHDR(light);	
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), GatherState);
