//------------------------------------------------------------------------------
//  ssr_cs.fx
//  (C) 2015 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

sampler2D DepthBuffer;
sampler2D ColorBuffer;
sampler2D SpecularBuffer;
sampler2D NormalBuffer;
readwrite rgba16f image2D EmissiveBuffer;

const float SearchDist = 5.0f;
const float SearchDistInv = 0.2f;
const int MaxBinarySearchSteps = 5;
const int MaxSteps = 50;
const float RayStep = 0.25f;
const float MinRayStep = 0.001f;
vec2 Resolution;
vec2 InvResolution;

samplerstate LinearState
{
	Samplers = {DepthBuffer, SpecularBuffer, NormalBuffer};
	Filter = Point;
	BorderColor = {0,0,0,0};
	AddressU = Border;
	AddressV = Border;
};

samplerstate ColorState
{
	Samplers = {ColorBuffer};
	BorderColor = {1,0,0,0};
	AddressU = Border;
	AddressV = Border;
};

#define KERNEL_RADIUS 16
#define HALF_KERNEL_RADIUS (KERNEL_RADIUS/2.0f)

#define SSR_TILE_WIDTH 320
#define SHARED_MEM_SIZE (KERNEL_RADIUS + SSR_TILE_WIDTH + KERNEL_RADIUS)
//groupshared vec4 SharedMemory[SSR_TILE_WIDTH];
groupshared float SharedDepth[32*32];

//------------------------------------------------------------------------------
/**
*/
vec2 
ConvertProjToTexCoord(in vec4 projCoord)
{
	vec2 res;
	res.xy = (projCoord.xy / projCoord.ww);
	//res.y = 1 - res.y;
	return res;
}

//------------------------------------------------------------------------------
/**
*/
vec2
RayTrace2D(in vec3 startDir, in vec3 startPos, in float startDepth, in ivec2 UV)
{
	mat4 trans;
	vec3 reflection = startPos;
	vec2 projCoord = ConvertProjToTexCoord(trans * vec4(reflection, 1.0f));
	float bufferDepth = textureLod(DepthBuffer, projCoord, 0).r;
	float reflectionDepth = length(reflection);
	
	for (int i = 0; i < MaxSteps; i++)
	{
		if (bufferDepth < reflectionDepth)
		{
			float delta = bufferDepth - reflectionDepth;
			if (delta < 0.03f)	break;
		}
		reflection += startDir;
		projCoord = ConvertProjToTexCoord(trans * vec4(reflection, 1.0f));
		bufferDepth = textureLod(DepthBuffer, projCoord, 0).r;
		reflectionDepth = length(reflection);
	}
	return vec2(-1);
}

//------------------------------------------------------------------------------
/**
*/

[localsizex] = 32
[localsizey] = 32
shader
void
csMain()
{
	ivec2 PixelCoord = ivec2(gl_GlobalInvocationID.xy);
	vec2 ScreenCoord = PixelCoord * InvResolution;
	vec2 UV = ScreenCoord + vec2(0.5f) * InvResolution;
	//UV.y = 1 - UV.y;
	
	float Depth = textureLod(DepthBuffer, UV, 0).r;
	vec4 Spec = textureLod(SpecularBuffer, UV, 0);
	
	// create view vector into frustum
	vec3 viewVec = normalize(vec3(UV * FocalLength.xy, -1));
	
	// calculate surface position so we know where to start tracing the ray
	vec3 surfacePos = viewVec * Depth;
	
	// calculate reflection vector
	vec3 viewSpaceNormal = UnpackViewSpaceNormal(textureLod(NormalBuffer, UV, 0));
	vec3 reflection = normalize(reflect(viewVec, viewSpaceNormal));
	
	// trace ray
	vec2 coord = RayTrace2D(-reflection * MinRayStep, surfacePos, Depth, PixelCoord);
	float modulate = saturate(dot(-reflection, viewVec));
	//float modulate = 0.1f;

	// blend with already existing emissive
	vec4 color = textureLod(ColorBuffer, coord, 0) * Spec;
	vec4 emissive = imageLoad(EmissiveBuffer, PixelCoord);
	groupMemoryBarrier();
	imageStore(EmissiveBuffer, PixelCoord, lerp(emissive, color, modulate) + vec4(reflection.xyz / 10, 0));
	//imageStore(EmissiveBuffer, PixelCoord, textureLod(ColorBuffer, UV, 0) * modulate);
}

//------------------------------------------------------------------------------
/**
*/
program SSR [ string Mask = "Alt0"; ]
{
	ComputeShader = csMain();
};
