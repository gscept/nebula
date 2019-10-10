//------------------------------------------------------------------------------
//  lights_cluster_classification.fxh
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

#define LIGHT_TYPE__SPOTLIGHT 0
#define LIGHT_TYPE__POINTLIGHT 1
#define LIGHT_TYPE__AREALIGHT 2

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
	int type;		// type of light, look at above definitions for a mapping
    float radius;	// radius of sphere (pointlight) or cone angle (spotlight)
	vec4 position;	// world space position of light
	vec4 forward;	// forward vector of light (spotlight and arealights)
};

// do not modify this one, keep it the same, its being fed through the lightserver
group(BATCH_GROUP) varbuffer LightList
{
	Light lights[];
};

// this is the output list, pointing to an index i in the Input.lights buffer and next to the next element in Output.list. 
struct LightTileList
{
	uint lightIndex[MAX_LIGHTS_PER_CLUSTER];
};

// this is the buffer we want to modify!
group(BATCH_GROUP) varbuffer LightIndexList
{
	LightTileList list[];
};

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) varblock Uniforms
{
    int NumInputLights;
    uvec2 FramebufferDimensions;
};


//------------------------------------------------------------------------------
/**
*/
[localsizex] = CLUSTER_SUBDIVS_X
shader 
void csClusterAABB()
{
    // calculate pixel coordinate and tile
	ivec2 tile = ivec2(gl_WorkGroupID.xy);

    // the depth is calculated as the exponent (normalized to 0-1) multiplied by the far plane
    float depth_near = exp(gl_GlobalInvocationID.z / 8.0f) / 2.72f * FocalLengthNearFar.z;
    float depth_far = exp((gl_GlobalInvocationID.z + 1) / 8.0f) / 2.72f * FocalLengthNearFar.w;

	// calculate rays on each edge of the frustum
	vec4 cluster_end[4];
    vec4 cluster_begin[4];
	cluster_end[0] = (vec4(tile.x, 	    tile.y,		depth_far, 0));	// top-left
	cluster_end[1] = (vec4(tile.x + 1, 	tile.y, 	depth_far, 0));	// top-right
	cluster_end[2] = (vec4(tile.x, 	    tile.y + 1,	depth_far, 0));	// bottom-left
	cluster_end[3] = (vec4(tile.x + 1, 	tile.y + 1,	depth_far, 0));	// bottom-right
    cluster_begin[0] = vec4(cluster_end[0].xy, depth_near, 0);      // top-left
	cluster_begin[1] = vec4(cluster_end[1].xy, depth_near, 0);      // top-right
	cluster_begin[2] = vec4(cluster_end[2].xy, depth_near, 0);      // bottom-left
	cluster_begin[3] = vec4(cluster_end[3].xy, depth_near, 0);      // bottom-right

    // convert far plane points to world space
	cluster_end[0] = (InvViewProjection * cluster_end[0]);
	cluster_end[1] = (InvViewProjection * cluster_end[1]);
	cluster_end[2] = (InvViewProjection * cluster_end[2]);
	cluster_end[3] = (InvViewProjection * cluster_end[3]);

    // convert near plane points to world space
    cluster_begin[0] = (InvViewProjection * cluster_begin[0]);
	cluster_begin[1] = (InvViewProjection * cluster_begin[1]);
	cluster_begin[2] = (InvViewProjection * cluster_begin[2]);
	cluster_begin[3] = (InvViewProjection * cluster_begin[3]);

	vec4 nearMin = min(min(cluster_begin[0], cluster_begin[1]), min(cluster_begin[2], cluster_begin[3]));
	vec4 nearMax = max(max(cluster_begin[0], cluster_begin[1]), max(cluster_begin[2], cluster_begin[3]));

	vec4 farMin = min(min(cluster_end[0], cluster_end[1]), min(cluster_end[2], cluster_end[3]));
	vec4 farMax = max(max(cluster_end[0], cluster_end[1]), max(cluster_end[2], cluster_end[3]));

	ClusterAABB aabb;
	aabb.minPoint = min(nearMin, farMin);
	aabb.maxPoint = max(nearMax, farMax);
	AABBs[gl_LocalInvocationIndex + gl_WorkGroupID.y * CLUSTER_SUBDIVS_X + gl_WorkGroupID.z * CLUSTER_SUBDIVS_X * gl_NumWorkGroups.y] = aabb;
}

//------------------------------------------------------------------------------
/**
*/
[localsizex] = CLUSTER_SUBDIVS_X
shader
void csLightCull()
{
	ClusterAABB aabb = AABBs[gl_LocalInvocationIndex + gl_WorkGroupID.y * CLUSTER_SUBDIVS_X + gl_WorkGroupID.z * CLUSTER_SUBDIVS_X * gl_NumWorkGroups.y];
	const int NumThreadsPerCluster = MAX_LIGHTS_PER_CLUSTER;
	for (uint i = 0; i < NumInputLights && i < MAX_LIGHTS_PER_CLUSTER; i += NumThreadsPerCluster)
	{
		int il = int(gl_LocalInvocationIndex + i);
		if (il < NumInputLights)
		{
			const Light light = lights[il];
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
program AABBGenerate [ string Mask = "Alt0"; ]
{
	ComputeShader = csClusterAABB();
};

//------------------------------------------------------------------------------
/**
*/
program LightCull[string Mask = "Alt1"; ]
{
	ComputeShader = csLightCull();
};