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
    modelAllocator.Set<Model_JointMasks>(id, info.jointMasks);
    modelAllocator.Set<Model_Takes>(id, info.takes);

#if WITH_NEBULA_EDITOR
    modelAllocator.Set<Model_NodeLookup>(id, info.nodeLookup);
#endif

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

#if WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
/**
*/
const
Util::Dictionary<Util::StringAtom, Models::ModelNode*>& GetModelNodeTable(const ModelId id)
{
    return modelAllocator.Get<Model_NodeLookup>(id.id);
}

#endif

} // namespace Models
