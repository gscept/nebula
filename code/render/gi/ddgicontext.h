#pragma once
//------------------------------------------------------------------------------
/**
    The DDGI context is responsible for managing the GI volumes used to apply
    indirect light in the scene. 

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <gi/shaders/probe_finalize.h>
#include <gi/shaders/probe_update.h>

#include "graphics/graphicscontext.h"
namespace GI
{

union DDGIOptions
{
    struct
    {
        uint scrolling : 1;             // Infinitely scrolls based on camera position
        uint classify : 1;              // Enables/disables probes based on hits
        uint relocate : 1;              // Relocate probes to avoid them being stuck inside geometry
        uint partialUpdate : 1;         // Update probes using a round-robin method
        uint lowPrecisionTextures : 1;  // Use more compact texture formats at the expense of quality
    } flags;
    uint32 bits = 0x0;
};

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

        float normalBias = 0.1f;
        float viewBias = 0.4f;
        float irradianceScale = 1.0f;
        float distanceExponent = 0.5f;
        float encodingGamma = 5.0f;
        float changeThreshold = 0.2f;
        float brightnessThreshold = 2.0f;
        float hysteresis = 0.97f;
        float blendCutoff = 0.0f;
        float blend = 0.0f;

        DDGIOptions options;
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
        CoreGraphics::TextureId radiance; // Ray tracing output
        CoreGraphics::TextureId irradiance, distance, offsets, states, scrollSpace;
        CoreGraphics::BufferId volumeUpdateBuffer, volumeBlendBuffer;
        CoreGraphics::ResourceTableId updateProbesTable, blendProbesTable;

        float normalBias = 0.1f;
        float viewBias = 0.4f;
        float irradianceScale = 1.0f;
        float distanceExponent = 50.0f;
        float encodingGamma = 5.0f;
        float changeThreshold = 0.2f;
        float brightnessThreshold = 2.0f;
        float hysteresis = 0.97f;
        float blendCutoff = 0.0f;
        float blend = 0.0f;
        
        ProbeUpdate::VolumeConstants updateConstants;
        ProbeFinalize::BlendConstants blendConstants;
        DDGIOptions options;

#ifndef PUBLIC_BUILD
        CoreGraphics::ResourceTableId debugResourceTable;
#endif

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
