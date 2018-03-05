//------------------------------------------------------------------------------
//  gather.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"
#include "lib/preetham.fxh"

vec4 FogDistances = vec4(0.0, 2500.0, 0.0, 1.0);
vec4 FogColor = vec4(0.5, 0.5, 0.63, 0.0);

/// Declaring used textures
sampler2D LightTexture;
sampler2D DepthTexture;
sampler2D EmissiveTexture;
sampler2D NormalTexture;
sampler2D SSSTexture;
sampler2D SSAOTexture;

samplerstate GatherSampler
{
	Samplers = { LightTexture, SSSTexture, SSAOTexture, DepthTexture, EmissiveTexture, NormalTexture };
	Filter = Point;
	AddressU = Border;
	AddressV = Border;
	BorderColor = { 0, 0, 0, 0 };
};

state GatherState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};

//------------------------------------------------------------------------------
/**
    Compute fogging given a sampled fog intensity value from the depth
    pass and a fog color.
*/
vec4 
psFog(float fogDepth, vec4 color, vec3 atmo)
{
    float fogIntensity = clamp((FogDistances.y - fogDepth) / (FogDistances.y - FogDistances.x), FogColor.a, 1.0);
    return vec4(lerp(FogColor.rgb * atmo, color.rgb, fogIntensity), color.a);
}

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
	vec4 sssLight = DecodeHDR(textureLod(SSSTexture, UV, 0));
	vec4 light = DecodeHDR(textureLod(LightTexture, UV, 0));
	vec4 emissiveColor = textureLod(EmissiveTexture, UV, 0);
	vec3 ViewSpaceNormal = UnpackViewSpaceNormal(texture(NormalTexture, UV));
	vec3 atmo = Preetham(ViewSpaceNormal, GlobalLightDir.xyz, A, B, C, D, E);
	float ssao = textureLod(SSAOTexture, UV, 0).r;
	
	// blend non-blurred light with SSS light, make sure emissive is multiplied by atmosphere
	vec4 color = vec4(lerp(light.rgb + emissiveColor.rgb * light.a, sssLight.rgb, sssLight.a) * (1.0f - ssao), 1.0f);
	
	float depth = textureLod(DepthTexture, UV, 0).r;
	color = psFog(depth, color, saturate(atmo * ONE_OVER_PI));
	MergedColor = EncodeHDR(color);
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), GatherState);
