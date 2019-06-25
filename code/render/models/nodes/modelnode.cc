//------------------------------------------------------------------------------
// modelnode.cc
// (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "modelnode.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/resourcetable.h"

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
bool
ModelNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate)
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
			fourcc.AsString().AsCharPtr(),
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
ModelNode::Discard()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
bool 
ModelNode::GetImplicitHierarchyActivation() const
{
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNode::ApplyNodeState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ModelNode::Instance::GetDrawPacketSize() const
{
	// implement in sublcass
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
Models::ModelNode::DrawPacket*
ModelNode::Instance::UpdateDrawPacket(void* mem)
{
	// implement in sublcass
	return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNode::Instance::Update()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNode::Instance::Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	this->node = node;
	this->parent = parent;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelNode::Instance::Draw()
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelNode::DrawPacket::Apply()
{
	// apply surface
	if (*this->surfaceInstance != Materials::SurfaceInstanceId::Invalid())
		Materials::MaterialApplySurfaceInstance(*this->surfaceInstance);

	// set resource tables
	for (IndexT i = 0; i < *this->numTables; i++)
		CoreGraphics::SetResourceTable(this->tables[i], this->slots[i], this->pipelines[i], this->numOffsets[i], &this->offsets[i]);
}

} // namespace Models