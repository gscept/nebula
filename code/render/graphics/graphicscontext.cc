//------------------------------------------------------------------------------
// graphicscontext.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "graphicscontext.h"

namespace Graphics
{

//------------------------------------------------------------------------------
/**
*/
GraphicsContext::GraphicsContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GraphicsContext::~GraphicsContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::InternalRegisterEntity(const Graphics::GraphicsEntityId id, Graphics::GraphicsContextState&& state)
{
    n_assert(!state.entitySliceMap.Contains(id));
    Graphics::ContextEntityId allocId = state.Alloc();
    state.entitySliceMap.Add(id, allocId);
    if ((unsigned)state.entities.Size() <= allocId.id)
    {
        if ((unsigned)state.entities.Capacity() <= allocId.id)
        {
            state.entities.Grow();
        }
        state.entities.Resize(allocId.id + 1);
    }
    state.entities[allocId.id] = id;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::InternalDeregisterEntity(const Graphics::GraphicsEntityId id, Graphics::GraphicsContextState&& state)
{
    IndexT i = state.entitySliceMap.FindIndex(id);
    n_assert(i != InvalidIndex);
    if (state.allowedRemoveStages & state.currentStage)
    {
        state.Dealloc(state.entitySliceMap.ValueAtIndex(id, i));
        state.entitySliceMap.EraseIndex(id, i);
    }
    else
    {
        state.delayedRemoveQueue.Append(id);
    }
}

} // namespace Graphics