//------------------------------------------------------------------------------
//  fbxskeletonparser.cc
//  (C) 2011 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbxskeletonparser.h"
#include "math/vector.h"
#include "math/quaternion.h"
#include "fbxparserbase.h"
#include "base/exporterbase.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::FBXSkeletonParser, 'FBXS', ToolkitUtil::FBXParserBase);

using namespace ToolkitUtil;
using namespace Math;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
FBXSkeletonParser::FBXSkeletonParser()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FBXSkeletonParser::~FBXSkeletonParser()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXSkeletonParser::Cleanup()
{
	for (int skeletonIndex = 0; skeletonIndex < this->skeletons.Size(); skeletonIndex++)
	{
		Skeleton* skeleton = this->skeletons[skeletonIndex];
		for (int jointIndex = 0; jointIndex < skeleton->joints.Size(); jointIndex++)
		{
			delete skeleton->joints[jointIndex];
		}
		skeleton->joints.Clear();
		delete skeleton;
	}
	this->skeletons.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXSkeletonParser::Parse( KFbxScene* scene, AnimBuilder* animBuilder /* = 0 */ )
{
	// make sure there are no dormant skeletons
	this->Cleanup();

	int skeletonCount = scene->GetSrcObjectCount(FBX_TYPE(KFbxSkeleton));
	SkeletonJoint* root = 0;
	for (int skeletonIndex = 0; skeletonIndex < skeletonCount; skeletonIndex++)
	{
		KFbxSkeleton* skeleton = scene->GetSrcObject(FBX_TYPE(KFbxSkeleton), skeletonIndex);
		// we only want to construct node trees if we have a root node
		if (skeleton->IsSkeletonRoot())
		{
			KFbxNode* node = skeleton->GetNode();
			
			n_printf("Exporting skeleton: %s\n", node->GetName());
			this->exporter->Progress(0, "Exporting: " + String(node->GetName()));
			Skeleton* newSkeleton = new Skeleton;
			SkeletonJoint* newJoint = new SkeletonJoint;
			newJoint->name = node->GetName();
			newJoint->index = 0;
			newJoint->parentIndex = SkeletonJoint::InvalidParent;
			newJoint->fbxNode = node;

			newSkeleton->joints.Append(newJoint);
			newSkeleton->root = newJoint;
			root = newJoint;
			skeletons.Append(newSkeleton);

			int rootNodeIndex = 0;
			ConstructJointTree(newSkeleton->joints, skeleton->GetNode(), rootNodeIndex, 0);
			n_printf("Skeleton done!\n");
			if (skeletonIndex > 0)
			{
				n_printf("NOTE: The model for this skeleton will be named: %s \n", node->GetName());
			}
		}
	}


	for (int skeletonIndex = 0; skeletonIndex < skeletons.Size(); skeletonIndex++)
	{
		// get our skeleton
		Skeleton* skeleton = skeletons[skeletonIndex];
		for (int jointIndex = 0; jointIndex < skeleton->joints.Size(); jointIndex++)
		{
			SkeletonJoint* joint = skeleton->joints[jointIndex];
			String name = joint->fbxNode->GetName();

			// Nebula wants the local transformation so that's what we are going to give it!
			KFbxXMatrix globalTrans = joint->fbxNode->EvaluateLocalTransform();
			KFbxVector4 position = joint->fbxNode->LclTranslation.Get();
			KFbxVector4 scale = joint->fbxNode->LclScaling.Get();

			KFbxQuaternion fbxQuat = globalTrans.GetQ();
			quaternion rotationQuat = quaternion((float)fbxQuat[0],(float)fbxQuat[1],(float)fbxQuat[2],(float)fbxQuat[3]);

			float4 trans((float)position[0] / this->scaleFactor, (float)position[1] / this->scaleFactor, (float)position[2] / this->scaleFactor, 0);

			// set joint information
			joint->translation	= trans;
			joint->rotation		= rotationQuat;
			joint->scale		= float4((float)scale[0], (float)scale[1], (float)scale[2], 0.0f);
				
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXSkeletonParser::ConstructJointTree( Util::Array<SkeletonJoint*>& joints, KFbxNode* fbxNode, int& currentIndex, int parentIndex )
{
	int childCount = fbxNode->GetChildCount();
	Util::Array<SkeletonJoint*> nodeChildren;
	for (int childIndex = 0; childIndex < childCount; childIndex++)
	{
		KFbxNode* child = fbxNode->GetChild(childIndex);

		if (child->GetSkeleton())
		{
			SkeletonJoint* newJoint = new SkeletonJoint;
			newJoint->name = child->GetName();
			newJoint->index = ++currentIndex;
			newJoint->parentIndex = parentIndex;
			newJoint->fbxNode = child;

			nodeChildren.Append(newJoint);
			joints.Append(newJoint);

			ConstructJointTree(joints, newJoint->fbxNode, currentIndex, newJoint->index);
		}
	}
}

} // namespace ToolkitUtil