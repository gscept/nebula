#pragma once
//------------------------------------------------------------------------------
/**
    SSR post effect

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
namespace PostEffects
{

class SSRContext : public Graphics::GraphicsContext
{
    _DeclarePluginContext();
public:
    /// constructor
    SSRContext();
    /// destructor
    virtual ~SSRContext();


    /// create context
    static void Create();
    /// discard context
    static void Discard();

    /// setup bloom context
    static void Setup(const Ptr<Frame::FrameScript>& script);

    /// update view resources
    static void UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

private:

    /// implement an empty alloc
    static Graphics::ContextEntityId Alloc() { return Graphics::InvalidContextEntityId; }
    /// implement a dummy dealloc
    static void Dealloc(Graphics::ContextEntityId id) {};

};

} // namespace PostEffects
