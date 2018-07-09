//------------------------------------------------------------------------------
//  animationparams.fxh
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef ANIMATIONPARAMS
#define ANIMATIONPARAMS
#include "std.fxh"

// material properties
group(INSTANCE_GROUP) shared varblock AnimationParams [ bool DynamicOffset = true; string Visibility = "VS"; ]
{
	vec2 AnimationDirection;
	float AnimationAngle;
	float AnimationLinearSpeed;
	float AnimationAngularSpeed;
	int NumXTiles = 1;
	int NumYTiles = 1;
};

#endif // ANIMATIONPARAMS