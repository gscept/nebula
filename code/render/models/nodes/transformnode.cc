//------------------------------------------------------------------------------
// transformnode.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "transformnode.h"

using namespace Util;
namespace Models
{

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
TransformNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader)
{
	bool retval = true;
	if (FourCC('POSI') == fourcc)
	{
		// position
		this->position = reader->ReadFloat4();
	}
	else if (FourCC('ROTN') == fourcc)
	{
		// rotation
		this->rotate = reader->ReadFloat4();
	}
	else if (FourCC('SCAL') == fourcc)
	{
		// scale
		this->scale = reader->ReadFloat4();
	}
	else if (FourCC('RPIV') == fourcc)
	{
		this->rotatePivot = reader->ReadFloat4();
	}
	else if (FourCC('SPIV') == fourcc)
	{
		this->scalePivot = reader->ReadFloat4();
	}
	else if (FourCC('SVSP') == fourcc)
	{
		this->isInViewSpace = reader->ReadBool();
	}
	else if (FourCC('SLKV') == fourcc)
	{
		this->lockedToViewer = reader->ReadBool();
	}
	else if (FourCC('SMID') == fourcc)
	{
		this->minDistance = reader->ReadFloat();
	}
	else if (FourCC('SMAD') == fourcc)
	{
		this->maxDistance = reader->ReadFloat();
	}
	else
	{
		retval = ModelNode::Load(fourcc, tag, reader);
	}
	return retval;
}

} // namespace Models