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



    /// Create volume
    void SetupVolume(const Graphics::GraphicsEntityId id);

private:

    struct Volume
    {
        CoreGraphics::TextureId radiance, normals, depth;
        CoreGraphics::BufferId constants, probeBuffer;

    };
    typedef Ids::IdAllocator<
        Volume
    > DDGIVolumeAllocator;
    static DDGIVolumeAllocator allocator;

    typedef Ids::IdAllocator<
        Graphics::GraphicsEntityId
    > ContributorAllocator;
    static ContributorAllocator contributorAllocator;
};

} // namespace GI
