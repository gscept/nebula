//------------------------------------------------------------------------------
//  camera.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "camera.h"
#include "visibility/visibilitycontext.h"
#include "graphics/graphicsserver.h"

#include "graphics/cameracontext.h"
#include "io/ioserver.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "input/mouse.h"

#include "audio/audiodevice.h"

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
Camera::AttachToView(const Ptr<Graphics::View>& view)
{
	view->SetCamera(this->cameraEntityId);
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::Setup(SizeT screenWidth, SizeT screenHeight)
{
	this->cameraEntityId = Graphics::CreateEntity();
	CameraContext::RegisterEntity(this->cameraEntityId);

    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;

	CameraContext::SetupProjectionFov(this->cameraEntityId, screenWidth / (float)screenHeight, Math::deg2rad(this->fov), 0.01f, 1000.0f);

    this->defaultViewPoint = Math::vec3(15.0f, 15.0f, -15.0f);

	this->Reset();
	CameraContext::SetView(this->cameraEntityId, this->mayaCameraUtil.GetCameraTransform());

    CameraContext::SetLODCamera(this->cameraEntityId);
    ObserverContext::RegisterEntity(this->cameraEntityId);
	ObserverContext::Setup(this->cameraEntityId, VisibilityEntityType::Camera);
}

//------------------------------------------------------------------------------
/**
*/
void
Camera::Update()
{
    auto& io = ImGui::GetIO();

    this->mayaCameraUtil.SetOrbitButton(io.MouseDown[Input::MouseButton::LeftButton]);
    this->mayaCameraUtil.SetPanButton(io.MouseDown[Input::MouseButton::MiddleButton]);
    this->mayaCameraUtil.SetZoomButton(io.MouseDown[Input::MouseButton::RightButton]);
    this->mayaCameraUtil.SetZoomInButton(io.MouseWheel > 0);
    this->mayaCameraUtil.SetZoomOutButton(io.MouseWheel < 0);
    this->mayaCameraUtil.SetMouseMovement({ -io.MouseDelta.x, -io.MouseDelta.y });
	this->mayaCameraUtil.Update();

	this->freeCamUtil.SetForwardsKey(io.KeysDown[Input::Key::W]);
	this->freeCamUtil.SetBackwardsKey(io.KeysDown[Input::Key::S]);
	this->freeCamUtil.SetRightStrafeKey(io.KeysDown[Input::Key::D]);
	this->freeCamUtil.SetLeftStrafeKey(io.KeysDown[Input::Key::A]);
	this->freeCamUtil.SetUpKey(io.KeysDown[Input::Key::Q]);
	this->freeCamUtil.SetDownKey(io.KeysDown[Input::Key::E]);

	this->freeCamUtil.SetMouseMovement({ -io.MouseDelta.x, -io.MouseDelta.y });
	this->freeCamUtil.SetAccelerateButton(io.KeyShift);

	this->freeCamUtil.SetRotateButton(io.MouseDown[Input::MouseButton::LeftButton]);
	this->freeCamUtil.Update();

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
	this->freeCamUtil.Update();
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
        CameraContext::SetupProjectionFov(this->cameraEntityId, this->screenWidth / (float)this->screenHeight, Math::deg2rad(this->fov), 0.01f, 1000.0f);
        break;
    case Editor::Camera::ORTHOGRAPHIC:
        CameraContext::SetupOrthographic(this->cameraEntityId, this->orthoWidth, this->orthoHeight, 0.1, 1000);
        break;
    default:
        break;
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


} // namespace Editor
