#pragma once
//------------------------------------------------------------------------------
/**
    @class Graphics::EnvironmentContext

    The environment context deals with anything related to the sky and atmosphere effects

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscontext.h"
namespace Graphics
{

class EnvironmentContext : public GraphicsContext
{
    __DeclarePluginContext();
public:

    /// create context
    static void Create(const Graphics::GraphicsEntityId sun);
    /// update shader server tick params per frame
    static void OnBeforeFrame(const Graphics::FrameContext& ctx);
    /// render IMGUI
    static void RenderUI(const Graphics::FrameContext& ctx);

    /// set the fog color
    static void SetFogColor(const Math::vec4& fogColor);
    /// set the fog distances
    static void SetFogDistances(const float nearFog, const float farFog);
    /// set the bloom color filter
    static void SetBloomColor(const Math::vec4& bloomColor);
    /// set the level at which pixels are considered 'bright' and should be bloomed
    static void SetBloomThreshold(const float threshold);
    /// set the maximum allowed luminance by the eye adaptation
    static void SetMaxLuminance(const float maxLuminance);
    /// set the number of global environment mips
    static void SetNumEnvironmentMips(const int mips);
};

} // namespace Graphics
