//------------------------------------------------------------------------------
//  tessellationparams.fxh
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef TESSELLATIONPARAMS
#define TESSELLATIONPARAMS
#include "std.fxh"

// material properties
group(BATCH_GROUP) shared constant TessellationParams [ bool DynamicOffset = true; string Visibility = "HS|DS";]
{
	float TessellationFactor = 1.0f;
	float MaxDistance = 250.0f;
	float MinDistance = 20.0f;
	float HeightScale = 0.0f;
	float SceneScale = 1.0f;
};

#endif // TESSELLATIONPARAMS