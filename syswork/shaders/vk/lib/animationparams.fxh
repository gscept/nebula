//------------------------------------------------------------------------------
//  animationparams.fxh
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef ANIMATIONPARAMS
#define ANIMATIONPARAMS
#include "std.fxh"

// material properties
group(BATCH_GROUP) shared constant AnimationParams [ string Visibility = "VS|PS"; ]
{
	vec2 AnimationDirection;
	float AnimationAngle;
	float AnimationLinearSpeed;
	float AnimationAngularSpeed;
	int NumXTiles = 1;
	int NumYTiles = 1;
	uint _padanim0;
};

#endif // ANIMATIONPARAMS
