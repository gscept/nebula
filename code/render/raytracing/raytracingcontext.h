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
#include "system_shaders/cluster_generate.h"
#include "materials/materialtemplates.h"
#include "materials/shaderconfig.h"

namespace Raytracing
{

struct RaytracingSetupSettings
{
    SizeT maxNumAllowedInstances;
};

enum ObjectType
{
    BRDFObject,
    BSDFObject,
    GLTFObject,
    ParticleObject
};

enum UpdateType
{
    Dynamic,
    Static
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
    ///
    static void Discard();

    /// Setup a model entity for ray tracing, assumes model context registration
    static void SetupModel(const Graphics::GraphicsEntityId id, CoreGraphics::BlasInstanceFlags flags, uchar mask);
    /// Setup a terrain system for ray tracing
    static void SetupMesh(
        const Graphics::GraphicsEntityId id
        , const UpdateType objectType
        , const CoreGraphics::VertexComponent::Format format
        , const CoreGraphics::IndexType::Code indexType
        , const CoreGraphics::VertexAlloc& vertices
        , const CoreGraphics::VertexAlloc& indices
        , const CoreGraphics::PrimitiveGroup& patchPrimGroup
        , const SizeT vertexOffsetStride
        , const SizeT patchVertexStride
        , const Util::Array<Math::mat4> transforms
        , const uint materialTableOffset
        , const MaterialTemplates::MaterialProperties shader
        , const CoreGraphics::VertexLayoutType vertexLayout
    );
    /// Invalidate a BLAS (for example, when the mesh has morphed) associated with a graphics entity
    static void InvalidateBLAS(const Graphics::GraphicsEntityId id);

    /// Build top level acceleration
    static void ReconstructTopLevelAcceleration(const Graphics::FrameContext& ctx);
    /// Update transforms
    static void UpdateTransforms(const Graphics::FrameContext& ctx);
    /// Wait for jobs to finish
    static void WaitForJobs(const Graphics::FrameContext& ctx);
    /// Update view constants
    static void UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

    /// Get light grid resources
    static CoreGraphics::ResourceTableId GetLightGridResourceTable();
    /// Get TLAS
    static CoreGraphics::TlasId GetTLAS();
    /// Get object binding buffer
    static CoreGraphics::BufferId GetObjectBindingBuffer();
    /// Get raytracing table
    static CoreGraphics::ResourceTableId GetRaytracingTable(const IndexT bufferIndex);

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
        Raytracing_UpdateType,
        Raytracing_NumStructures,
        Raytracing_Blases
    };
    typedef Ids::IdAllocator<
        Memory::RangeAllocation,
        Raytracing::UpdateType,
        uint,
        Util::FixedArray<CoreGraphics::BlasId>
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
