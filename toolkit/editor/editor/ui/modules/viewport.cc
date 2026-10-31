//------------------------------------------------------------------------------
//  viewport.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "viewport.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "imgui.h"
#include "editor/editor/tools/selectioncontext.h"
#include "lighting/lightcontext.h"
#include "im3d/im3dcontext.h"
#include "graphics/cameracontext.h"
#include "coregraphics/texture.h"
#include "resources/resourceserver.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/assetbrowser.h"

#include "renderutil/drawfullscreenquad.h"

#include "frame/default.h"
#include "frame/preview.h"
#include "gpulang/render/system_shaders/editor/editor_wireframe.h"

namespace Presentation
{

namespace Modules
{

namespace
{

/// viewports that have been initialized, used to find the render mode of the view being rendered
Util::Array<Viewport*> ActiveViewports;

/// colors are linear, both frame scripts draw the wireframe after tonemapping into an sRGB target
constexpr float WireframeBackgroundColor[4] = { 0.025f, 0.025f, 0.03f, 1.0f };
constexpr float WireframeLineColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
constexpr float LitWireframeLineColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

CoreGraphics::ShaderProgramId WireframeBackgroundProgram = CoreGraphics::InvalidShaderProgramId;
CoreGraphics::ShaderProgramId WireframeStaticProgram = CoreGraphics::InvalidShaderProgramId;

//------------------------------------------------------------------------------
/**
    Runs in the "Wireframe" subgraph of the default and preview frame scripts.
    The meshes are drawn with the Wireframe batch of their material template.
*/
void
DrawWireframe(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
{
    const Graphics::ViewId view = Graphics::GraphicsServer::Instance()->GetCurrentView();
    const Viewport::RenderMode mode = Viewport::GetRenderMode(view);
    if (mode != Viewport::Wireframe && mode != Viewport::LitWireframe)
        return;

    // All programs of the wireframe shader share one pipeline layout,
    // so the color pushed here stays valid while the batch switches between them
    EditorWireframe::WireframeConstants constants;
    if (mode == Viewport::Wireframe)
    {
        // Hide the shaded scene, fill with the background color and reset depth to the far plane
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
        // Bind the wireframe pipeline layout before pushing the line color
        CoreGraphics::CmdSetShaderProgram(cmdBuf, WireframeStaticProgram, queue);
        memcpy(constants.Color, LitWireframeLineColor, sizeof(constants.Color));
    }
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, 0, sizeof(constants), &constants);
    Frame::DrawBatch(cmdBuf, MaterialTemplatesGPULang::BatchGroup::Wireframe, Graphics::ViewGetCamera(view), bufferIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
SetupWireframeRendering()
{
    if (WireframeBackgroundProgram != CoreGraphics::InvalidShaderProgramId)
        return;

    CoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:system_shaders/editor/editor_wireframe.gplb");
    WireframeBackgroundProgram = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureMask("Background"));
    WireframeStaticProgram = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureMask("Static"));

    // Pipelines are created on demand when drawing, but registering the callback avoids the missing callback warning
    auto noPipelines = [](const CoreGraphics::RenderPassId pass) {};
    FrameScript_default::RegisterSubgraph_Wireframe_Render(DrawWireframe);
    FrameScript_default::RegisterSubgraphPipelines_Wireframe_Render(noPipelines);
    FrameScript_preview::RegisterSubgraph_Wireframe_Render(DrawWireframe);
    FrameScript_preview::RegisterSubgraphPipelines_Wireframe_Render(noPipelines);
}

} // namespace

//------------------------------------------------------------------------------
/**
*/
Viewport::Viewport()
{

}

//------------------------------------------------------------------------------
/**
*/
Viewport::~Viewport()
{
    IndexT index = ActiveViewports.FindIndex(this);
    if (index != InvalidIndex)
        ActiveViewports.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
Viewport::RenderMode
Viewport::GetRenderMode(const Graphics::ViewId view)
{
    for (const Viewport* viewport : ActiveViewports)
    {
        if (viewport->view == view)
            return viewport->renderMode;
    }
    return TexturedLit;
}

//------------------------------------------------------------------------------
/**
*/
void
Viewport::Init(Util::String const & viewName, const Graphics::StageMask mask)
{
    static int unique = 0;
    Util::String name = viewName;
    name.AppendInt(unique++);
    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "Viewport Target"_atm;
    texInfo.format = CoreGraphics::PixelFormat::SRGBA8;
    texInfo.width = 1280;
    texInfo.height = 900;
    // Copied to by the preview frame script and sampled by ImGui, clear so it starts out in a readable layout
    texInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::TransferDestination;
    texInfo.clear = true;
    this->targetTexture = CoreGraphics::CreateTexture(texInfo);
    this->directionalLight = Graphics::GraphicsServer::Instance()->CreateGraphicsEntity();
    this->view = Graphics::GraphicsServer::Instance()->CreateView(name, FrameScript_preview::Run, Math::rectangle<int>(0, 0, 1280, 900), mask, [this](IndexT frameIndex, IndexT bufferIndex)
    {
        FrameScript_preview::Bind_Target(Frame::TextureImport(this->targetTexture));
    });
    Lighting::LightContext::RegisterEntity(this->directionalLight);
    Lighting::LightContext::SetupDirectionalLight(
        this->directionalLight, 
        {
            .view = this->view,
            .color = Math::vec3(1.0f),
            .intensity = 50.0f,
            .zenith = (float)70_rad,
            .azimuth = (float)0_rad,
            .stageMask = mask,
            .castShadows = false
        }
    );
    
    this->camera.Setup(1280, 900, 1 << 3);
	this->camera.AttachToView(this->view);
	this->camera.Update();
    this->imguiTextureId = Dynui::AllocateImguiTextureId({});

    SetupWireframeRendering();
    if (ActiveViewports.FindIndex(this) == InvalidIndex)
        ActiveViewports.Append(this);
}

//------------------------------------------------------------------------------
/**
*/
void
Viewport::Init(const Graphics::ViewId view)
{
    this->view = view;
    this->camera.Setup(1280, 900);
	this->camera.AttachToView(this->view);
	this->camera.Update();
    this->targetTexture = CoreGraphics::InvalidTextureId;
    this->imguiTextureId = Dynui::AllocateImguiTextureId({});

    SetupWireframeRendering();
    if (ActiveViewports.FindIndex(this) == InvalidIndex)
        ActiveViewports.Append(this);
}

//------------------------------------------------------------------------------
/**
*/
void
Viewport::SetFrameBuffer(Util::String const& name)
{
    this->frameBuffer = name;
}

//------------------------------------------------------------------------------
/**
*/
void
Viewport::Update()
{
    if (this->focused && !Tools::SelectionContext::IsPaused())
    {
        camera.Update();
        this->focused = false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Viewport::Render()
{
    if (ImGui::BeginMenuBar())
    {
        ImVec2 padding = ImGui::GetStyle().WindowPadding;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding + ImVec2(4, 4));

        if (ImGui::BeginMenu("    Camera    "))
        {
            if (ImGui::MenuItem(
                    "Perspective", (const char*)0, this->camera.GetProjectionMode() == Editor::Camera::ProjectionMode::PERSPECTIVE
                ))
            {
                this->camera.SetProjectionMode(Editor::Camera::ProjectionMode::PERSPECTIVE);
            }
            if (ImGui::BeginMenu("Perspective settings"))
            {
                static const float min = 10.0f;
                static const float max = 130.0f;
                if (ImGui::SliderScalar("Field of view", ImGuiDataType_Float, &this->camera.fov, &min, &max))
                {
                    this->camera.SetProjectionMode(Editor::Camera::ProjectionMode::PERSPECTIVE);
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem(
                    "Orthographic",
                    (const char*)0,
                    this->camera.GetProjectionMode() == Editor::Camera::ProjectionMode::ORTHOGRAPHIC
                ))
            {
                this->camera.SetProjectionMode(Editor::Camera::ProjectionMode::ORTHOGRAPHIC);
            }
            if (ImGui::BeginMenu("Orthographic settings"))
            {
                static const float min = 1.0f;
                static const float max = 100.0f;
                if (ImGui::SliderScalar(
                        "Width", ImGuiDataType_Float, &this->camera.orthoWidth, &min, &max, "%.3f", ImGuiSliderFlags_None
                    ))
                {
                    this->camera.SetProjectionMode(Editor::Camera::ProjectionMode::ORTHOGRAPHIC);
                }
                if (ImGui::SliderScalar(
                        "Height", ImGuiDataType_Float, &this->camera.orthoHeight, &min, &max, "%.3f", ImGuiSliderFlags_None
                    ))
                {
                    this->camera.SetProjectionMode(Editor::Camera::ProjectionMode::ORTHOGRAPHIC);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Orbit", (const char*)0, this->camera.GetCameraMode() == Editor::Camera::CameraMode::ORBIT))
            {
                this->camera.SetCameraMode(Editor::Camera::CameraMode::ORBIT);
            }
            if (ImGui::MenuItem(
                    "Free Camera", (const char*)0, this->camera.GetCameraMode() == Editor::Camera::CameraMode::FREECAM
                ))
            {
                this->camera.SetCameraMode(Editor::Camera::CameraMode::FREECAM);
            }
            ImGui::Separator();
            // TODO: These should center either around 0,0 or around the currently selected object(s).
            if (ImGui::MenuItem("Top"))
            {
            }
            if (ImGui::MenuItem("Bottom"))
            {
            }
            if (ImGui::MenuItem("Left"))
            {
            }
            if (ImGui::MenuItem("Right"))
            {
            }
            if (ImGui::MenuItem("Front"))
            {
            }
            if (ImGui::MenuItem("Back"))
            {
            }
            ImGui::Separator();
            Graphics::GraphicsEntityId cameraEntity = this->camera.GetGraphicsEntity();
            Graphics::DepthOfField dof = Graphics::CameraContext::GetDepthOfField(cameraEntity);
            static const float focusMin = 0.01f;
            static const float focusMax = 500.0f;
            static const float rangeMin = 0.01f;
            static const float rangeMax = 200.0f;
            static const float radiusMin = 0.0f;
            static const float radiusMax = 30.0f;
            bool dofChanged = false;
            dofChanged |= ImGui::SliderScalar("Focus distance", ImGuiDataType_Float, &dof.focusDistance, &focusMin, &focusMax, "%.2f");
            dofChanged |= ImGui::SliderScalar("Focus range", ImGuiDataType_Float, &dof.focusRange, &rangeMin, &rangeMax, "%.2f");
            dofChanged |= ImGui::SliderScalar("Max radius", ImGuiDataType_Float, &dof.maxRadius, &radiusMin, &radiusMax, "%.1f");
            int blades = (int)dof.blades;
            static const int bladesMin = 0;
            static const int bladesMax = 8;
            if (ImGui::SliderInt("Blades", &blades, bladesMin, bladesMax))
            {
                dof.blades = (float)blades;
                dofChanged = true;
            }
            float rotationDeg = Math::rad2deg(dof.rotation);
            static const float rotationMin = -180.0f;
            static const float rotationMax = 180.0f;
            if (ImGui::SliderScalar("Rotation", ImGuiDataType_Float, &rotationDeg, &rotationMin, &rotationMax, "%.1f"))
            {
                dof.rotation = Math::deg2rad(rotationDeg);
                dofChanged = true;
            }
            static const float roundMin = 0.0f;
            static const float roundMax = 1.0f;
            dofChanged |= ImGui::SliderScalar("Roundness", ImGuiDataType_Float, &dof.roundness, &roundMin, &roundMax, "%.2f");
            if (dof.dirtMask == Resources::InvalidResourceId)
                ImGui::Text("Dirt mask: none");
            else
                ImGui::Text("Dirt mask: %s", CoreGraphics::TextureGetName(CoreGraphics::TextureId(dof.dirtMask.resource)).Value());
            if (ImGui::Button("Set dirt mask"))
            {
                auto browser = (Presentation::AssetBrowser*)Presentation::AssetBrowserPickerWindow;
                browser->PickFile(IO::Path(), [cameraEntity](const IO::Path& filePath)
                {
                    n_assert(filePath.IsFile());
                    if (filePath.GetType() != "tex")
                    {
                        n_warning("Dirt mask must be a texture\n");
                        return;
                    }
                    Resources::CreateResource(filePath, "editor", [cameraEntity](Resources::ResourceId id)
                    {
                        Graphics::DepthOfField current = Graphics::CameraContext::GetDepthOfField(cameraEntity);
                        if (current.dirtMask != Resources::InvalidResourceId)
                            Resources::DiscardResource(current.dirtMask);
                        current.dirtMask = id;
                        Graphics::CameraContext::SetDepthOfField(cameraEntity, current);
                    }, nullptr);
                });
            }
            if (dof.dirtMask != Resources::InvalidResourceId && ImGui::Button("Clear dirt mask"))
            {
                Resources::DiscardResource(dof.dirtMask);
                dof.dirtMask = Resources::InvalidResourceId;
                dofChanged = true;
            }
            if (dofChanged)
            {
                Graphics::CameraContext::SetDepthOfField(cameraEntity, dof);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("    Frame    "))
        {
            if (ImGui::MenuItem("Textured + Lit", NULL, this->renderMode == TexturedLit))
            {
                this->renderMode = TexturedLit;
            }
            if (ImGui::MenuItem("Wireframe", NULL, this->renderMode == Wireframe))
            {
                this->renderMode = Wireframe;
            }
            if (ImGui::MenuItem("Lit Wireframe", NULL, this->renderMode == LitWireframe))
            {
                this->renderMode = LitWireframe;
            }
            ImGui::EndMenu();
        }

        static bool showWorldAxis = false;
        if (ImGui::BeginMenu("    Debug    "))
        {
            ImGui::Checkbox("   Show World axis   ", &showWorldAxis);
            ImGui::EndMenu();
        }

        if (showWorldAxis)
        {
            Math::mat4 invView = Math::inverse(this->camera.GetViewTransform());
            Math::mat4 invViewTranslation = Math::translation(invView.position());
            Math::mat4 scaling = Math::scaling(0.15f);
            Math::mat4 offset = Math::translation(-invView.z_axis() * 0.5f);
            Im3d::Im3dContext::DrawAxes(invViewTranslation * offset * scaling);
        }

        const Math::vec3 cameraPos = Math::inverse(this->camera.GetViewTransform()).position();
        Util::String cameraPosStr = Util::String::Sprintf("Camera Position x: %.3f, y: %.3f, z: %.3f", cameraPos.x, cameraPos.y, cameraPos.z);
        Util::String fps = Util::String::Sprintf("FPS: %.2f, ms: %.1f", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime * 1000);

        ImVec2 fpsTextSize = ImGui::CalcTextSize(fps.AsCharPtr());
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - fpsTextSize.x - 15, 0));
        ImGui::Text(fps.AsCharPtr());

        ImVec2 textSize = ImGui::CalcTextSize(cameraPosStr.AsCharPtr());
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - textSize.x - fpsTextSize.x - 100, 0));
        ImGui::Text(cameraPosStr.AsCharPtr());

        ImGui::PopStyleVar();
        ImGui::EndMenuBar();
    }

    CoreGraphics::TextureId textureId = this->targetTexture == CoreGraphics::InvalidTextureId ? FrameScript_default::Texture_SceneBuffer() : this->targetTexture;
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(textureId);

    using namespace CoreGraphics;

    // WindowPadding.x insets the menu bar. The image still covers the inner width.
    const float padX = ImGui::GetStyle().WindowPadding.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - padX);
    ImVec2 space = ImGui::GetContentRegionAvail();
    space.x += padX;
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 viewportPos = ImGui::GetWindowViewport()->Pos;
    ImVec2 localWindowPos = ImVec2 {windowPos.x - viewportPos.x, windowPos.y - viewportPos.y};

    if (this->targetTexture != CoreGraphics::InvalidTextureId)
    {
        if (dims.width < space.x || dims.height < space.y)
        {
            CoreGraphics::DestroyTexture(this->targetTexture);
            CoreGraphics::TextureCreateInfo texInfo;
            texInfo.name = "Viewport Target"_atm;
            texInfo.format = CoreGraphics::PixelFormat::SRGBA8;
            texInfo.width = space.x;
            texInfo.height = space.y;
            texInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::TransferDestination;
            texInfo.clear = true;
            this->targetTexture = CoreGraphics::CreateTexture(texInfo);
        }
    }

    ImVec2 imageSize = {(float)space.x, (float)space.y};
    imageSize.x = Math::max(imageSize.x, 1.0f);
    imageSize.y = Math::max(imageSize.y, 1.0f);
    ImVec2 uv = { space.x / dims.width, space.y / dims.height };
    this->camera.SetViewDimensions(imageSize.x, imageSize.y);

    //auto windowSize = ImGui::GetWindowSize();
    //windowSize.y -= ImGui::GetCursorPosY() - 20;
    Dynui::ImguiTextureId textureData;
    textureData.nebulaHandle = textureId;
    textureData.useAlpha = 0;

    Dynui::SetImguiTextureIdData(this->imguiTextureId, textureData);
    ImTextureRef ref {this->imguiTextureId};
    ImVec2 clipMin = ImGui::GetCursorScreenPos();
    clipMin.x = ImGui::GetWindowPos().x;
    ImVec2 clipMax = ImGui::GetWindowPos() + ImGui::GetWindowSize();
    ImGui::PushClipRect(clipMin, clipMax, false);
    ImGui::Image(ref, imageSize, ImVec2(0, 0), uv);
    ImGui::PopClipRect();

    ImVec2 elementPos = ImGui::GetItemRectMin();
    ImVec2 imagePosition = { cursorPos.x + localWindowPos.x, cursorPos.y + localWindowPos.y };
    
    this->viewportScreenPosition = {ImGui::GetWindowViewport()->Pos.x, ImGui::GetWindowViewport()->Pos.y};
    this->lastViewportImagePositionAbsolute = { elementPos.x, elementPos.y };
    this->lastViewportImageSizeAbsolute = { imageSize.x, imageSize.y };
    this->lastViewportImagePosition = { imagePosition.x, imagePosition.y };
    this->lastViewportImageSize = { imageSize.x, imageSize.y };

    ViewSetViewport(this->view, Math::rectangle<int>(0, 0, imageSize.x, imageSize.y));
    this->focused = ImGui::IsItemFocused() || ImGui::IsWindowHovered();


    if (this->focused)
    {
        ImGui::GetIO().WantCaptureKeyboard = false;
        ImGui::GetIO().WantCaptureMouse = false;
    }
    else
    {
        ImGui::GetIO().WantCaptureKeyboard = true;
        ImGui::GetIO().WantCaptureMouse = true;
    }
}


//------------------------------------------------------------------------------
/**
*/
const Graphics::ViewId
Viewport::GetView() const
{
     n_assert(this->view != Graphics::InvalidViewId);
     return this->view;
}

//------------------------------------------------------------------------------
/**
*/
void
Viewport::SetStage(const uint16_t stage)
{
    ViewSetStageMask(this->view, stage);
}

} // namespace Modules

} // namespace Presentation
