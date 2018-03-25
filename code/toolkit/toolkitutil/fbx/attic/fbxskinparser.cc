//------------------------------------------------------------------------------
//  fbxskinparser.cc
//  (C) 2011 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "math/float4.h"
#include "fbxskinparser.h"
#include "fbxparserbase.h"
#include "base/exporterbase.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::FBXSkinParser, 'FBXK', ToolkitUtil::FBXParserBase);

using namespace ToolkitUtil;
using namespace Math;
using namespace Util;
//------------------------------------------------------------------------------
/**
*/
FBXSkinParser::FBXSkinParser()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FBXSkinParser::~FBXSkinParser()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXSkinParser::Parse( KFbxScene* scene, AnimBuilder* animBuilder /* = 0 */ )
{
	KFbxMesh* fbxMesh = this->mesh->fbxNode->GetMesh();
	int vertexCount = fbxMesh->GetControlPointsCount();
	int skinCount = fbxMesh->GetDeformerCount(KFbxDeformer::eSKIN);
	int* jointArray = new int[vertexCount*4];
	double* weightArray = new double[vertexCount*4];
	int* slotArray = new int[vertexCount];
	memset(jointArray, 0, sizeof(int)*vertexCount*4);
	memset(weightArray, 0, sizeof(double)*vertexCount*4);
	memset(slotArray, 0, sizeof(int)*vertexCount);

	for (int skinIndex = 0; skinIndex < skinCount; skinIndex++)
	{
		KFbxSkin* skin = static_cast<KFbxSkin*>(fbxMesh->GetDeformer(skinIndex, KFbxDeformer::eSKIN));
		double* blendWeights = skin->GetControlPointBlendWeights();
		int* indices = skin->GetControlPointIndices();
		int controlPointCount = skin->GetControlPointIndicesCount();
		mesh->isSkinned = true;
		int clusterCount = skin->GetClusterCount();
		Array<int> usedJoints;

		n_printf("Applying skin to model: %s \n", fbxMesh->GetNode()->GetName());

		for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
		{
			KFbxCluster* cluster = skin->GetCluster(clusterIndex);
			KFbxNode* joint = cluster->GetLink();
			KFbxSkeleton* skeletonLink = joint->GetSkeleton();
			// assign skeleton to skin
			if (skeletonLink && skeletonLink->IsSkeletonRoot())
			{
				// find correct skeleton
				for (int skeletonIndex = 0; skeletonIndex < this->skeletons.Size(); skeletonIndex++)
				{
					if (this->skeletons[skeletonIndex]->root->fbxNode == joint)
					{
						mesh->skeleton = this->skeletons[skeletonIndex];
						mesh->skeleton->skinnedMeshes.Append(mesh);
						break;
					}
				}
			}

			int clusterVertexIndexCount = cluster->GetControlPointIndicesCount();
			for (int vertexIndex = 0; vertexIndex < clusterVertexIndexCount; vertexIndex++)
			{
				int vertex = cluster->GetControlPointIndices()[vertexIndex];
				double weight = cluster->GetControlPointWeights()[vertexIndex];
				int stride = slotArray[vertex];

				// this is just a fail safe for smooth operators
				if (vertex >= vertexCount)
					continue;
				
				int* jointData = jointArray + (vertex*4);
				double* weightData = weightArray + (vertex*4);

				// ignore weights and indices over 4 (optimal vertex-to-joint ratio for games is 4)
				if (stride > 3) continue;

				jointData[stride] = clusterIndex;
				weightData[stride] = weight;
				slotArray[vertex]++;
			}
		}

		n_printf("Skin applied!\n");
	}
	if (mesh->isSkinned)
	{
		// finally, traverse through every vertex and set their joint indices and weighs
		for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
		{
			MeshBuilderVertex& vertex = mesh->meshSource->VertexAt(vertexIndex);
			int* indices = jointArray + (vertexIndex*4);
			double* weights = weightArray + (vertexIndex*4);
			vertex.SetComponent(MeshBuilderVertex::WeightsUB4NIndex, float4((float)weights[0], (float)weights[1], (float)weights[2], (float)weights[3]));
			vertex.SetComponent(MeshBuilderVertex::JIndicesUB4Index, float4((float)indices[0], (float)indices[1], (float)indices[2], (float)indices[3]));
		}
	}

	delete [] slotArray;
	delete [] jointArray;
	delete [] weightArray;
}

} // namespace ToolkitUtil