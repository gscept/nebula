//------------------------------------------------------------------------------
//  viewport.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "viewport.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/stage.h"
#include "imgui.h"
#include "dynui/imguicontext.h"

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
Viewport::Init(Util::String const & viewName)
{
    static int unique = 0;
    Util::String name = viewName;
    name.AppendInt(unique++);
    this->view = Graphics::GraphicsServer::Instance()->CreateView(name, "frame:vkdefault.json");
    
    this->camera.Setup(1280, 900);
	this->camera.AttachToView(this->view);
	this->camera.Update();
}

//------------------------------------------------------------------------------
/**
*/
void
Viewport::Init(Ptr<Graphics::View> const& view)
{
    this->view = view;
    this->camera.Setup(1280, 900);
	this->camera.AttachToView(this->view);
	this->camera.Update();
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
Viewport::Render()
{
    Ptr<Frame::FrameServer> frameServer = Frame::FrameServer::Instance();
    Ptr<Frame::FrameScript> frame = this->view->GetFrameScript();

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Camera"))
        {
            if (ImGui::MenuItem("Perspective", (const char*)0, this->camera.GetProjectionMode() == Editor::Camera::ProjectionMode::PERSPECTIVE))
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
            if (ImGui::MenuItem("Orthographic", (const char*)0, this->camera.GetProjectionMode() == Editor::Camera::ProjectionMode::ORTHOGRAPHIC))
            {
                this->camera.SetProjectionMode(Editor::Camera::ProjectionMode::ORTHOGRAPHIC);
            }
            if (ImGui::BeginMenu("Orthographic settings"))
            {
                static const float min = 1.0f;
                static const float max = 100.0f;
                if (ImGui::SliderScalar("Width", ImGuiDataType_Float, &this->camera.orthoWidth, &min, &max, "%.3f", ImGuiSliderFlags_None))
                {
                    this->camera.SetProjectionMode(Editor::Camera::ProjectionMode::ORTHOGRAPHIC);
                }
                if (ImGui::SliderScalar("Height", ImGuiDataType_Float, &this->camera.orthoHeight, &min, &max, "%.3f", ImGuiSliderFlags_None))
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
            if (ImGui::MenuItem("Free Camera", (const char*)0, this->camera.GetCameraMode() == Editor::Camera::CameraMode::FREECAM))
            {
                this->camera.SetCameraMode(Editor::Camera::CameraMode::FREECAM);
            }
            ImGui::Separator();
            // TODO: These should center either around 0,0 or around the currently selected object(s).
            if (ImGui::MenuItem("Top")){}
            if (ImGui::MenuItem("Bottom")) {}
            if (ImGui::MenuItem("Left")) {}
            if (ImGui::MenuItem("Right")) {}
            if (ImGui::MenuItem("Front")) {}
            if (ImGui::MenuItem("Back")) {}

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Frame"))
        {
            if (ImGui::MenuItem("Textured + Lit", NULL, this->renderMode == TexturedLit))
            {
                this->renderMode = TexturedLit;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    CoreGraphics::TextureId textureId = frame->GetTexture(this->frameBuffer);
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(textureId);

    using namespace CoreGraphics;

    // Needs to not be nuked scope since we're sending a void*
    static CoreGraphics::TextureId id;
    id = textureId;

    static Dynui::ImguiTextureId textureInfo;
    textureInfo.nebulaHandle = id.HashCode64();
    textureInfo.mip = 0;
    textureInfo.layer = 0;

    ImVec2 imageSize = {(float)dims.width, (float)dims.height};
    imageSize.x = ImGui::GetWindowContentRegionWidth();
    float ratio = (float)dims.height / (float)dims.width;
    imageSize.y = imageSize.x * ratio;

    //auto windowSize = ImGui::GetWindowSize();
    //windowSize.y -= ImGui::GetCursorPosY() - 20;
    ImGui::ImageButton((void*)& textureInfo, imageSize, ImVec2(0,0), ImVec2(1,1), 0);
    
    this->hovered = ImGui::IsItemHovered();
}

//------------------------------------------------------------------------------
/**
*/
void
Viewport::SetStage(Ptr<Graphics::Stage> const & stage)
{
    this->stage = stage;
    this->view->SetStage(stage);
}

} // namespace Modules

} // namespace Presentation
