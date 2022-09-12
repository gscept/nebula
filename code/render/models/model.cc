//------------------------------------------------------------------------------
// model.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "model.h"
#include "coregraphics/config.h"

namespace Models
{

ModelAllocator modelAllocator;
//------------------------------------------------------------------------------
/**
*/
const ModelId
CreateModel(const ModelCreateInfo& info)
{
    Ids::Id32 id = modelAllocator.Alloc();
    modelAllocator.Set<Model_BoundingBox>(id, info.boundingBox);
    modelAllocator.Set<Model_Nodes>(id, info.nodes);

    ModelId ret;
    ret.resourceId = id;
    ret.resourceType = CoreGraphics::ModelIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyModel(const ModelId id)
{
    modelAllocator.Get<Model_Nodes>(id.resourceId).Clear();
    modelAllocator.Dealloc(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode*>&
ModelGetNodes(const ModelId id)
{
    return modelAllocator.Get<Model_Nodes>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
ModelGetBoundingBox(const ModelId id)
{
    return modelAllocator.Get<Model_BoundingBox>(id.resourceId);
}

} // namespace Models
