//------------------------------------------------------------------------------
//  animationparams.fxh
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef ANIMATIONPARAMS
#define ANIMATIONPARAMS
#include "std.fxh"

// material properties
group(BATCH_GROUP) shared varblock AnimationParams [ string Visibility = "VS"; ]
{
	vec2 AnimationDirection;
	float AnimationAngle;
	float AnimationLinearSpeed;
	float AnimationAngularSpeed;
	int NumXTiles = 1;
	int NumYTiles = 1;
};

#endif // ANIMATIONPARAMS