//------------------------------------------------------------------------------
// modelnode.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "modelnode.h"

using namespace Util;
using namespace Math;
namespace Models
{

//------------------------------------------------------------------------------
/**
*/
ModelNode::ModelNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelNode::~ModelNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelNode::Instance*
ModelNode::CreateInstance(Memory::ChunkAllocator<0xFFF>& alloc) const
{
	return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader)
{
	if (FourCC('LBOX') == fourcc)
	{
		// bounding box
		point center = reader->ReadFloat4();
		vector extents = reader->ReadFloat4();
		this->boundingBox.set(center, extents);
	}
	else if (FourCC('MNTP') == fourcc)
	{
		// model node type, deprecated
		reader->ReadString();
	}

	else if (FourCC('SSTA') == fourcc)
	{
		// string attribute, deprecated
		StringAtom key = reader->ReadString();
		String value = reader->ReadString();
		//this->SetStringAttr(key, value);
	}
	else
	{
		// throw error on unknown tag (we can't skip unknown tags)
		n_error("ModelNode::Load: unknown data tag '%s' in '%s'!",
			tag.AsString().AsCharPtr(),
			reader->GetStream()->GetURI().AsString().AsCharPtr());
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNode::Unload()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNode::OnFinishedLoading()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNode::Setup()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNode::Discard()
{
	// override in subclass
}

} // namespace Models