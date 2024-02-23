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
#include "system_shaders/cluster_generate.h"

namespace Raytracing
{

struct RaytracingSetupSettings
{
    SizeT maxNumAllowedInstances;
    const Ptr<Frame::FrameScript>& script;
};

enum ObjectType
{
    BRDFObject,
    BSDFObject,
    GLTFObject,
    ParticleObject
};

class RaytracingContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:
    /// constructor
    RaytracingContext();
    /// destructor
    ~RaytracingContext();

    /// Setup ray tracing context
    static void Create(const RaytracingSetupSettings& settings);

    /// Setup a model entity for ray tracing, assumes model context registration
    static void SetupModel(const Graphics::GraphicsEntityId id, CoreGraphics::BlasInstanceFlags flags, uchar mask);
    /// Setup a terrain system for ray tracing
    static void SetupTerrain(
        const Graphics::GraphicsEntityId id
        , const CoreGraphics::VertexComponent::Format format
        , const CoreGraphics::IndexType::Code indexType
        , const CoreGraphics::VertexAlloc& vertices
        , const CoreGraphics::VertexAlloc& indices
        , const CoreGraphics::PrimitiveGroup& patchPrimGroup
        , SizeT vertexOffsetStride
        , Util::Array<Math::mat4> transforms
        , MaterialInterface::TerrainMaterial material
        
    );

    /// Build top level acceleration
    static void ReconstructTopLevelAcceleration(const Graphics::FrameContext& ctx);
    /// Update transforms
    static void UpdateTransforms(const Graphics::FrameContext& ctx);
    /// Wait for jobs to finish
    static void WaitForJobs(const Graphics::FrameContext& ctx);
    /// Update view constants
    static void UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

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
