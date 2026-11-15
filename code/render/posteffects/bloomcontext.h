#pragma once
//------------------------------------------------------------------------------
/**
    Bloom post effect

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/texture.h"
namespace PostEffects
{

class BloomContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    BloomContext();
    /// destructor
    virtual ~BloomContext();

    /// create context
    static void Create();
    /// discard context
    static void Discard();

    /// setup bloom context
    static void Setup();

    /// Screen-space lens dirt multiplied onto the bloom. White leaves it unchanged.
    /// DirtMask is a texture2D. The initial texture is CoreGraphics::White2D.
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
