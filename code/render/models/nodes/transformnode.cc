//------------------------------------------------------------------------------
// transformnode.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformnode.h"

using namespace Util;
namespace Models
{

__ImplementClass(Models::TransformNode, 'TRFN', Models::ModelNode);
//------------------------------------------------------------------------------
/**
*/
TransformNode::TransformNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TransformNode::~TransformNode()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
bool
TransformNode::Load(const Util::FourCC& tag, const Ptr<Models::ModelLoader>& loader, const Ptr<IO::BinaryReader>& reader)
{
	bool retval = true;
	if (FourCC('POSI') == tag)
	{
		// position
		this->position = reader->ReadFloat4();
	}
	else if (FourCC('ROTN') == tag)
	{
		// rotation
		this->rotate = reader->ReadFloat4();
	}
	else if (FourCC('SCAL') == tag)
	{
		// scale
		this->scale = reader->ReadFloat4();
	}
	else if (FourCC('RPIV') == tag)
	{
		this->rotatePivot = reader->ReadFloat4();
	}
	else if (FourCC('SPIV') == tag)
	{
		this->scalePivot = reader->ReadFloat4();
	}
	else if (FourCC('SVSP') == tag)
	{
		this->isInViewSpace = reader->ReadBool();
	}
	else if (FourCC('SLKV') == tag)
	{
		this->lockedToViewer = reader->ReadBool());
	}
	else if (FourCC('SMID') == tag)
	{
		this->minDistance = reader->ReadFloat();
	}
	else if (FourCC('SMAD') == tag)
	{
		this->maxDistance = reader->ReadFloat();
	}
	else
	{
		retval = ModelNode::Load(tag, reader);
	}
	return retval;
}

} // namespace Models