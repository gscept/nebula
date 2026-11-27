//------------------------------------------------------------------------------
//  @file rendersettings.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "graphicsfeature/editor/rendersettings.h"

#if WITH_NEBULA_EDITOR
#include "graphicsfeature/editor/graphicseditor.h"
#include "graphicsfeature/editor/assets/assetpreview.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "editorassets/assetfield.h"
#include "neweditor/api/editorfeature.h"
#include "graphics/cameracontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "coregraphics/shader.h"
#include "coregraphics/commandbuffer.h"
#include "resources/resourceserver.h"
#include "renderutil/drawfullscreenquad.h"
#include "materials/material.h"
#include "frame/default.h"
#include "frame/preview.h"
#include "gpulang/render/system_shaders/editor/editor_wireframe.h"
#include "imgui.h"

namespace GraphicsFeature
{

namespace
{

enum Shading
{
    ShadingLit = 0,
    ShadingWireframe,
    ShadingLitWireframe,
};

struct RenderSettings
{
    int shading = ShadingLit;
    bool depthOfField = false;
    Graphics::DepthOfField dof;
    /// the aperture texture, folder:file:tex
    IO::Path aperture;
} settings;

/// colors are linear, the wireframe is drawn after tonemapping into an sRGB target
constexpr float WireframeBackgroundColor[4] = { 0.025f, 0.025f, 0.03f, 1.0f };
constexpr float WireframeLineColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
constexpr float LitWireframeLineColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

CoreGraphics::ShaderProgramId WireframeBackgroundProgram = CoreGraphics::InvalidShaderProgramId;
CoreGraphics::ShaderProgramId WireframeStaticProgram = CoreGraphics::InvalidShaderProgramId;

//------------------------------------------------------------------------------
/**
    Runs in the Wireframe subgraph of the game's frame script, which draws after tonemapping. The
    meshes are drawn with the Wireframe batch of their material template. Only the viewport's
    Rendering view draws it, the game's own frames never do
*/
void
DrawWireframe(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
{
    if (WireframeStaticProgram == CoreGraphics::InvalidShaderProgramId)
        return;

    // the viewport's Rendering view and the asset preview each have their own shading. The game's
    // own frames are always lit
    const Graphics::ViewId view = Graphics::GraphicsServer::Instance()->GetCurrentView();
    int shading = ShadingLit;
    if (view == GraphicsFeatureUnit::Instance()->GetDefaultView() && GraphicsViewportPanel::ShowsFullPipeline())
        shading = settings.shading;
    else if (AssetPreview* preview = AssetPreview::Peek(); preview != nullptr && view == preview->GetView())
        shading = preview->GetShading();
    if (shading == ShadingLit)
        return;

    // the programs of the wireframe shader share one pipeline layout, so the color pushed here stays
    // valid while the batch switches between them
    EditorWireframe::WireframeConstants constants;
    if (shading == ShadingWireframe)
    {
        // hide the shaded scene, fill with the background color and reset depth to the far plane
        CoreGraphics::CmdSetShaderProgram(cmdBuf, WireframeBackgroundProgram, queue);
        memcpy(constants.Color, WireframeBackgroundColor, sizeof(constants.Color));
        CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, 0, sizeof(constants), &constants);
        RenderUtil::DrawFullScreenQuad::ApplyMesh(cmdBuf);
        CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);
        CoreGraphics::CmdDraw(cmdBuf, RenderUtil::DrawFullScreenQuad::GetPrimitiveGroup());
        memcpy(constants.Color, WireframeLineColor, sizeof(constants.Color));
    }
    else
    {
        // the wireframe pipeline layout is bound before the line color is pushed
        CoreGraphics::CmdSetShaderProgram(cmdBuf, WireframeStaticProgram, queue);
        memcpy(constants.Color, LitWireframeLineColor, sizeof(constants.Color));
    }
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, 0, sizeof(constants), &constants);
    Frame::DrawBatch(cmdBuf, MaterialTemplatesGPULang::BatchGroup::Wireframe, Graphics::ViewGetCamera(view), bufferIndex);
}

//------------------------------------------------------------------------------
/**
    The aperture texture is loaded for the camera, the one it had is let go
*/
void
SetAperture(const IO::Path& texture)
{
    if (settings.dof.dirtMask != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(settings.dof.dirtMask);
        settings.dof.dirtMask = Resources::InvalidResourceId;
    }
    settings.aperture = texture;
    if (texture.IsFile())
        settings.dof.dirtMask = Resources::CreateResource(texture, "editor", nullptr, nullptr, { .immediate = true });
}

//------------------------------------------------------------------------------
/**
    The render side's part of the viewport's View popup
*/
void
DrawViewportSettings(const char* viewMode)
{
    const bool full = Util::String(viewMode) == "rendering";

    ImGui::SeparatorText("Shading");
    ImGui::BeginDisabled(!full);
    ImGui::RadioButton("Lit", &settings.shading, ShadingLit);
    ImGui::SameLine();
    ImGui::RadioButton("Wireframe", &settings.shading, ShadingWireframe);
    ImGui::SameLine();
    ImGui::RadioButton("Lit wireframe", &settings.shading, ShadingLitWireframe);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Camera");
    ImGui::BeginDisabled(!full);
    ImGui::Checkbox("Depth of field", &settings.depthOfField);
    if (settings.depthOfField)
    {
        Graphics::DepthOfField& dof = settings.dof;
        ImGui::SetNextItemWidth(180.0f);
        ImGui::SliderFloat("Focus distance", &dof.focusDistance, 0.01f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::SetNextItemWidth(180.0f);
        ImGui::SliderFloat("Focus range", &dof.focusRange, 0.01f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::SetNextItemWidth(180.0f);
        ImGui::SliderFloat("Max radius", &dof.maxRadius, 0.0f, 30.0f, "%.1f px");
        int blades = (int)dof.blades;
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderInt("Blades", &blades, 0, 8))
            dof.blades = (float)blades;
        ImGui::SetItemTooltip("3 to 8 is a regular iris, anything else a disk");
        float rotation = Math::rad2deg(dof.rotation);
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderFloat("Rotation", &rotation, -180.0f, 180.0f, "%.1f deg"))
            dof.rotation = Math::deg2rad(rotation);
        ImGui::SetNextItemWidth(180.0f);
        ImGui::SliderFloat("Roundness", &dof.roundness, 0.0f, 1.0f, "%.2f");
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Aperture");
        ImGui::SameLine();
        Editor::AssetField("aperture", settings.aperture, "tex", [](const IO::Path& texture) { SetAperture(texture); }, 180.0f);
        if (settings.aperture.IsFile())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear##aperture"))
                SetAperture(IO::Path());
        }
    }
    ImGui::EndDisabled();
    if (!full)
        ImGui::TextDisabled("Shading and camera effects show in the Rendering view");
}

} // namespace

//------------------------------------------------------------------------------
/**
    Off is no blur at any depth
*/
void
ApplyCameraEffects(Graphics::GraphicsEntityId camera)
{
    if (camera == Graphics::InvalidGraphicsEntityId || !Graphics::CameraContext::IsEntityRegistered(camera))
        return;
    Graphics::DepthOfField dof = settings.dof;
    if (!settings.depthOfField)
        dof.maxRadius = 0.0f;
    Graphics::CameraContext::SetDepthOfField(camera, dof);
}

//------------------------------------------------------------------------------
/**
*/
void
RegisterRenderSettings()
{
    Editor::EditorFeatureRegistry::RegisterViewportSettings(Util::Delegate<void(const char*)>::FromFunction<&DrawViewportSettings>());
}

//------------------------------------------------------------------------------
/**
    After the graphics server is up, which the viewport's setup is
*/
void
SetupWireframe()
{
    if (WireframeBackgroundProgram == CoreGraphics::InvalidShaderProgramId)
    {
        CoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:system_shaders/editor/editor_wireframe.gplb");
        if (shader != CoreGraphics::InvalidShaderId)
        {
            WireframeBackgroundProgram = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureMask("Background"));
            WireframeStaticProgram = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureMask("Static"));
        }
    }
    // the pipelines are made when the batch draws, the callback keeps the missing callback warning away
    FrameScript_default::RegisterSubgraph_Wireframe_Render(&DrawWireframe);
    FrameScript_default::RegisterSubgraphPipelines_Wireframe_Render([](const CoreGraphics::RenderPassId pass) {});
    // the asset preview's script has the subgraph too, with the preview's own shading
    FrameScript_preview::RegisterSubgraph_Wireframe_Render(&DrawWireframe);
    FrameScript_preview::RegisterSubgraphPipelines_Wireframe_Render([](const CoreGraphics::RenderPassId pass) {});
}

//------------------------------------------------------------------------------
/**
*/
void
ClearRenderSettings()
{
    settings.shading = ShadingLit;
    SetAperture(IO::Path());
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
