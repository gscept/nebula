//------------------------------------------------------------------------------
//  clustering.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef CLUSTERING_FXH
#define CLUSTERING_FXH

#ifndef CLUSTERING_GROUP
#define CLUSTERING_GROUP BATCH_GROUP
#endif

#ifndef CLUSTERING_VISIBILITY
#define CLUSTERING_VISIBILITY "CS|PS"
#endif

struct ClusterAABB
{
	vec4 maxPoint;
	vec4 minPoint;
	uint featureFlags;
};

group(CLUSTERING_GROUP) rw_buffer ClusterAABBs [ string Visibility = CLUSTERING_VISIBILITY; ]
{
	ClusterAABB AABBs[];
};

// this is used to keep track of how many lights we have active
group(CLUSTERING_GROUP) constant ClusterUniforms [ string Visibility = CLUSTERING_VISIBILITY; ]
{
	vec2 FramebufferDimensions;
	vec2 InvFramebufferDimensions;
	uvec2 BlockSize;
	float InvZScale;
	float InvZBias;

	uvec3 NumCells;
	float ZDistribution;
};

const uint CLUSTER_POINTLIGHT_BIT = 0x1u;
const uint CLUSTER_SPOTLIGHT_BIT = 0x2u;
const uint CLUSTER_AREALIGHT_BIT = 0x4u;
const uint CLUSTER_LIGHTPROBE_BIT = 0x8u;
const uint CLUSTER_PBR_DECAL_BIT = 0x10u;
const uint CLUSTER_EMISSIVE_DECAL_BIT = 0x20u;
const uint CLUSTER_FOG_SPHERE_BIT = 0x40u;
const uint CLUSTER_FOG_BOX_BIT = 0x80u;

// set a fixed number of cluster entries
const uint NUM_CLUSTER_ENTRIES = 16384;

#define CHECK_FLAG(bits, bit) ((bits & bit) == bit)

//------------------------------------------------------------------------------
/**
	Calculate 3D index from screen position and depth
*/
uint3 CalculateClusterIndex(vec2 screenPos, float depth, float scale, float bias)
{
	uint i = uint(screenPos.x);
	uint j = uint(screenPos.y);
	uint k = uint(log2(-depth) * scale + bias);

	return uint3(i, j, k);
}

//------------------------------------------------------------------------------
/**
*/
bool
TestAABBAABB(ClusterAABB aabb, vec3 bboxMin, vec3 bboxMax)
{
	// this expression can be unfolded like this:
	//	C = min x, y, z in A must be smaller than the max x, y, z in B
	//	D = max x, y, z in A must be bigger than the min x, y, z in B
	//	E = C equals D
	//	return if all members of E are true
	return all(equal(lessThan(aabb.minPoint.xyz, bboxMax), greaterThan(aabb.maxPoint.xyz, bboxMin)));
}


//------------------------------------------------------------------------------
/**
*/
bool
TestAABBSphere(ClusterAABB aabb, vec3 pos, float radius)
{
	float sqDist = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		float v = (pos)[i];

		if (v < aabb.minPoint[i]) sqDist += pow(aabb.minPoint[i] - v, 2);
		if (v > aabb.maxPoint[i]) sqDist += pow(v - aabb.maxPoint[i], 2);
	}
	return sqDist <= radius * radius;
}

//------------------------------------------------------------------------------
/**
	Treat AABB as a sphere for simplicity of intersection detection.

	https://bartwronski.com/2017/04/13/cull-that-cone/
*/
bool
TestAABBCone(ClusterAABB aabb, vec3 pos, vec3 forward, float radius, vec2 sinCosAngles)
{
	float3 aabbExtents = (aabb.maxPoint.xyz - aabb.minPoint.xyz) * 0.5f;
	float3 aabbCenter = aabb.minPoint.xyz + aabbExtents;
	float aabbRadius = aabb.maxPoint.w;

	float3 v = aabbCenter - pos;
	const float vlensq = dot(v, v);
	const float v1len = dot(v, -forward);
	const float distanceClosestPoint = sinCosAngles.y * sqrt(vlensq - v1len * v1len) - v1len * sinCosAngles.x;

	const bool angleCull = distanceClosestPoint > aabbRadius;
	const bool frontCull = v1len > aabbRadius + radius;
	const bool backCull = v1len < -aabbRadius;
	return !(angleCull || backCull || frontCull);
}

#endif