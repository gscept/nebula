#ifndef SKINNING_H
#define SKINNING_H


tbuffer PerCharacter
{
	matrix JointPalette[128] : JointPalette;
}

Texture2D JointInstanceTexture : JointInstanceTexture;

static const uint JointsPerRow = 1024 / 4;

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex position.
*/
float4
SkinnedPosition(const float4 inPos, const float4 weights, const uint4 indices)
{
	// need to re-normalize weights because of compression
	float3 pos[4];
	float4 normWeights = weights / dot(weights, float4(1.0, 1.0, 1.0, 1.0));

	// we always have one joint, even if we have a rigid bind
	pos[0] = (mul(inPos, JointPalette[indices[0]])) * normWeights[0];
	
	if (normWeights[0] == 1)
	{
		pos[1] = pos[2] = pos[3] = 0;
	}
	else
	{
		pos[1] = (mul(inPos, JointPalette[indices[1]])) * normWeights[1];	
		pos[2] = (mul(inPos, JointPalette[indices[2]])) * normWeights[2];	
		pos[3] = (mul(inPos, JointPalette[indices[3]])) * normWeights[3];	
	}    

    return float4(pos[0] + pos[1] + pos[2] + pos[3], 1.0f);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex position using .
*/
float4
SkinnedPositionInstanced(const float4 inPos, const float4 weights, const uint4 indices, const uint ID)
{
    // need to re-normalize weights because of compression
    float3 pos[4];
    float4 normWeights = weights / dot(weights, float4(1.0, 1.0, 1.0, 1.0));

	// we always have one joint, even if we have a rigid bind
    pos[0] = (mul(inPos, JointPalette[indices[0]])) * normWeights[0];
	
	if (normWeights[0] == 1)
	{
		pos[1] = pos[2] = pos[3] = 0;
	}
	else
	{
		pos[1] = (mul(inPos, JointPalette[indices[1]])) * normWeights[1];	
		pos[2] = (mul(inPos, JointPalette[indices[2]])) * normWeights[2];	
		pos[3] = (mul(inPos, JointPalette[indices[3]])) * normWeights[3];	
	}    

    return float4(pos[0] + pos[1] + pos[2] + pos[3], 1.0f);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex normal.
*/
float3
SkinnedNormal(const float3 inNormal, const float4 weights, const uint4 indices)
{
    // normals don't need to be 100% perfect, so don't normalize weights
    float3 normal[4];

	// we also want to convert the joint palette to a rotation matrix to avoid translating the normal
    normal[0] = mul(inNormal, (float3x3)JointPalette[indices[0]]) * weights[0];
	
	if (weights[0] == 1)
	{
		normal[1] = normal[2] = normal[3] = 0;
	}
	else
	{
		normal[1] = mul(inNormal, (float3x3)JointPalette[indices[1]]) * weights[1];
		normal[2] = mul(inNormal, (float3x3)JointPalette[indices[2]]) * weights[2];
		normal[3] = mul(inNormal, (float3x3)JointPalette[indices[3]]) * weights[3];
	}

    
    return float3(normal[0] + normal[1] + normal[2] + normal[3]);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex normal.
*/
float3
SkinnedNormalInstanced(const float3 inNormal, const float4 weights, const uint4 indices, const uint ID)
{
    // normals don't need to be 100% perfect, so don't normalize weights
    float3 normal[4];

	// we also want to convert the joint palette to a rotation matrix to avoid translating the normal
    normal[0] = mul(inNormal, (float3x3)JointPalette[indices[0]]) * weights[0];
	
	if (weights[0] == 1)
	{
		normal[1] = normal[2] = normal[3] = 0;
	}
	else
	{
		normal[1] = mul(inNormal, (float3x3)JointPalette[indices[1]]) * weights[1];
		normal[2] = mul(inNormal, (float3x3)JointPalette[indices[2]]) * weights[2];
		normal[3] = mul(inNormal, (float3x3)JointPalette[indices[3]]) * weights[3];
	}

    
    return float3(normal[0] + normal[1] + normal[2] + normal[3]);
}



#endif
