//------------------------------------------------------------------------------
// meshnode.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "meshnode.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/base/vertexbufferbase.h"
#include "meshnodeinstance.h"
#include "graphics/meshentity.h"

using namespace CoreGraphics;
namespace Models
{

__ImplementClass(Models::MeshNode, 'MSND', Models::StateNode);
//------------------------------------------------------------------------------
/**
*/
MeshNode::MeshNode() :
	entity(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
MeshNode::~MeshNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Models::ModelNodeInstance>
MeshNode::CreateNodeInstance() const
{
	Ptr<Models::ModelNodeInstance> newInst = (Models::ModelNodeInstance*) MeshNodeInstance::Create();
	return newInst;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshNode::SetEntity(const Ptr<Graphics::MeshEntity>& entity)
{
	n_assert(entity.isvalid());
	this->entity = entity;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshNode::ApplySharedState(IndexT frameIndex)
{
	n_assert(this->prim.GetNumIndices() > 0 || this->prim.GetNumVertices() > 0)

	// first call parent class
	StateNode::ApplySharedState(frameIndex);
	this->entity->ApplyState(this->prim);
}

} // namespace Models