#pragma once
//------------------------------------------------------------------------------
/**
    Bloom post effect

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
namespace PostEffects
{

class BloomContext : public Graphics::GraphicsContext
{
    _DeclarePluginContext();
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

private:

    /// implement an empty alloc
    static Graphics::ContextEntityId Alloc() { return Graphics::ContextEntityId::Invalid(); }
    /// implement a dummy dealloc
    static void Dealloc(Graphics::ContextEntityId id) {};

};

} // namespace PostEffects
