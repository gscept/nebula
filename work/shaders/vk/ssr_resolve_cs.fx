//------------------------------------------------------------------------------
//  ssr_resolve_cs.fx
//  (C) 2020 Fredrik Lindahl
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"
#include "lib/pbr.fxh"

//texture2D LightBuffer;
texture2D TraceBuffer;
//read image2D TraceBuffer;

write rgba16f image2D ReflectionBuffer;

samplerstate LinearState
{
	//Samplers = {DepthBuffer, SpecularBuffer, NormalBuffer, AlbedoBuffer};
	Filter = Linear;
	BorderColor = {0,0,0,0};
	AddressU = Border;
	AddressV = Border;
};

samplerstate NoFilterState
{
	//Samplers = {DepthBuffer, SpecularBuffer, NormalBuffer, AlbedoBuffer};
	Filter = Point;
	BorderColor = {0,0,0,0};
	AddressU = Border;
	AddressV = Border;
};

//------------------------------------------------------------------------------
/**
*/
[localsizex] = 32
[localsizey] = 32
shader
void
csMain()
{
	ivec2 location = ivec2(gl_GlobalInvocationID.xy);
    const vec2 screenSize = imageSize(ReflectionBuffer);
	if (location.x >= screenSize.x || location.y > screenSize.y)
		return;

	vec2 invScreenSize = vec2(1.0/screenSize.x, 1.0/screenSize.y);

	vec2 UV = location * invScreenSize;

	vec3 rayResult = texelFetch(sampler2D(TraceBuffer, NoFilterState), location / 2, 0).xyz;
	vec3 color = vec3(0);//sample2DLod(LightBuffer, LinearState, UV, 0).xyz;
	if (rayResult.r >= 0.0f)
	{
		vec4 albedo = sample2DLod(AlbedoBuffer, LinearState, UV, 0);
		vec4 material = sample2DLod(SpecularBuffer, LinearState, UV, 0);

		vec3 F0 = vec3(0.04);
		CalculateF0(albedo.rgb, material[0], F0);

    	vec3 F = FresnelSchlickGloss(F0, max(rayResult.b, 0.0), material[1]);

		//vec3 fresnel;
		//vec3 brdf;
		//CalculateBRDF(1, cosTheta, cosTheta, 1, material[1], F0, fresnel, brdf);

		vec3 reflection = sample2DLod(LightBuffer, LinearState, rayResult.xy, 0).xyz;
		
		//vec3 kD = vec3(1.0f) - F;
		//kD *= 1.0f - material[0];
		
		color += (reflection * F) * material[2];
	}
	
	imageStore(ReflectionBuffer, location, color.xyzx);
}

//------------------------------------------------------------------------------
/**
*/
program SSR [ string Mask = "Alt0"; ]
{
	ComputeShader = csMain();
};
