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

    struct VolumeSetup
    {
        uint numProbesX, numProbesY, numProbesZ;
        uint numPixelsPerProbe;
        Math::vec3 size;
        Math::vec3 position;
    };

    /// Create volume
    void SetupVolume(const Graphics::GraphicsEntityId id, const VolumeSetup& setup);
    /// Set volume position
    void SetPosition(const Graphics::GraphicsEntityId id, const Math::vec3& position);
    /// Set volume scale
    void SetScale(const Graphics::GraphicsEntityId id, const Math::vec3& scale);

private:

    struct Volume
    {
        uint numProbesX, numProbesY, numProbesZ;
        uint numPixelsPerProbe;
        Math::vec3 size;
        Math::vec3 position;
        CoreGraphics::TextureId radiance, normals, depth;
        CoreGraphics::BufferWithStaging probeBuffer;
        CoreGraphics::BufferId constants;
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
