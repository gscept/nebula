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

#include "frame/default.h"
#include "frame/preview.h"

namespace Presentation
{

namespace Modules
{

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
    texInfo.format = CoreGraphics::PixelFormat::SRGBA8;
    texInfo.width = 1280;
    texInfo.height = 900;
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

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("    Frame    "))
        {
            if (ImGui::MenuItem("Textured + Lit", NULL, this->renderMode == TexturedLit))
            {
                this->renderMode = TexturedLit;
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
            Math::mat4 invViewTranslation = Math::translation(invView.position);
            Math::mat4 scaling = Math::scaling(0.15f);
            Math::mat4 offset = Math::translation(-invView.z_axis * 0.5f);
            Im3d::Im3dContext::DrawAxes(invViewTranslation * offset * scaling);
        }

        const Math::vec3 cameraPos = Math::inverse(this->camera.GetViewTransform()).position;
        Util::String cameraPosStr = Util::String::Sprintf("Camera Position x: %.3f, y: %.3f, z: %.3f", cameraPos.x, cameraPos.y, cameraPos.z);
        Util::String fps = Util::String::Sprintf("FPS: %.2f, ms: %.1f", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime * 1000);

        ImVec2 fpsTextSize = ImGui::CalcTextSize(fps.AsCharPtr());
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - fpsTextSize.x - 15, 0));
        ImGui::Text(fps.AsCharPtr());

        ImVec2 textSize = ImGui::CalcTextSize(cameraPosStr.AsCharPtr());
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - textSize.x - fpsTextSize.x - 100, 0));
        ImGui::Text(cameraPosStr.AsCharPtr());

        ImGui::EndMenuBar();
    }

    CoreGraphics::TextureId textureId = this->targetTexture == CoreGraphics::InvalidTextureId ? FrameScript_default::Texture_SceneBuffer() : this->targetTexture;
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(textureId);

    using namespace CoreGraphics;

    this->textureInfo.nebulaHandle = textureId;
    this->textureInfo.mip = 0;
    this->textureInfo.layer = 0;

    ImVec2 space = ImGui::GetContentRegionAvail();
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
            texInfo.format = CoreGraphics::PixelFormat::SRGBA8;
            texInfo.width = space.x;
            texInfo.height = space.y;
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
    ImGui::Image((void*)&this->textureInfo, imageSize, ImVec2(0, 0), uv);

    ImVec2 elementPos = ImGui::GetItemRectMin();
    ImVec2 imagePosition = { cursorPos.x + localWindowPos.x, cursorPos.y + localWindowPos.y };
    
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
