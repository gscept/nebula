#pragma once
//------------------------------------------------------------------------------
/**
    Bloom post effect

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
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
    static void Setup(const Ptr<Frame::FrameScript>& script);

    /// Handle window resize
    static void WindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height);

private:

    /// implement an empty alloc
    static Graphics::ContextEntityId Alloc() { return Graphics::InvalidContextEntityId; }
    /// implement a dummy dealloc
    static void Dealloc(Graphics::ContextEntityId id) {};

};

} // namespace PostEffects
