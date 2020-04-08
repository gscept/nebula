//------------------------------------------------------------------------------
//  clustering.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------

struct ClusterAABB
{
	vec4 maxPoint;
	vec4 minPoint;
	uint featureFlags;
};

group(BATCH_GROUP) rw_buffer ClusterAABBs  [ string Visibility = "CS|PS"; ]
{
	ClusterAABB AABBs[];
};

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) constant ClusterUniforms [ string Visibility = "CS|PS"; ]
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
const uint CLUSTER_FOG_BIT = 0x40u;

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