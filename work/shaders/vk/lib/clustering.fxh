//------------------------------------------------------------------------------
//  clustering.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef CLUSTERING_FXH
#define CLUSTERING_FXH

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