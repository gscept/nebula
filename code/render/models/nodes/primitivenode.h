#pragma once
//------------------------------------------------------------------------------
/**
	A primitive node contains a mesh resource and a primitive group id.

	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/bbox.h"
#include "coregraphics/primitivegroup.h"
#include "resources/resourceid.h"
#include "shaderstatenode.h"
#include "coregraphics/mesh.h"
namespace Models
{
class PrimitiveNode : public ShaderStateNode
{
public:
	/// constructor
	PrimitiveNode();
	/// destructor
	virtual ~PrimitiveNode();

	struct Instance : public ShaderStateNode::Instance
	{
		// empty
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(Memory::ChunkAllocator<0xFFF>& alloc) const;

protected:
	friend class StreamModelPool;

	/// load primitive
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);

	Resources::ResourceName meshName;
	CoreGraphics::MeshId res;
	uint32_t primitiveGroupIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline ModelNode::Instance*
PrimitiveNode::CreateInstance(Memory::ChunkAllocator<0xFFF>& alloc) const
{
	return alloc.Alloc<PrimitiveNode::Instance>();
}

} // namespace Models