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

struct RaytracingSetupSettings
{
    SizeT maxNumAllowedInstances;
    const Ptr<Frame::FrameScript>& script;
};

class RaytracingContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:
    /// constructor
    RaytracingContext();
    /// destructor
    virtual ~RaytracingContext();

    /// Setup ray tracing context
    static void Create(const RaytracingSetupSettings& settings);

    /// Setup an entity for ray tracing, assumes model context registration
    static void Setup(const Graphics::GraphicsEntityId id);

    /// Build top level acceleration
    static void ReconstructTopLevelAcceleration(const Graphics::FrameContext& ctx);
    /// Update transforms
    static void UpdateTransforms(const Graphics::FrameContext& ctx);
    /// Wait for jobs to finish
    static void WaitForJobs(const Graphics::FrameContext& ctx);

#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
#endif

private:

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);

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
};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
RaytracingContext::Alloc()
{
    return raytracingContextAllocator.Alloc();
}
} // namespace Raytracing
