#pragma once
//------------------------------------------------------------------------------
/**
    Full-resolution circle of confusion, half-resolution bokeh gather, before tonemap.

    DofMask runs on the compute queue and writes the context-owned BokehMask from Aperture.
    DofSetup, DofGather, and DofComposite run on the graphics queue.
    DofComposite writes the mix into LightBuffer. Color Downsample and bloom run on that.
    finalize.gpul adds bloom.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/texture.h"
namespace PostEffects
{

class DofContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    DofContext();
    /// destructor
    virtual ~DofContext();

    /// create context
    static void Create();
    /// discard context
    static void Discard();

    /// setup dof context
    static void Setup();

    /// Aperture.x blade count, y rotation in radians, z roundness in [0, 1].
    /// Blade count 3..8 is a regular iris. Any other count is a disk.
    /// Roundness 0 is straight blades, 1 is a disk.
    /// w is 0 while the dirt mask is the white placeholder.
    /// The mask is rebuilt on the next DofMask dispatch.
    static void SetBokeh(float blades, float rotation, float roundness);

    /// Aperture intensity for every bokeh disk. r is the weight. White leaves the gather unchanged.
    /// The initial texture is CoreGraphics::White2D.
    static void SetDirtMask(const CoreGraphics::TextureId texture);

    /// Handle window resize
    static void Resize(const uint framescriptHash, SizeT width, SizeT height);

private:

    /// implement an empty alloc
    static Graphics::ContextEntityId Alloc() { return Graphics::InvalidContextEntityId; }
    /// implement a dummy dealloc
    static void Dealloc(Graphics::ContextEntityId id) {};

};

} // namespace PostEffects
