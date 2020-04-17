//------------------------------------------------------------------------------
// transformnode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "transformnode.h"
#include "coregraphics/transformdevice.h"

using namespace Util;
namespace Models
{

//------------------------------------------------------------------------------
/**
*/
TransformNode::TransformNode() :
	position(0.0f, 0.0f, 0.0f),
	rotate(0.0f, 0.0f, 0.0f, 1.0f),
	scale(1.0f, 1.0f, 1.0f),
	rotatePivot(0.0f, 0.0f, 0.0f),
	scalePivot(0.0f, 0.0f, 0.0f),
	isInViewSpace(false),
	minDistance(0.0f),
	maxDistance(10000.0f),
	useLodDistances(false),
	lockedToViewer(false)
{
	this->type = TransformNodeType;
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
TransformNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate)
{
	bool retval = true;
	if (FourCC('POSI') == fourcc)
	{
		// position
		this->position = xyz(reader->ReadVec4());
	}
	else if (FourCC('ROTN') == fourcc)
	{
		// rotation
		this->rotate = reader->ReadVec4();
	}
	else if (FourCC('SCAL') == fourcc)
	{
		// scale
		this->scale = xyz(reader->ReadVec4());
	}
	else if (FourCC('RPIV') == fourcc)
	{
		this->rotatePivot = xyz(reader->ReadVec4());
	}
	else if (FourCC('SPIV') == fourcc)
	{
		this->scalePivot = xyz(reader->ReadVec4());
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
		retval = ModelNode::Load(fourcc, tag, reader, immediate);
	}
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
TransformNode::Instance::Update()
{
	CoreGraphics::TransformDevice* transformDevice = CoreGraphics::TransformDevice::Instance();
	transformDevice->SetModelTransform(this->modelTransform);
	transformDevice->SetObjectId(this->objectId);
}

} // namespace Models