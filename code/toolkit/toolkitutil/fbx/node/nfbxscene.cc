//------------------------------------------------------------------------------
//  fbxnoderegistry.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "nfbxscene.h"
#include "fbx/character/skinpartitioner.h"
#include "modelutil/modeldatabase.h"

using namespace Util;
using namespace ToolkitUtil;
namespace ToolkitUtil
{
__ImplementSingleton(ToolkitUtil::NFbxScene);
__ImplementClass(ToolkitUtil::NFbxScene, 'FBXS', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
NFbxScene::NFbxScene() :
	isOpen(false)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
NFbxScene::~NFbxScene()
{
	if (this->IsOpen())
	{
		this->Close();
	}
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
NFbxScene::Open()
{
	n_assert(!this->IsOpen());
	this->isOpen = true;
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxScene::Close()
{
	n_assert(this->IsOpen());
	this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxScene::Setup( FbxScene* scene, const ExportFlags& exportFlags, const ExportMode& exportMode, float scale )
{
	n_assert(this->IsOpen());
	n_assert(scene);
	this->scene = scene;

	// create scene mesh
	this->mesh = new MeshBuilder;

	// set export settings
	this->flags = exportFlags;
	this->mode = exportMode;

	// create physics mesh
	this->physicsMesh = new MeshBuilder;

	// set scale
	this->scale = scale;

	float fps = this->TimeModeToFPS(scene->GetGlobalSettings().GetTimeMode());

	// handle special case for custom FPS
	if (fps == -1)
	{
		fps = (float)scene->GetGlobalSettings().GetCustomFrameRate();
	}
	scene->GetRootNode()->ResetPivotSetAndConvertAnimation(fps);

	// split meshes based on material
	FbxGeometryConverter* converter = new FbxGeometryConverter(this->GetScene()->GetFbxManager());
	bool triangulated = converter->Triangulate(scene, true);

	// Okay so we want to do this, we really do, but if we do, 
	// the GetSrcObjectCount will give us an INCORRECT amount of meshes, 
	// and getting the mesh will give us invalid meshes...
	//bool foo = converter->SplitMeshesPerMaterial(scene, true);
	//delete converter;
		
	// split meshes by materials
	int meshCount = scene->GetSrcObjectCount<FbxMesh>();

	// recalculate mesh count
	// meshCount = scene->GetSrcObjectCount(FBX_TYPE(FbxMesh));
	int jointCount = scene->GetSrcObjectCount<FbxSkeleton>();
	int transformCount = scene->GetSrcObjectCount<FbxNull>();
	int lodCount = scene->GetSrcObjectCount<FbxLODGroup>();
	int nodeCount = scene->GetSrcObjectCount<FbxNode>();
	this->nodes.Reserve(nodeCount);


	FbxPose * bindpose = 0;
	int poses = scene->GetPoseCount();
	for (int i = 0; i < poses; i++)
	{
		FbxPose * pose = scene->GetPose(i);
		if (pose->IsBindPose())
		{
			bindpose = pose;
			break;
		}
	}

	// gather all joints
	for (int jointIndex = 0; jointIndex < jointCount; jointIndex++)
	{
		Ptr<NFbxJointNode> jointNode = NFbxJointNode::Create();
		jointNode->fbxScene = this->scene;
		FbxNode* fbxJointNode = scene->GetSrcObject<FbxSkeleton>(jointIndex)->GetNode();
		jointNode->Setup(fbxJointNode, this, jointIndex, bindpose);

		this->jointNodes.Add(fbxJointNode, jointNode);
		this->nodes.Add(fbxJointNode, jointNode.upcast<NFbxNode>());
	}

	// gather all lod nodes
	for (int lodIndex = 0; lodIndex < lodCount; lodIndex++)
	{
		Ptr<NFbxNode> lodNode = NFbxNode::Create();
		lodNode->fbxScene = this->scene;
		FbxNode* fbxLodNode = scene->GetSrcObject<FbxLODGroup>(lodIndex)->GetNode();

		// setup node
		lodNode->Setup(fbxLodNode, this);
		this->lodNodes.Add(fbxLodNode, lodNode);
		this->nodes.Add(fbxLodNode, lodNode);
	}

	// gather all mesh nodes
	for (int meshIndex = 0; meshIndex < meshCount; meshIndex++)
	{
		Ptr<NFbxMeshNode> meshNode = NFbxMeshNode::Create();
		meshNode->fbxScene = this->scene;		
		FbxNode* fbxMeshNode = scene->GetSrcObject<FbxMesh>(meshIndex)->GetNode();
		n_assert_fmt(fbxMeshNode, "Mesh lacks connection to node (possibly corrupt file?)\nfile: %s\n", this->name.AsCharPtr());

		// set export flags before setup, since setup will extract the mesh information
		meshNode->SetExportFlags(exportFlags);
		meshNode->SetExportMode(exportMode);
		meshNode->SetGroupId(meshIndex);
		meshNode->Setup(fbxMeshNode, this);

		this->meshNodes.Add(fbxMeshNode, meshNode);
		this->nodes.Add(fbxMeshNode, meshNode.upcast<NFbxNode>());
	}

	// gather all transforms
	for (int transformIndex = 0; transformIndex < transformCount; transformIndex++)
	{
		Ptr<NFbxTransformNode> transformNode = NFbxTransformNode::Create();
		transformNode->fbxScene = this->scene;
		FbxNode* fbxTransformNode = scene->GetSrcObject<FbxNull>(transformIndex)->GetNode();
		transformNode->Setup(fbxTransformNode, this);

		this->transformNodes.Add(fbxTransformNode, transformNode);
		this->nodes.Add(fbxTransformNode, transformNode.upcast<NFbxNode>());
	}

	// link parents
	for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
	{
		FbxNode* fbxNode = scene->GetSrcObject<FbxNode>(nodeIndex);
		if (this->HasNode(fbxNode))
		{
			Ptr<NFbxNode> node = this->GetNode(fbxNode);
			FbxNode* fbxParentNode = node->GetNode()->GetParent();
			if (this->HasNode(fbxParentNode))
			{
				Ptr<NFbxNode> parentNode = this->GetNode(fbxParentNode);
				parentNode->AddChild(node);
			}
			else
			{
				node->isRoot = true;
				this->rootNodes.Append(node);
			}

			// if node is a skeleton node, and it's the root node, add it to the list of skeleton roots
			if (node->IsA(NFbxJointNode::RTTI))
			{
				Ptr<NFbxJointNode> jointNode = node.downcast<NFbxJointNode>();
				if (jointNode->IsRoot())
				{
					this->skeletonRoots.Append(node.downcast<NFbxJointNode>());
				}				
			}

		}
	}

	// extract meshes
	for (int meshIndex = 0; meshIndex < meshCount; meshIndex++)
	{
		// get mesh
		Ptr<NFbxMeshNode> mesh = this->meshNodes.ValueAtIndex(meshIndex);

		// extract mesh
		mesh->ExtractMesh();
	}

	// get model attributes
	Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(this->category + "/" + this->name);

	// traverse skeleton roots and extract animations
	for (int rootIndex = 0; rootIndex < this->skeletonRoots.Size(); rootIndex++)
	{
		Ptr<NFbxJointNode> skeletonRoot = this->skeletonRoots[rootIndex];

		// convert space to local space
		skeletonRoot->ConvertJointsToLocal();

		// generate clip for joint
		skeletonRoot->GenerateAnimationClips(attributes);
	}

	// link joint-specific index
	for (int jointIndex = 0; jointIndex < jointCount; jointIndex++)
	{
		Ptr<NFbxJointNode> joint = this->jointNodes.ValueAtIndex(jointIndex);
		if (!joint->IsRoot() && joint->GetParent().isvalid())
		{
			Ptr<NFbxJointNode> parent = joint->GetParent().downcast<NFbxJointNode>();
			joint->parentIndex = parent->jointIndex;
		}
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
NFbxScene::Cleanup()
{
	IndexT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		this->nodes.ValueAtIndex(i)->Discard();
	}
	delete this->mesh;
	delete this->physicsMesh;
	this->meshNodes.Clear();
	this->jointNodes.Clear();
	this->transformNodes.Clear();
	this->nodes.Clear();
	this->rootNodes.Clear();
	this->skeletonRoots.Clear();
	this->physicsNodes.Clear();
}

//------------------------------------------------------------------------------
/**
	Searches all lists to find key, emits en error if the node wasn't found (use HasNode to ensure existence)
*/
NFbxNode* 
NFbxScene::GetNode( FbxNode* key )
{
	IndexT index = this->nodes.FindIndex(key);
	if (index != InvalidIndex)
	{
		return this->nodes.ValueAtIndex(index);
	}
	else
	{
		n_error("Node with name: '%s' is not registered!", key->GetName());
		return 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
bool 
NFbxScene::HasNode( FbxNode* key )
{
	return this->nodes.FindIndex(key) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
NFbxMeshNode* 
NFbxScene::GetMeshNode( FbxNode* key )
{
	n_assert(this->meshNodes.Contains(key));
	return this->meshNodes[key];
}

//------------------------------------------------------------------------------
/**
*/
bool 
NFbxScene::HasMeshNode( FbxNode* key )
{
	return this->meshNodes.Contains(key);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<NFbxMeshNode> >
NFbxScene::GetMeshNodes() const
{
	return this->meshNodes.ValuesAsArray();
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxScene::RemoveMeshNode( const Ptr<NFbxMeshNode>& node )
{
	this->nodes.Erase(node->GetNode());
	this->meshNodes.Erase(node->GetNode());
}

//------------------------------------------------------------------------------
/**
*/
NFbxJointNode* 
NFbxScene::GetJointNode( FbxNode* key )
{
	n_assert(this->jointNodes.Contains(key));
	return this->jointNodes[key];
}


//------------------------------------------------------------------------------
/**
*/
bool 
NFbxScene::HasJointNode( FbxNode* key )
{
	return this->jointNodes.Contains(key);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<NFbxJointNode> >
NFbxScene::GetJointNodes() const
{
	return this->jointNodes.ValuesAsArray();
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxScene::RemoveJointNode( const Ptr<NFbxJointNode>& node )
{
	this->nodes.Erase(node->GetNode());
	this->jointNodes.Erase(node->GetNode());
}

//------------------------------------------------------------------------------
/**
*/
NFbxTransformNode* 
NFbxScene::GetTransformNode( FbxNode* key )
{
	n_assert(this->transformNodes.Contains(key));
	return this->transformNodes[key];
}

//------------------------------------------------------------------------------
/**
*/
bool 
NFbxScene::HasTransformNode( FbxNode* key )
{
	return this->transformNodes.Contains(key);
}

//------------------------------------------------------------------------------
/**
*/
const  Util::Array<Ptr<NFbxTransformNode> > 
NFbxScene::GetTransformNodes() const
{
	return this->transformNodes.ValuesAsArray();
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxScene::RemoveTransformNode( const Ptr<NFbxTransformNode>& node )
{
	this->nodes.Erase(node->GetNode());
	this->transformNodes.Erase(node->GetNode());
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<FbxNode*, Ptr<NFbxNode> >& 
NFbxScene::GetNodes()
{
	return this->nodes;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<NFbxNode> >& 
NFbxScene::GetRootNodes()
{
	return this->rootNodes;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<NFbxJointNode> > 
NFbxScene::GetSkeletonRoots() const
{
	return this->skeletonRoots;
}

//------------------------------------------------------------------------------
/**
*/
float 
NFbxScene::TimeModeToFPS( const FbxTime::EMode& timeMode )
{
	switch (timeMode)
	{
	case FbxTime::eFrames100: return 100;
	case FbxTime::eFrames120: return 120;
	case FbxTime::eFrames1000: return 1000;
	case FbxTime::eFrames30: return 30;
	case FbxTime::eFrames30Drop: return 30;
	case FbxTime::eFrames48: return 48;
	case FbxTime::eFrames50: return 50;
	case FbxTime::eFrames60: return 60;
	case FbxTime::eNTSCDropFrame: return 29.97002617f;
	case FbxTime::eNTSCFullFrame: return 29.97002617f;
	case FbxTime::ePAL: return 25;
	case FbxTime::eCustom: return -1; // invalid mode, has to be handled separately
	default: return 24;

	}
}

//------------------------------------------------------------------------------
/**
	Flattens hierarchical structure by merging nodes recursively.
	Each node type defines their own DoMerge.
*/
void 
NFbxScene::Flatten()
{
	// create dictionary containing meshes
	Dictionary<String, Array<Ptr<NFbxMeshNode> > > meshNodes;
	IndexT numRoots = this->rootNodes.Size();
	IndexT rootIndex;

	// traverse root nodes and recursively merge trees
	for (rootIndex = 0; rootIndex < numRoots; rootIndex++)
	{
		Ptr<NFbxNode> rootNode = this->rootNodes[rootIndex];
		rootNode->MergeChildren(meshNodes);
	}

	// we should now be able to remove our meshes from the scene
	this->meshNodes.Clear();

	// get scene mesh
	MeshBuilder* rootMeshSource;

	// create group index
	IndexT groupIndex = 0;

	// create group index for physics
	IndexT physicsGroupIndex = 0;

	// the hierarchy is now flat, next task is to merge meshes with the same material
	IndexT i;
	for (i = 0; i < meshNodes.Size(); i++)
	{
		bool physics = meshNodes.KeyAtIndex(i) == "physics";

		// pick mesh source
		if (physics)
		{
			rootMeshSource = this->physicsMesh;
		}
		else
		{
			rootMeshSource = this->mesh;
		}
		
		// get list of meshes for material
		Array<Ptr<NFbxMeshNode>> meshList = meshNodes.ValueAtIndex(i);

		// treat first mesh as the root for which we should merge the others to
		Ptr<NFbxMeshNode> rootMesh = meshList[0];

		// create new list of fragments
		Array<Ptr<SkinFragment>> rootFragments;
		String nodeName;

		// begin bounding box extending, this should be cheaper than a complete recalculation
		Math::bbox mergedBox;
		mergedBox.begin_extend();

		// create counter for physics
		IndexT physicsNodeIndex = 0;

		IndexT k;
		for (k = 0; k < meshList.Size(); k++)
		{
			Ptr<NFbxMeshNode> meshNode = meshList[k];
			meshNode->SetGroupId(groupIndex);

			// add physics node to list of physics nodes
			if (physics)
			{
				// set new group id for physics
				meshNode->SetGroupId(physicsNodeIndex);

				// then add node to physics node
				this->physicsNodes.Append(meshNode);
			}
			
			MeshBuilder* meshSource = meshNode->GetMesh();
			SizeT vertCount = meshSource->GetNumVertices();
			IndexT vertIndex;
			SizeT triCount = meshSource->GetNumTriangles();
			IndexT triIndex;

			// get list of fragments
			const Util::Array<Ptr<SkinFragment>>& fragments = meshNode->GetSkinFragments();

			// extend bounding box
			mergedBox.extend(meshNode->GetBoundingBox());

			SizeT vertOffset = rootMeshSource->GetNumVertices();
			SizeT triOffset = rootMeshSource->GetNumTriangles();

			// finally just add vertices to root mesh
			for (vertIndex = 0; vertIndex < vertCount; vertIndex++)
			{
				// just use position if we have physics
				if (physics)
				{
					const MeshBuilderVertex& vert = meshSource->VertexAt(vertIndex);
					Math::float4 pos = vert.GetComponent(MeshBuilderVertex::CoordIndex);
					MeshBuilderVertex newVert;
					newVert.SetComponent(MeshBuilderVertex::CoordIndex, pos);
					rootMeshSource->AddVertex(newVert);
				}
				else
				{
					MeshBuilderVertex vert = meshSource->VertexAt(vertIndex);
					rootMeshSource->AddVertex(vert);
				}
				
			}

			IndexT fragIndex;
			for (fragIndex = 0; fragIndex < fragments.Size(); fragIndex++)
			{
				Ptr<SkinFragment> fragment = fragments[fragIndex];

				fragment->SetGroupId(groupIndex++);

				const Array<SkinFragment::TriangleIndex>& triangles = fragment->GetTriangles();
				for (triIndex = 0; triIndex < triangles.Size(); triIndex++)
				{
					// take old triangle
					MeshBuilderTriangle& tri = meshSource->TriangleAt(triangles[triIndex]);

					// update it
					tri.SetGroupId(fragment->GetGroupId());
				}
				
				rootFragments.Append(fragment);
			}

			// then add triangles and update vertex indices
			for (triIndex = 0; triIndex < triCount; triIndex++)
			{
				MeshBuilderTriangle tri = meshSource->TriangleAt(triIndex);
				tri.SetVertexIndex(0, vertOffset + tri.GetVertexIndex(0));
				tri.SetVertexIndex(1, vertOffset + tri.GetVertexIndex(1));
				tri.SetVertexIndex(2, vertOffset + tri.GetVertexIndex(2));

				if (fragments.Size() == 0)
				{
					if (physics)
					{
						tri.SetGroupId(physicsGroupIndex);
					}
					else
					{
						tri.SetGroupId(groupIndex);
					}
				}

				// add triangle to mesh
				rootMeshSource->AddTriangle(tri);
			}

			if (physics)
			{
				physicsGroupIndex++;
				physicsNodeIndex++;
			}
		}

		// clear mesh list
		meshList.Clear();
		mergedBox.end_extend();

		if (rootFragments.Size() == 0 && !physics)
		{
			groupIndex++;
		}

		// delete physics from node list
		if (physics)
		{
			meshNodes.EraseAtIndex(i--);
			physicsGroupIndex++;
		}
		else
		{
			// set root mesh stuff
			rootMesh->SetBoundingBox(mergedBox);
			rootMesh->SetSkinFragments(rootFragments);
			rootMesh->SetGroupId(i);

			// add mesh back
			meshList.Append(rootMesh);

			// now set list back
			meshNodes[meshNodes.KeyAtIndex(i)] = meshList;

			// add merged node to dictionaries
			this->meshNodes.Add(rootMesh->GetNode(), rootMesh);

			this->nodes.Add(rootMesh->GetNode(), rootMesh.upcast<NFbxNode>());		
			this->rootNodes.Append(rootMesh.upcast<NFbxNode>());
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxScene::FragmentSkins()
{
	// create skin partitioner
	Ptr<SkinPartitioner> skinPartitioner = SkinPartitioner::Create();
	SizeT numMeshes = this->meshNodes.Size();
	IndexT meshIndex;

	// go through meshes and fragment each skin
	for (meshIndex = 0; meshIndex < numMeshes; meshIndex++)
	{
		Ptr<NFbxMeshNode> mesh = this->meshNodes.ValueAtIndex(meshIndex);

		if (mesh->GetMeshFlags() & ToolkitUtil::HasSkin)
		{
			// create dummy mesh
			MeshBuilder* fragmentedMesh = new MeshBuilder;

			// create list of skin fragments
			Array<Ptr<SkinFragment> > fragments;

			// fragment again, but this time we only want the skin fragments for each individual piece
			skinPartitioner->FragmentSkin((*mesh->GetMesh()), *fragmentedMesh, fragments);

			// set fragments in skin
			mesh->SetSkinFragments(fragments);

			// delete old mesh
			delete mesh->GetMesh();

			// set fragmented mesh
			mesh->SetMesh(fragmentedMesh);
		}
	}
}

//------------------------------------------------------------------------------
/**
	Converts import mode to string.
	Each value separated by a pipe means the mesh has this specific feature, and is only compliant with materials with the same features.
*/
const Util::String 
NFbxScene::GetSceneFeatureString()
{
	Util::String featureString;
	switch(this->mode)
	{
	case Static:
		featureString = "static";
		break;
	case Skeletal:
		featureString = "skeletal";
		break;
	default:
		featureString = "static";
	}

	if (this->flags & ToolkitUtil::ImportColors) featureString += "|vertexcolors";
    if (this->flags & ToolkitUtil::ImportSecondaryUVs) featureString += "|secondaryuvs";
	return featureString;
}

}