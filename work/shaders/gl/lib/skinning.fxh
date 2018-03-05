//------------------------------------------------------------------------------
//  skinning.fxh
//  (C) 2015 Invidiual contributors
//------------------------------------------------------------------------------

#ifndef SKINNING_FXH
#define SKINNING_FXH

/*
shared buffers=1024 varblock Joints
{
	mat4 JointPalette[96];
};
*/
shared varbuffer JointBlock
{
	mat4 JointPalette[];
};

sampler2D JointInstanceTexture;

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex position.
*/
vec4
SkinnedPosition(const vec3 inPos, const vec4 weights, const uvec4 indices)
{		
	// need to re-normalize weights because of compression
	vec4 pos[4];
	vec4 normWeights = weights / dot(weights, vec4(1.0));

	// we always have one joint, even if we have a rigid bind
	pos[0] = JointPalette[indices[0]] * vec4(inPos, 1) * normWeights[0];
	pos[1] = JointPalette[indices[1]] * vec4(inPos, 1) * normWeights[1];
	pos[2] = JointPalette[indices[2]] * vec4(inPos, 1) * normWeights[2];
	pos[3] = JointPalette[indices[3]] * vec4(inPos, 1) * normWeights[3];

    return vec4((pos[0] + pos[1] + pos[2] + pos[3]).xyz, 1);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex position using .
*/
vec4
SkinnedPositionInstanced(const vec3 inPos, const vec4 weights, const uvec4 indices, const uint ID)
{	
    // need to re-normalize weights because of compression
    vec4 pos[4];
    vec4 normWeights = weights / dot(weights, vec4(1.0));

	// we always have one joint, even if we have a rigid bind
    pos[0] = JointPalette[indices[0]] * vec4(inPos, 1) * normWeights[0];
	pos[1] = JointPalette[indices[1]] * vec4(inPos, 1) * normWeights[1];
	pos[2] = JointPalette[indices[2]] * vec4(inPos, 1) * normWeights[2];
	pos[3] = JointPalette[indices[3]] * vec4(inPos, 1) * normWeights[3];

    return vec4((pos[0] + pos[1] + pos[2] + pos[3]).xyz, 1);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex normal.
*/
vec4
SkinnedNormal(const vec3 inNormal, const vec4 weights, const uvec4 indices)
{
    // normals don't need to be 100% perfect, so don't normalize weights
    vec4 normal[4];

	// we also want to convert the joint palette to a rotation matrix to avoid translating the normal
    normal[0] = JointPalette[indices[0]] * vec4(inNormal, 0) * weights[0];
	normal[1] = JointPalette[indices[1]] * vec4(inNormal, 0) * weights[1];
	normal[2] = JointPalette[indices[2]] * vec4(inNormal, 0) * weights[2];
	normal[3] = JointPalette[indices[3]] * vec4(inNormal, 0) * weights[3];

    return vec4((normal[0] + normal[1] + normal[2] + normal[3]).xyz, 0);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex normal.
*/
vec4
SkinnedNormalInstanced(const vec3 inNormal, const vec4 weights, const uvec4 indices, const uint ID)
{
    // normals don't need to be 100% perfect, so don't normalize weights
    vec4 normal[4];

	// we also want to convert the joint palette to a rotation matrix to avoid translating the normal
    normal[0] = JointPalette[indices[0]] * vec4(inNormal, 0) * weights[0];
	normal[1] = JointPalette[indices[1]] * vec4(inNormal, 0) * weights[1];
	normal[2] = JointPalette[indices[2]] * vec4(inNormal, 0) * weights[2];
	normal[3] = JointPalette[indices[3]] * vec4(inNormal, 0) * weights[3];

    return vec4((normal[0] + normal[1] + normal[2] + normal[3]).xyz, 0);
}

#endif
