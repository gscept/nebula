//------------------------------------------------------------------------------
//  skinning.fxh
//  (C) 2015 Invidiual contributors
//------------------------------------------------------------------------------

#ifndef SKINNING_FXH
#define SKINNING_FXH
#include "lib/shared.fxh"

/*
shared buffers=1024 varblock Joints
{
	mat4 JointPalette[96];
};
*/
group(INSTANCE_GROUP) shared varblock JointBlock [ bool DynamicOffset = true; string Visibility = "VS"; ]
{
	mat4 JointPalette[256];
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
	vec4 normWeights = weights / dot(weights, vec4(1.0));
	
	// the fact that this works blows my mind, but it must be faster...
	mat4x4 joint = JointPalette[indices[0]] * normWeights[0] + 
				   JointPalette[indices[1]] * normWeights[1] + 
				   JointPalette[indices[2]] * normWeights[2] + 
				   JointPalette[indices[3]] * normWeights[3];
	return joint * vec4(inPos, 1);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex position using .
*/
vec4
SkinnedPositionInstanced(const vec3 inPos, const vec4 weights, const uvec4 indices, const uint ID)
{	
    // need to re-normalize weights because of compression
    vec4 normWeights = weights / dot(weights, vec4(1.0));
	mat4x4 joint = JointPalette[indices[0]] * normWeights[0] + 
				   JointPalette[indices[1]] * normWeights[1] + 
				   JointPalette[indices[2]] * normWeights[2] + 
				   JointPalette[indices[3]] * normWeights[3];
	return joint * vec4(inPos, 1);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex normal.
*/
vec4
SkinnedNormal(const vec3 inNormal, const vec4 weights, const uvec4 indices)
{
	// the fact that this works blows my mind, but it must be faster...
	mat4x4 joint = JointPalette[indices[0]] * weights[0] + 
				   JointPalette[indices[1]] * weights[1] + 
				   JointPalette[indices[2]] * weights[2] + 
				   JointPalette[indices[3]] * weights[3];
	return joint * vec4(inNormal, 0);
}

//------------------------------------------------------------------------------
/**
    Compute a skinned vertex normal.
*/
vec4
SkinnedNormalInstanced(const vec3 inNormal, const vec4 weights, const uvec4 indices, const uint ID)
{
	mat4x4 joint = JointPalette[indices[0]] * weights[0] + 
				   JointPalette[indices[1]] * weights[1] + 
				   JointPalette[indices[2]] * weights[2] + 
				   JointPalette[indices[3]] * weights[3];
	return joint * vec4(inNormal, 0);
}

#endif
