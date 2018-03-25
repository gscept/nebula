//------------------------------------------------------------------------------
//  skinpartitioner.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "skinpartitioner.h"
#include "toolkitutil/meshutil/meshbuildervertex.h"
#include "toolkitutil/meshutil/meshbuildertriangle.h"
#include "skinfragment.h"


using namespace ToolkitUtil;
using namespace Math;
using namespace Util;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::SkinPartitioner, 'SKPR', Core::RefCounted);


//------------------------------------------------------------------------------
/**
*/
SkinPartitioner::SkinPartitioner()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SkinPartitioner::~SkinPartitioner()
{
	// empty
}

//------------------------------------------------------------------------------
/**
	Takes the source mesh, splits it, writes result to destination mesh and a list of fragments to the fragment list
*/
void 
SkinPartitioner::FragmentSkin( const ToolkitUtil::MeshBuilder& sourceMesh, ToolkitUtil::MeshBuilder& destinationMesh, Util::Array<Ptr<SkinFragment> >& fragmentList )
{
	int numTriangles = sourceMesh.GetNumTriangles();
	for (int triIndex = 0; triIndex < numTriangles; triIndex++)
	{
		const MeshBuilderTriangle& triangle = sourceMesh.TriangleAt(triIndex);

		bool fragmentFilled = true;
		Ptr<SkinFragment> fragment;
		for (int fragmentIndex = 0; fragmentIndex < fragmentList.Size(); fragmentIndex++)
		{
			 fragment = fragmentList[fragmentIndex];
			 if (fragment->GetGroupId() == triangle.GetGroupId())
			 {
				 if (fragment->AddTriangle(triIndex))
				 {
					fragmentFilled = false;
					break;
				 }
			 }
		}
		if (fragmentFilled)
		{
			fragment = SkinFragment::Create();
			fragment->SetMesh(sourceMesh);
			fragment->SetGroupId(triangle.GetGroupId());

			// this has to work because the fragment is newly created
			fragmentFilled = fragment->AddTriangle(triIndex);
			n_assert(fragmentFilled);

			// the group ID will be determined based on the frag count, so frag 0 will be its own group
			fragmentList.Append(fragment);
		}		
	}

	this->UpdateMesh(sourceMesh, destinationMesh, fragmentList);
}

//------------------------------------------------------------------------------
/**
*/
void 
SkinPartitioner::UpdateMesh( const MeshBuilder& sourceMesh, MeshBuilder& destinationMesh, const Util::Array<Ptr<SkinFragment> >& fragmentList )
{
	destinationMesh.Clear();
	for (int fragIndex = 0; fragIndex < fragmentList.Size(); fragIndex++)
	{
		const Ptr<SkinFragment>& fragment = fragmentList[fragIndex];
		fragment->SetGroupId(fragIndex);

		const Array<SkinFragment::TriangleIndex>& triangles = fragment->GetTriangles();
		for (int triIndex = 0; triIndex < triangles.Size(); triIndex++)
		{
			// copy old triangle
			MeshBuilderTriangle newTri = sourceMesh.TriangleAt(triangles[triIndex]);

			int vertexIndices[3];
			int newVertexIndices[3];
			// retrieve old vertex indices
			newTri.GetVertexIndices(vertexIndices[0], vertexIndices[1], vertexIndices[2]);

			for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++)
			{
				MeshBuilderVertex vertex = sourceMesh.VertexAt(vertexIndices[vertexIndex]);

				// figure out if we use UByte4 indices or unpacked indices
				MeshBuilderVertex::ComponentIndex index;
				if (vertex.HasComponent(MeshBuilderVertex::JIndicesBit)) index = MeshBuilderVertex::JIndicesIndex;
				if (vertex.HasComponent(MeshBuilderVertex::JIndicesUB4Bit)) index = MeshBuilderVertex::JIndicesUB4Index;

				// convert global indices to partition-local indices
				const float4& globalIndices = vertex.GetComponent(index);
				float4 localIndices;
				localIndices.x() = (float)fragment->GetLocalJointIndex((int)globalIndices.x());
				localIndices.y() = (float)fragment->GetLocalJointIndex((int)globalIndices.y());
				localIndices.z() = (float)fragment->GetLocalJointIndex((int)globalIndices.z());
				localIndices.w() = (float)fragment->GetLocalJointIndex((int)globalIndices.w());

				newVertexIndices[vertexIndex] = destinationMesh.GetNumVertices();
				vertex.SetComponent(index, localIndices);
				destinationMesh.AddVertex(vertex);
			}

			// change information from the copy
			newTri.SetGroupId(fragIndex);
			newTri.SetVertexIndices(newVertexIndices[0], newVertexIndices[1], newVertexIndices[2]);
			destinationMesh.AddTriangle(newTri);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SkinPartitioner::Defragment( const Util::Array<Ptr<SkinFragment> >& fragmented, Util::Array<Ptr<SkinFragment> >& defragmented, MeshBuilder& mesh, IndexT& baseGroupId )
{
	n_assert(fragmented.Size() > 0);

	// we need to collect all joint indices in groups of 72
	Array<Array<SkinFragment::JointIndex> > indexMap;

	// save index to 72 joint chunk
	IndexT chunk = 0;

	// create initial skin fragment
	Ptr<SkinFragment> newFragment = SkinFragment::Create();

	// set group id of fragment
	newFragment->SetGroupId(baseGroupId);

	// add it to defragmented list
	defragmented.Append(newFragment);

	// add first array to map
	indexMap.Append(Array<SkinFragment::JointIndex>());

	// first go through all fragments
	IndexT fragIndex;
	for (fragIndex = 0; fragIndex < fragmented.Size(); fragIndex++)
	{
		// get fragment
		Ptr<SkinFragment> fragment = fragmented[fragIndex];

		// go through all joints in fragment
		IndexT jointIndex;
		for (jointIndex = 0; jointIndex < fragment->GetJointPalette().Size(); jointIndex++)
		{
			// get joint
			const SkinFragment::JointIndex joint = fragment->GetJointPalette()[jointIndex];
			if (indexMap[chunk].FindIndex(joint) == InvalidIndex)
			{
				// if we are about to add to our chunk, but chunk is full, create a new one
				if (indexMap[chunk].Size() == MAXJOINTSPERFRAGMENT)
				{
					indexMap.Append(Array<SkinFragment::JointIndex>());

					// also create new fragment
					newFragment = SkinFragment::Create();

					// add it to list
					defragmented.Append(newFragment);

					// set group id of fragment
					newFragment->SetGroupId(baseGroupId + chunk);

					// increase chunk counter and base group index value
					chunk++;
					baseGroupId++;
				}

				// append chunk to map
				indexMap[chunk].Append(joint);

				// add joint to new fragment
				newFragment->InsertJoint(jointIndex);
			}
		}

		// now simply copy triangles from old fragment to new
		const Array<SkinFragment::TriangleIndex>& origTris = fragment->GetTriangles();

		IndexT triIndex;
		for (triIndex = 0; triIndex < origTris.Size(); triIndex++)
		{
			// copy triangle
			newFragment->InsertTriangle(origTris[triIndex]);

			// get triangle from mesh and change group id of triangle
			MeshBuilderTriangle& tri = mesh.TriangleAt(origTris[triIndex]);

			// set group id of triangle
			tri.SetGroupId(baseGroupId + chunk);
		}
	}
}
} // namespace ToolkitUtil