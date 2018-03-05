#pragma once
//------------------------------------------------------------------------------
/**
	The transform node contains just a hierarchical transform
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/matrix44.h"
#include "math/transform44.h"
#include "math/quaternion.h"
namespace Models
{
class TransformNode : public ModelNode
{
public:
	/// constructor
	TransformNode();
	/// destructor
	virtual ~TransformNode();

	struct Instance : public ModelNode::Instance
	{
		Math::transform44 transform;
		Math::matrix44 modelTransform;
		bool isInViewSpace;
		bool lockedToViewer;
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(Memory::ChunkAllocator<0xFFF>& alloc) const;

protected:
	friend class StreamModelPool;
	friend class ModelContext;

	/// load transform
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);

	Math::point position;
	Math::quaternion rotate;
	Math::vector scale;
	Math::point rotatePivot;
	Math::point scalePivot;
	bool isInViewSpace;
	float minDistance;
	float maxDistance;
	bool useLodDistances;
	bool lockedToViewer;
};


//------------------------------------------------------------------------------
/**
*/
inline ModelNode::Instance*
TransformNode::CreateInstance(Memory::ChunkAllocator<0xFFF>& alloc) const
{
	TransformNode::Instance* tnode = alloc.Alloc<TransformNode::Instance>();
	tnode->transform.setposition(this->position);
	tnode->transform.setrotate(this->rotate);
	tnode->transform.setscale(this->scale);
	tnode->transform.setrotatepivot(this->rotatePivot);
	tnode->transform.setscalepivot(this->scalePivot);
	return tnode;
}

} // namespace Models