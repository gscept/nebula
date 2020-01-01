#pragma once
//------------------------------------------------------------------------------
/**
	Contains relevant information about a skeleton joint
		
	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
namespace Characters
{

// this structure is used update the skeleton in the eval job
struct SkeletonJobJoint
{
	// NOTE: DO NOT MODIFY THE MEMBERS OF THIS STRUCTURE!
	SkeletonJobJoint() :
		varTranslationX(0.0f),
		varTranslationY(0.0f),
		varTranslationZ(0.0f),
		varTranslationW(0.0f), // for correct alignment, not used
		varScaleX(1.0f),
		varScaleY(1.0f),
		varScaleZ(1.0f),
		parentJointIndex(InvalidIndex)
	{ };

	float varTranslationX, varTranslationY, varTranslationZ, varTranslationW;
	float varScaleX, varScaleY, varScaleZ;
	int parentJointIndex;
};

} // namespace Characters
