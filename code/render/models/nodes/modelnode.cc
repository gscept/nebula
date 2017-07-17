//------------------------------------------------------------------------------
// modelnode.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelnode.h"

using namespace Util;
using namespace Math;
namespace Models
{

__ImplementClass(Models::ModelNode, 'MONO', Core::RefCounted);
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
ModelNode::Load(const Util::FourCC& tag, const Ptr<Models::ModelLoader>& loader, const Ptr<IO::BinaryReader>& reader)
{
	if (FourCC('LBOX') == tag)
	{
		// bounding box
		point center = reader->ReadFloat4();
		vector extents = reader->ReadFloat4();
		this->boundingBox.set(center, extents);
	}
	else if (FourCC('MNTP') == tag)
	{
		// model node type, deprecated
		reader->ReadString();
	}

	else if (FourCC('SSTA') == tag)
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
			fourCC.AsString().AsCharPtr(),
			reader->GetStream()->GetURI().AsString().AsCharPtr());
		return false;
	}
	return true;
}

} // namespace Models