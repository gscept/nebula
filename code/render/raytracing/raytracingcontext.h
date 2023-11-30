#pragma once
//------------------------------------------------------------------------------
/**
    Context dealing with scene management for ray tracing

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/accelerationstructure.h"
#include "coregraphics/buffer.h"
#include <array>
#include "cluster_generate.h"

namespace Raytracing
{

class RaytracingContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:
    /// constructor
    RaytracingContext();
    /// destructor
    virtual ~RaytracingContext();

    /// Setup ray tracing context
    static void Create();

    /// Setup an entity for ray tracing, assumes model context registration
    static void Setup(const Graphics::GraphicsEntityId id);

    /// Build top level acceleration
    static void RebuildToplevelAcceleration();

#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
#endif

private:

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);

    static CoreGraphics::TopLevelAccelerationId ToplevelAccelerationStructure;
    static bool TopLevelNeedsRebuild;
    static bool ToplevelNeedsUpdate;

    enum
    {
        Raytracing_Allocation,
        Raytracing_NumStructures
    };
    typedef Ids::IdAllocator<
        Memory::RangeAllocation,
        uint
    > RaytracingAllocator;
    static RaytracingAllocator raytracingContextAllocator;

    static Util::Array<CoreGraphics::BottomLevelAccelerationId> BottomLevelAccelerationStructures;
    static Memory::RangeAllocator BottomLevelAccelerationAllocator;
};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
RaytracingContext::Alloc()
{
    return raytracingContextAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
inline void
RaytracingContext::Dealloc(Graphics::ContextEntityId id)
{
    // clean up old stuff, but don't deallocate entity
    Memory::RangeAllocation range = raytracingContextAllocator.Get<Raytracing_Allocation>(id.id);
    SizeT numAllocs = raytracingContextAllocator.Get<Raytracing_NumStructures>(id.id);
    for (IndexT i = range.offset; i < numAllocs; i++)
    {
        CoreGraphics::DestroyBottomLevelAcceleration(RaytracingContext::BottomLevelAccelerationStructures[i]);
    }
    raytracingContextAllocator.Dealloc(id.id);
    RaytracingContext::TopLevelNeedsRebuild = true;
}
} // namespace Raytracing
