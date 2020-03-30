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

group(BATCH_GROUP) varbuffer ClusterAABBs  [ string Visibility = "CS|PS"; ]
{
	ClusterAABB AABBs[];
};

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) varblock ClusterUniforms [ string Visibility = "CS|PS"; ]
{
	vec2 FramebufferDimensions;
	vec2 InvFramebufferDimensions;
	uvec2 BlockSize;
	float InvZScale;
	float InvZBias;

	uvec3 NumCells;
	float ZDistribution;
};

#define CLUSTER_POINTLIGHT_BIT 0x1u
#define CLUSTER_SPOTLIGHT_BIT 0x2u
#define CLUSTER_AREALIGHT_BIT 0x4u
#define CLUSTER_LIGHTPROBE_BIT 0x8u
#define CLUSTER_PBR_DECAL_BIT 0x10u
#define CLUSTER_EMISSIVE_DECAL_BIT 0x20u
#define CLUSTER_FOG_BIT 0x40u

#define CHECK_FLAG(bits, bit) ((bits & bit) == bit)


//------------------------------------------------------------------------------
/**
	Unpack a 1D index into a 3D index
*/
uint3
Unpack1DTo3D(uint index1D, uint width, uint height)
{
	uint i = index1D % width;
	uint j = index1D % (width * height) / width;
	uint k = index1D / (width * height);

	return uint3(i, j, k);
}

//------------------------------------------------------------------------------
/**
	Pack a 3D index into a 1D array index
*/
uint
Pack3DTo1D(uint3 index3D, uint width, uint height)
{
	return index3D.x + (width * (index3D.y + height * index3D.z));
}

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