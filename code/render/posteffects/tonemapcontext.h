#pragma once
//------------------------------------------------------------------------------
/**
    Tonemapping post effect

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "debug/framescriptinspector.h"
namespace PostEffects
{

class TonemapContext : public Graphics::GraphicsContext
{
    _DeclarePluginContext();
public:
    /// constructor
    TonemapContext();
    /// destructor
    virtual ~TonemapContext();


    /// create context
    static void Create();
    /// discard context
    static void Discard();

    /// setup bloom context
    static void Setup(const Ptr<Frame::FrameScript>& script);

private:

    /// implement an empty alloc
    static Graphics::ContextEntityId Alloc() { return Graphics::InvalidContextEntityId; }
    /// implement a dummy dealloc
    static void Dealloc(Graphics::ContextEntityId id) {};

};

} // namespace PostEffects
