//------------------------------------------------------------------------------
//  camera.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "camera.h"
#include "visibility/visibilitycontext.h"
#include "graphics/graphicsserver.h"
#include "dynui/imguicontext.h"
#include "graphics/cameracontext.h"
#include "io/ioserver.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "input/mouse.h"

#include "audio/audiodevice.h"
#include "basegamefeature/managers/timemanager.h"
#include "editor/editor.h"

#include "imgui.h"

using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
Camera::Camera()
{
}

//------------------------------------------------------------------------------
/**
*/
Camera::~Camera()
{
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::AttachToView(const Graphics::ViewId view)
{
    ViewSetCamera(view, this->cameraEntityId);
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::Setup(SizeT screenWidth, SizeT screenHeight, Graphics::StageMask stageMask)
{
	this->cameraEntityId = Graphics::CreateEntity();
	CameraContext::RegisterEntity(this->cameraEntityId);

    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;
    this->stageMask = stageMask;

	CameraContext::SetupProjectionFov(this->cameraEntityId, screenWidth / (float)screenHeight, Math::deg2rad(this->fov), 0.01f, 1000.0f, stageMask);

    this->defaultViewPoint = Math::vec3(15.0f, 15.0f, -15.0f);

	this->Reset();
	CameraContext::SetView(this->cameraEntityId, this->mayaCameraUtil.GetCameraTransform());

    CameraContext::AddLODCamera(this->cameraEntityId);
    ObserverContext::RegisterEntity(this->cameraEntityId);
	ObserverContext::Setup(this->cameraEntityId, VisibilityEntityType::Camera, stageMask);
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::Update()
{
    auto& io = ImGui::GetIO();

    Game::TimeSource* timeSource = Game::Time::GetTimeSource(TIMESOURCE_EDITOR);

    this->mayaCameraUtil.SetOrbitButton(io.MouseDown[ImGuiMouseButton_Left]);
    this->mayaCameraUtil.SetPanButton(io.MouseDown[ImGuiMouseButton_Middle]);
    this->mayaCameraUtil.SetZoomButton(io.MouseDown[ImGuiMouseButton_Right]);
    this->mayaCameraUtil.SetZoomInButton(io.MouseWheel > 0);
    this->mayaCameraUtil.SetZoomOutButton(io.MouseWheel < 0);
    this->mayaCameraUtil.SetMouseMovement({ -io.MouseDelta.x, -io.MouseDelta.y });
    this->mayaCameraUtil.Update(timeSource->frameTime);

	this->freeCamUtil.SetForwardsKey(ImGui::IsKeyPressed(ImGuiKey_W));
	this->freeCamUtil.SetBackwardsKey(ImGui::IsKeyPressed(ImGuiKey_S));
	this->freeCamUtil.SetRightStrafeKey(ImGui::IsKeyPressed(ImGuiKey_D));
	this->freeCamUtil.SetLeftStrafeKey(ImGui::IsKeyPressed(ImGuiKey_A));
	this->freeCamUtil.SetUpKey(ImGui::IsKeyPressed(ImGuiKey_Q));
	this->freeCamUtil.SetDownKey(ImGui::IsKeyPressed(ImGuiKey_E));

	this->freeCamUtil.SetMouseMovement({ -io.MouseDelta.x, -io.MouseDelta.y });
	this->freeCamUtil.SetAccelerateButton(ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift));

	this->freeCamUtil.SetRotateButton(io.MouseDown[Input::MouseButton::RightButton]);
	this->freeCamUtil.Update(timeSource->frameTime);

	switch (this->cameraMode)
	{
	case 0:
		CameraContext::SetView(this->cameraEntityId, Math::inverse(this->mayaCameraUtil.GetCameraTransform()));
        if (Audio::AudioDevice::HasInstance())
            Audio::AudioDevice::Instance()->SetListenerTransform(Math::inverse(this->mayaCameraUtil.GetCameraTransform()));
		break;
	case 1:
		CameraContext::SetView(this->cameraEntityId, Math::inverse(this->freeCamUtil.GetTransform()));
        if (Audio::AudioDevice::HasInstance())
            Audio::AudioDevice::Instance()->SetListenerTransform(Math::inverse(this->freeCamUtil.GetTransform()));
		break;
	default:
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::Reset()
{
    this->freeCamUtil.Setup(this->defaultViewPoint, Math::normalize(this->defaultViewPoint));
	this->mayaCameraUtil.Setup(Math::point(0.0f, 0.0f, 0.0f), this->defaultViewPoint, Math::vector(0.0f, 1.0f, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
bool const
Camera::GetCameraMode() const
{
    return this->cameraMode;
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::SetCameraMode(CameraMode mode)
{
    this->cameraMode = mode;
}

//------------------------------------------------------------------------------
/**
*/
bool const
Camera::GetProjectionMode() const
{
    return this->projectionMode;
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::SetProjectionMode(ProjectionMode mode)
{
    this->projectionMode = mode;
    switch (mode)
    {
    case Editor::Camera::PERSPECTIVE:
        CameraContext::SetupProjectionFov(this->cameraEntityId, this->screenWidth / (float)this->screenHeight, Math::deg2rad(this->fov), 0.01f, 1000.0f, this->stageMask);
        break;
    case Editor::Camera::ORTHOGRAPHIC:
        CameraContext::SetupOrthographic(this->cameraEntityId, this->orthoWidth, this->orthoHeight, 0.1, 1000, this->stageMask);
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
Camera::SetViewDimensions(SizeT screenWidth, SizeT screenHeight)
{
    if (this->screenWidth != screenWidth || this->screenHeight != screenHeight)
    {
        this->screenWidth = screenWidth;
        this->screenHeight = screenHeight;
        this->SetProjectionMode(this->projectionMode);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::SetTransform(Math::mat4 const& val)
{
    CameraContext::SetView(this->cameraEntityId, val);
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::SetTargetPosition(Math::vec3 const& point)
{
    this->freeCamUtil.SetTargetPosition(point);
    //this->mayaCameraUtil.SetTargetPosition(point);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
Camera::GetViewTransform() const
{
    return CameraContext::GetView(this->cameraEntityId);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
Camera::GetProjectionTransform() const
{
    return CameraContext::GetProjection(this->cameraEntityId);
}

//------------------------------------------------------------------------------
/**
*/
const Graphics::CameraSettings& 
Camera::GetCameraSettings() const
{
    return CameraContext::GetSettings(this->cameraEntityId);
}


} // namespace Editor
