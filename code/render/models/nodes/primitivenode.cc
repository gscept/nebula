//------------------------------------------------------------------------------
// primitivenode.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "primitivenode.h"
#include "models/modelloader.h"

using namespace Util;
namespace Models
{

__ImplementClass(Models::PrimitiveNode, 'SPND', Models::ShaderStateNode);
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
PrimitiveNode::Load(const Util::FourCC& tag, const Ptr<Models::ModelLoader>& loader, const Ptr<IO::BinaryReader>& reader)
{
	bool retval = true;
	if (FourCC('MESH') == tag)
	{
		// get mesh resource
		this->meshName = reader->ReadString();

		// add as pending resource in loader
		loader->pendingResources.Append(this->meshName);
	}
	else if (FourCC('PGRI') == tag)
	{
		// primitive group index
		this->primitiveGroupIndex = reader->ReadUInt();
	}
	else
	{
		retval = ShaderStateNode::Load(tag, reader);
	}
	return retval;
}

} // namespace Models