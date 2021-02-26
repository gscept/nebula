#pragma once
//------------------------------------------------------------------------------
/**
    Histogram calculates a HDR luminance histogram over the entire image 
    to account for light intensity and color saturation.

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
namespace PostEffects 
{

class HistogramContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    HistogramContext();
    /// destructor
    virtual ~HistogramContext();

    /// create context
    static void Create();
    /// discard context
    static void Discard();

    /// set histogram window in relative coordinates and mip of source texture
    static void SetWindow(const Math::float2 offset, Math::float2 size, int mip);
    
    /// setup bloom context
    static void Setup(const Ptr<Frame::FrameScript>& script);

    /// update view resources
    static void UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

private:

    /// implement an empty alloc
    static Graphics::ContextEntityId Alloc() { return Graphics::InvalidContextEntityId; }
    /// implement a dummy dealloc
    static void Dealloc(Graphics::ContextEntityId id) {};

};

} // namespace PostEffects
