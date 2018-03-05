//------------------------------------------------------------------------------
//  clouds.fx
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/preetham.fxh"

sampler2D CloudLayer1;
sampler2D CloudLayer2;
sampler2D CloudLayer3;
vec4 CloudSizes;
vec4 CloudThicknesses;
vec4 CloudThickColor = vec4(0.4f, 0.4f, 0.4f, 0.4f);


samplerstate CloudSampler
{
	Samplers = { CloudLayer1, CloudLayer2, CloudLayer3 };
	//MaxAnisotropic = 16;
	//Filter = Anisotropic;
	AddressU = Mirror;
	AddressV = Mirror;
};

state CloudState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	CullMode = None;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Equal;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsClouds([slot=0] in vec2 position,
		out vec4 WorldPos,
	out vec4 FollowWorldPos,
	out int Instance) 
{
	vec4 pos = vec4(position.x, EyePos.y + 100.0f + gl_InstanceID * 25.0f, position.y, 1);
	FollowWorldPos = pos + vec4(EyePos.x, 0, EyePos.z, 0);
	gl_Position = Projection * View * FollowWorldPos;
	WorldPos = pos;
	Instance = gl_InstanceID;
}

const vec2 InstanceOffsets[] =
{

	vec2(-0.25f, 0.75),
	vec2(-2f, 1),
	vec2(2, 4),
	vec2(-5, 3),
	vec2(1, 0.75)
};


//------------------------------------------------------------------------------
/**
*/
shader
void
psClouds(
	in vec4 WorldPos, 
	in vec4 FollowWorldPos,
	flat in int Instance,
	[color0] out vec4 Color)
{
	
	// calculate atmospheric contribution
	vec3 lightDir = normalize(GlobalLightDir.xyz);
	vec3 dir = normalize(WorldPos.xyz);
	vec3 atmo = Preetham(dir, lightDir, A, B, C, D, E, Z);
	
	vec2 InstanceOffset = InstanceOffsets[Instance % 5];
	
	// calculate distance from center so we fade out toward horizon
	float len = 1.0f - smoothstep(1.0f, 750.0f, distance(EyePos.xz, FollowWorldPos.xz));
	vec2 uv1 = (WorldPos.xz / vec2(CloudSizes.x) + InstanceOffset) + vec2(0.5, -0.3) * TimeAndRandom.x * 0.05f; 
	vec2 uv2 = (WorldPos.xz / vec2(CloudSizes.y) + InstanceOffset) + vec2(0.2, 0.5) * TimeAndRandom.x * 0.007f; 
	vec2 uv3 = (WorldPos.xz / vec2(CloudSizes.z) + InstanceOffset) + vec2(-0.3, 0.2) * TimeAndRandom.x * 0.009f; 
	float layer1 = texture(CloudLayer1, uv1).r;
	float layer2 = texture(CloudLayer2, uv2).r;
	float layer3 = texture(CloudLayer3, uv3).r;
	
	layer1 = saturate(layer1 * 2 - sin(TimeAndRandom.x * 0.05f)) * 2;
	layer2 = saturate(layer2 * 2 - cos(TimeAndRandom.x * 0.007f)) * 2;
	layer3 = saturate(layer3 * 2 - sin(TimeAndRandom.x * 0.009f)) * 2;
	
	float fade1 = smoothstep(0.0f, CloudThicknesses.x, layer1);
	float fade2 = smoothstep(0.0f, CloudThicknesses.y, layer2);
	float fade3 = smoothstep(0.0f, CloudThicknesses.z, layer3);
	float wet1 = smoothstep(CloudThicknesses.x, 1.0f, layer1);
	float wet2 = smoothstep(CloudThicknesses.y, 1.0f, layer2);
	float wet3 = smoothstep(CloudThicknesses.z, 1.0f, layer3);
	
	Color = lerp(vec4(GlobalLightColor.rgb * atmo, 0), vec4(1), fade1 * fade2 * fade3) * lerp(vec4(1), CloudThickColor, wet1 * wet2 * wet3);
	Color.a *= len;
	
	//float depth = smoothstep(0.0f, max(max(layer1, layer2), layer3) * 4, layer1 * layer2 * 2.0f * layer3 * 2.0f);
	//float depth = saturate(pow(layer1 * layer2 * layer3, 4.0f));
	
	// lerp between global light color towards the outer rims, toward a thick dark grey cloud in the center
	//vec4 color = lerp(vec4(GlobalLightColor.rgb, 0), CloudThickColor, depth);
	//color.rgb = color.rgb * atmo;
	//vec2 line = (vec2(cos(WorldPos.x / GridSize), cos(WorldPos.z / GridSize)) - vec2(0.90, 0.90));
	//float c = saturate(max(line.x, line.y)) * 5;
	
	//c = smoothstep(0.5f, 1, c);
	//Color = vec4(color.rgb, depth * len);
	gl_FragDepth = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsClouds(), psClouds(), CloudState);
