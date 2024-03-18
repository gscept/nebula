//------------------------------------------------------------------------------
// model.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

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

    ModelId ret = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyModel(const ModelId id)
{
    modelAllocator.Get<Model_Nodes>(id.id).Clear();
    modelAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode*>&
ModelGetNodes(const ModelId id)
{
    return modelAllocator.Get<Model_Nodes>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
ModelGetBoundingBox(const ModelId id)
{
    return modelAllocator.Get<Model_BoundingBox>(id.id);
}

} // namespace Models
