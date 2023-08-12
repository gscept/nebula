//------------------------------------------------------------------------------
// transformnode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "transformnode.h"

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
    this->bits = HasTransformBit;
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
void
TransformNode::GetLODDistances(float& minDistance, float& maxDistance)
{
    if (this->useLodDistances)
    {
        minDistance = this->minDistance;
        maxDistance = this->maxDistance;
    }
    else
    {
        minDistance = maxDistance = FLT_MAX;
    }
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
        this->maxDistance = Math::max(this->minDistance, this->maxDistance);
        this->useLodDistances = true;
    }
    else if (FourCC('SMAD') == fourcc)
    {
        this->maxDistance = reader->ReadFloat();
        this->minDistance = Math::min(this->minDistance, this->maxDistance);
        this->useLodDistances = true;
    }
    else
    {
        retval = ModelNode::Load(fourcc, tag, reader, immediate);
    }
    return retval;
}

} // namespace Models
