//------------------------------------------------------------------------------
//  combine.fx
//  (C) 2020 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"

texture2D Fog;
texture2D Reflections;
readwrite rgba16f image2D Lighting;

group(BATCH_GROUP) constant CombineUniforms [ string Visibility = "CS"; ]
{
	vec2 LowresResolution;
};

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void csCombine()
{
	vec2 coord = vec2(gl_GlobalInvocationID.xy) * LowresResolution;
	ivec2 fullscaleCoord = ivec2(gl_GlobalInvocationID.xy);
	vec4 fog = texture(sampler2D(Fog, PosteffectUpscaleSampler), coord);
	vec4 reflections = texture(sampler2D(Reflections, PosteffectUpscaleSampler), coord);
	vec4 light = imageLoad(Lighting, fullscaleCoord);

	//vec4 res = vec4(mix(light.rgb, fog.rgb, fog.a), 1);
	vec3 res = 
		light.rgb * fog.a 
		//+ reflections.rgb * fog.a 
		+ fog.rgb;
	imageStore(Lighting, fullscaleCoord, vec4(res, 1));
}

//------------------------------------------------------------------------------
/**
*/
program Combine [ string Mask = "Combine"; ]
{
	ComputeShader = csCombine();
};
