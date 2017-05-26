#pragma once
//------------------------------------------------------------------------------
/**
	A mesh node encapsulates a mesh defined in code, and not in a model resource.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "statenode.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/primitivegroup.h"

namespace Graphics
{
class MeshEntity;
}

namespace Models
{
class MeshNode : public StateNode
{
	__DeclareClass(MeshNode);
public:

	/// constructor
	MeshNode();
	/// destructor
	virtual ~MeshNode();

	/// create a model node instance
	virtual Ptr<Models::ModelNodeInstance> CreateNodeInstance() const;

	/// set primitive group within the vertex/index buffers
	void SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& group);
	/// set surface
	void SetSurface(const Resources::ResourceId& resource);
	/// set mesh entity used to create this node
	void SetEntity(const Ptr<Graphics::MeshEntity>& entity);

	/// apply shared state, which means ibo, vbo and primitive group
	virtual void ApplySharedState(IndexT frameIndex);
private:
	CoreGraphics::PrimitiveGroup prim;
	Ptr<Graphics::MeshEntity> entity;
};

//------------------------------------------------------------------------------
/**
*/
inline void
MeshNode::SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& group)
{
	this->prim = group;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshNode::SetSurface(const Resources::ResourceId& resource)
{
	this->materialName = resource;
}

} // namespace Models