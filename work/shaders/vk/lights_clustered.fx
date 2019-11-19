//------------------------------------------------------------------------------
//  lights_clustered.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"

// make sure this matches LightContext
#define CLUSTER_SUBDIVS_X 64
#define CLUSTER_SUBDIVS_Y 64
#define CLUSTER_SUBDIVS_Z 16
#define MAX_LIGHTS_PER_CLUSTER 32

#define LIGHT_TYPE__DIRECTIONAL 0
#define LIGHT_TYPE__POINTLIGHT 1
#define LIGHT_TYPE__SPOTLIGHT 2
#define LIGHT_TYPE__AREALIGHT 3

struct ClusterAABB
{
	vec4 maxPoint;
	vec4 minPoint;
};

group(BATCH_GROUP) varbuffer ClusterAABBs
{
	ClusterAABB AABBs[];
};

// note, this is just the information we require from the light to perform the tiling, and doesn't contain the lights themselves
struct Light
{
	uint type;		// type of light, look at above definitions for a mapping
    float radius;	// radius of sphere (pointlight) or cone angle (spotlight)
	vec4 position;	// world space position of light
	vec4 forward;	// forward vector of light (spotlight and arealights)
};

// do not modify this one, keep it the same, its being fed through the lightserver
group(BATCH_GROUP) varblock LightList
{
	Light Lights[1024];
};

// this is the output list, pointing to an index i in the Input.lights buffer and next to the next element in Output.list. 
struct LightTileList
{
	uint numLights;
	uint lightIndex[MAX_LIGHTS_PER_CLUSTER];
};

// this is the buffer we want to modify!
group(BATCH_GROUP) varbuffer LightIndexList
{
	LightTileList Lists[];
};

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) varblock Uniforms
{
    uint NumInputLights;
	float ZPerspectiveCorrection;
    vec2 InvFramebufferDimensions;
	uvec2 BlockSize;

	uvec3 NumCells;
	float Fov;
};

//------------------------------------------------------------------------------
/**
*/
bool TestAABBSphere(ClusterAABB aabb, Light light)
{
	/*

	ivec3 mask0 = ivec3(lessThan(light.position, aabb.minPoint).xyz);
	ivec3 mask1 = ivec3(greaterThan(light.position, aabb.maxPoint).xyz);
	vec3 distMin = aabb.minPoint.xyz - light.position.xyz;
	distMin *= distMin * mask0;
	vec3 distMax = light.position.xyz - aabb.maxPoint.xyz;
	distMax *= distMax * mask1;
	float dist = distMin.x + distMin.y + distMin.z + distMax.x + distMax.y + distMax.z;
	return dist < light.radius * light.radius;
	*/

	vec4 nearest = max(min(light.position, aabb.maxPoint), aabb.minPoint);
	vec3 diff = nearest.xyz - light.position.xyz;
	return dot(diff, diff) < light.radius * light.radius;
}

//------------------------------------------------------------------------------
/**
*/
bool IntersectLineWithPlane(vec3 lineStart, vec3 lineEnd, vec4 plane, out vec3 intersect)
{
	float3 ab = lineEnd - lineStart;
	float t = (plane.w - dot(plane.xyz, lineStart)) / dot(plane.xyz, ab);
	bool ret = (t >= 0.0f && t <= 1.0f);
	intersect = float3(0, 0, 0);
	if (ret)
	{
		intersect = lineStart + t * ab;
	}

	return ret;
}

write rgba16f image2D DebugOutput;

//------------------------------------------------------------------------------
/**
*/
[localsizex] = 1024
shader 
void csClusterAABB()
{
	uint index1D = gl_GlobalInvocationID.x;
	uint3 index3D = Unpack1DTo3D(index1D, NumCells.x, NumCells.y);

	if (index1D > NumCells.x * NumCells.y * NumCells.z)
		return;

	// Calculate near and far plane in the XY plane, offset at our Z offset
	vec4 nearPlane	= vec4(0, 0, 1.0f, -FocalLengthNearFar.z * pow(abs(ZPerspectiveCorrection), index3D.z));
	vec4 farPlane	= vec4(0, 0, 1.0f, -FocalLengthNearFar.z * pow(abs(ZPerspectiveCorrection), index3D.z + 1));

	// Transform the corners to view space
	vec4 minCorner = PixelToView(index3D.xy * vec2(BlockSize) * InvFramebufferDimensions, 1);
	vec4 maxCorner = PixelToView((index3D.xy + ivec2(1.0f)) * vec2(BlockSize) * InvFramebufferDimensions, 1);

	// Trace a ray from the eye (origin) towards the four corner points
	vec3 nearMin, nearMax, farMin, farMax;
	vec3 eye = vec3(0.0f, 0.0f, 0.0f);
	IntersectLineWithPlane(eye, minCorner.xyz, nearPlane, nearMin);
	IntersectLineWithPlane(eye, maxCorner.xyz, nearPlane, nearMax);
	IntersectLineWithPlane(eye, minCorner.xyz, farPlane, farMin);
	IntersectLineWithPlane(eye, maxCorner.xyz, farPlane, farMax);

	// Calculate AABB using min and max
	ClusterAABB aabb;
	aabb.minPoint = vec4(min(nearMin, min(nearMax, min(farMin, farMax))), 1);
	aabb.maxPoint = vec4(max(nearMin, max(nearMax, max(farMin, farMax))), 1);
	LightTileList cell;
	cell.numLights = 0;
	
	for (uint i = 0; i < NumInputLights && cell.numLights < MAX_LIGHTS_PER_CLUSTER; i++)
	{
		const Light light = Lights[i];
		if (light.type == LIGHT_TYPE__POINTLIGHT && TestAABBSphere(aabb, light))
		{
			cell.lightIndex[cell.numLights] = i;
			cell.numLights++;
		}
		else if (light.type == LIGHT_TYPE__SPOTLIGHT && TestAABBSphere(aabb, light))
		{
			cell.lightIndex[cell.numLights] = i;
			cell.numLights++;
		}
		else if (light.type == LIGHT_TYPE__AREALIGHT) // implement for area lights
		{

		}
	}
	Lists[index1D] = cell;
	AABBs[index1D] = aabb;
}

//------------------------------------------------------------------------------
/**
*/
[localsizex] = 256
shader
void csLightDebug()
{

	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	float depth = fetch2D(DepthBuffer, PosteffectSampler, coord, 0).r;

	vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth);
	

	uint3 index3D = CalculateClusterIndex(coord, viewPos.z, BlockSize, FocalLengthNearFar.z, Fov);
	uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);
	LightTileList cell = Lists[idx];
	uint heat = 0;
	vec4 color = vec4(0, 0, 0, 0);
	if (cell.numLights > 0)
	{
		heat = cell.numLights;
		vec4 cold = vec4(0, 1, 0, 1);
		vec4 hot = vec4(1, 0, 0, 1);
		vec4 color = mix(cold, hot, saturate(heat / 6.0f));
		/*
		for (uint i = 0; i < cell.numLights; i++)
		{
			Light light = lights[cell.lightIndex[i]];
		}
		*/
	}

	
	imageStore(DebugOutput, int2(coord), color);
}

//------------------------------------------------------------------------------
/**
*/
program AABBGenerate [ string Mask = "AABBAndCull"; ]
{
	ComputeShader = csClusterAABB();
};

//------------------------------------------------------------------------------
/**
*/
program ClusterDebug[string Mask = "ClusterDebug";]
{
	ComputeShader = csLightDebug();
};
