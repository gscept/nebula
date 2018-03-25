//------------------------------------------------------------------------------
// primitivenode.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "primitivenode.h"
#include "models/modelpool.h"
#include "resources/resourcemanager.h"
#include "coregraphics/mesh.h"

using namespace Util;
namespace Models
{

//------------------------------------------------------------------------------
/**
*/
PrimitiveNode::PrimitiveNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
PrimitiveNode::~PrimitiveNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
PrimitiveNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader)
{
	bool retval = true;
	if (FourCC('MESH') == fourcc)
	{
		// get mesh resource
		this->meshName = reader->ReadString();

		// add as pending resource in loader
		this->res = Resources::CreateResource(this->meshName, tag, nullptr, nullptr, false);
	}
	else if (FourCC('PGRI') == fourcc)
	{
		// primitive group index
		this->primitiveGroupIndex = reader->ReadUInt();
	}
	else
	{
		retval = ShaderStateNode::Load(fourcc, tag, reader);
	}
	return retval;
}

} // namespace Models