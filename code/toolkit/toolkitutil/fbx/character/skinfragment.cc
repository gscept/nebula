//------------------------------------------------------------------------------
//  skinfragment.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "skinfragment.h"

using namespace ToolkitUtil;
using namespace Math;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::SkinFragment, 'SKFR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
SkinFragment::SkinFragment()
{
	memset(this->jointMask, 0, sizeof(this->jointMask));
}

//------------------------------------------------------------------------------
/**
*/
SkinFragment::~SkinFragment()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
SkinFragment::ExtractJointPalette( const MeshBuilder& sourceMesh, const MeshBuilderTriangle& triangle, Util::Array<JointIndex>& jointIndices )
{
	int vertexIndices[3];
	triangle.GetVertexIndices(vertexIndices[0], vertexIndices[1], vertexIndices[2]);

	for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++)
	{
		const MeshBuilderVertex& vertex = sourceMesh.VertexAt(vertexIndices[vertexIndex]);
		const float4& weights = vertex.GetComponent(MeshBuilderVertex::WeightsUB4NIndex);

		// figure out what type of indices we are using
		MeshBuilderVertex::ComponentIndex index;
		if (vertex.HasComponent(MeshBuilderVertex::JIndicesBit)) index = MeshBuilderVertex::JIndicesIndex;
		if (vertex.HasComponent(MeshBuilderVertex::JIndicesUB4Bit)) index = MeshBuilderVertex::JIndicesUB4Index;
		const float4& indices = vertex.GetComponent(index);

		if (jointIndices.FindIndex((int)indices.x()) == InvalidIndex)
		{
			jointIndices.Append((int)indices.x());
		}
		if (jointIndices.FindIndex((int)indices.y()) == InvalidIndex)
		{
			jointIndices.Append((int)indices.y());
		}
		if (jointIndices.FindIndex((int)indices.z()) == InvalidIndex)
		{
			jointIndices.Append((int)indices.z());
		}
		if (jointIndices.FindIndex((int)indices.w()) == InvalidIndex)
		{
			jointIndices.Append((int)indices.w());
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SkinFragment::GetUniqueJointList( const Util::Array<JointIndex>& jointIndices, Util::Array<JointIndex>& uniqueList )
{
	for (int jointIndex = 0; jointIndex < jointIndices.Size(); jointIndex++)
	{
		if (!this->jointMask[jointIndices[jointIndex]])
		{
			uniqueList.Append(jointIndices[jointIndex]);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
bool 
SkinFragment::AddTriangle( TriangleIndex triangleIndex )
{
	const MeshBuilderTriangle& triangle = this->mesh.TriangleAt(triangleIndex);

	// extract joints for the triangle
	Util::Array<JointIndex> triangleJoints;
	this->ExtractJointPalette(this->mesh, triangle, triangleJoints);

	// exclude non-unique indices
	Util::Array<JointIndex> uniqueJoints;
	this->GetUniqueJointList(triangleJoints, uniqueJoints);

	if ((this->jointPalette.Size() + uniqueJoints.Size()) <= MAXJOINTSPERFRAGMENT)
	{
		this->triangles.Append(triangleIndex);
		for (int index = 0; index < uniqueJoints.Size(); index++)
		{
			this->jointPalette.Append(uniqueJoints[index]);
			// set the used joint to true
			this->jointMask[uniqueJoints[index]] = true;
		}
		return true;
	}

	// getting here means this partition has been filled (joint count == 128), so there!
	return false;
}

//------------------------------------------------------------------------------
/**
*/
SkinFragment::JointIndex 
SkinFragment::GetLocalJointIndex( JointIndex globalIndex )
{
	for (int jointIndex = 0; jointIndex < this->jointPalette.Size(); jointIndex++)
	{
		if (globalIndex == this->jointPalette[jointIndex])
		{
			return jointIndex;
		}
	}

	// the joint is not in this partition
	return 0;
}

} // namespace ToolkitUtil