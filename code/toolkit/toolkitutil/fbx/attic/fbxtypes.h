#pragma once
//------------------------------------------------------------------------------
/**    
	This header contains Nebula objects which also have an FBX connection
    
    (C) 2012 gscept
*/
#include <fbxsdk.h>
#include "stdneb.h"
#include "toolkit/toolkitutil/meshutil/meshbuilder.h"
#include "toolkit/toolkitutil/n3util/n3modeldata.h"

namespace ToolkitUtil
{
	struct SkeletonJoint
	{
		static const short InvalidParent = -1;

		Util::String name;
		Math::float4 translation;
		Math::quaternion rotation;
		Math::float4 scale;
		int parentIndex;
		int index;

		const Joint ConvertToModelJoint()
		{
			Joint j;
			j.name = this->name;
			j.parent = this->parentIndex;
			j.translation = this->translation;
			j.rotation = this->rotation;
			j.scale = this->scale;
			j.index = this->index;

			return j;
		}

		// used to find bind pose
		KFbxNode* fbxNode;
	};

	struct ShapeNode;
	struct Skeleton
	{
		SkeletonJoint* root;
		Util::Array<SkeletonJoint*> joints;
		
		// list of all skinned meshes
		Util::Array<ShapeNode*> skinnedMeshes;
	};

	struct ShapeNode
	{
		Util::String name;
		Util::String resource;
		ToolkitUtil::MeshBuilder* meshSource;
		Math::vector translation;
		Math::quaternion rotation;
		Math::bbox boundingBox;
		Math::vector scale;
		bool isSkinned;
		IndexT primGroup;

		KFbxNode* fbxNode;
		// skeleton identifier
		Skeleton* skeleton;
	};


	typedef Util::Array<Skeleton*> SkeletonList;
	typedef Util::Array<ShapeNode*> MeshList;
}
