#pragma once
//------------------------------------------------------------------------------
/**
    The DDGI context is responsible for managing the GI volumes used to apply
    indirect light in the scene. 

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
namespace GI
{

class DDGIContext : public Graphics::GraphicsContext
{
    __DeclareContext()
public:
    /// Constructor
    DDGIContext();
    /// Destructor
    ~DDGIContext();

    /// setup light context
    static void Create();
    /// discard light context
    static void Discard();

    struct VolumeSetup
    {
        uint numProbesX, numProbesY, numProbesZ;
        uint numRaysPerProbe;
        Math::vec3 size;
        Math::vec3 position;
    };

    /// Create volume
    static void SetupVolume(const Graphics::GraphicsEntityId id, const VolumeSetup& setup);
    /// Set volume position
    static void SetPosition(const Graphics::GraphicsEntityId id, const Math::vec3& position);
    /// Set volume scale
    static void SetSize(const Graphics::GraphicsEntityId id, const Math::vec3& size);

    /// prepare light lists
    static void UpdateActiveVolumes(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

#ifndef PUBLIC_BUILD
    static void OnRenderDebug(uint32_t flags);
#endif

private:

    struct Volume
    {
        uint numProbesX, numProbesY, numProbesZ;
        uint numRaysPerProbe;
        Math::vec3 size;
        Math::vec3 position;
        Math::bbox boundingBox;
        CoreGraphics::TextureId radiance, depth;
        CoreGraphics::BufferId probeBuffer;
        CoreGraphics::ResourceTableId resourceTable;
    };
    
    typedef Ids::IdAllocator<
        Volume
    > DDGIVolumeAllocator;
    static DDGIVolumeAllocator ddgiVolumeAllocator;
    

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace GI
