//------------------------------------------------------------------------------
//  objects_shared.fxh
//  (C) 2020 gscept
//------------------------------------------------------------------------------
#ifndef OBJECTS_SHARED_FXH
#define OBJECTS_SHARED_FXH

#include "std.fxh"

// contains variables which are guaranteed to be unique per object.
group(DYNAMIC_OFFSET_GROUP) constant ObjectBlock [ string Visibility = "VS|PS"; ]
{
	mat4 Model;
	mat4 InvModel;
	int ObjectId;
	float DitherFactor;
};

// define how many objects we can render with instancing
const int MAX_INSTANCING_BATCH_SIZE = 256;
group(DYNAMIC_OFFSET_GROUP) constant InstancingBlock [ string Visibility = "VS"; ]
{
	mat4 ModelArray[MAX_INSTANCING_BATCH_SIZE];
	mat4 ModelViewArray[MAX_INSTANCING_BATCH_SIZE];
	mat4 ModelViewProjectionArray[MAX_INSTANCING_BATCH_SIZE];
	int IdArray[MAX_INSTANCING_BATCH_SIZE];
};

group(DYNAMIC_OFFSET_GROUP) constant JointBlock [ string Visibility = "VS"; ]
{
	mat4 JointPalette[256];
};

#endif // OBJECTS_SHARED_FXH