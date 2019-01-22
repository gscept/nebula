#pragma once
//------------------------------------------------------------------------------
/**
	Contains relevant information about a skeleton joint
		
	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
namespace Characters
{

struct SkeletonJoint
{
	// NOTE: DO NOT MODIFY THE MEMBERS OF THIS STRUCTURE!
	SkeletonJoint() :
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
