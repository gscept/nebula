//------------------------------------------------------------------------------
// modelnode.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "modelnode.h"

using namespace Util;
using namespace Math;
namespace Models
{

IndexT ModelNode::ModelNodeUniqueIdCounter = 0;
//------------------------------------------------------------------------------
/**
*/
ModelNode::ModelNode() :
    uniqueId(ModelNodeUniqueIdCounter++)
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
        vec3 center = xyz(reader->ReadVec4());
        vec3 extents = xyz(reader->ReadVec4());
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
        UNUSED(StringAtom) key = reader->ReadString();
        UNUSED(String) value = reader->ReadString();
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
ModelNode::OnFinishedLoading(ModelStreamingData* streamingData)
{
    this->uniqueId = ModelNodeUniqueIdCounter++;
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

} // namespace Models
