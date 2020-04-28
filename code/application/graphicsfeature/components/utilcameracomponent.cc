//------------------------------------------------------------------------------
//  utilcameracomponent.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "utilcameracomponent.h"
#include "basegamefeature/managers/componentmanager.h"
#include "game/component/componentserialization.h"
#include "graphicsfeature/components/graphicsdata.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "graphics/cameracontext.h"
#include "graphicsfeature/messages/graphicsprotocol.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "visibility/visibilitycontext.h"
#include "renderutil/mayacamerautil.h"
#include "renderutil/freecamerautil.h"

using namespace Graphics;

namespace GraphicsFeature
{

struct InstanceData
{
    RenderUtil::MayaCameraUtil maya;
    RenderUtil::FreeCameraUtil free;
};
static Util::HashTable<Ids::Id32, InstanceData> instanceData;

static UtilCameraComponentAllocator* component;

__ImplementComponent(GraphicsFeature::UtilCameraComponent, component)

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::Create()
{
	if (component != nullptr)
	{
		component->DestroyAll();
	}
	else
	{
		component = n_new(UtilCameraComponentAllocator());
	}

	component->DestroyAll();

	__SetupDefaultComponentBundle(component);
	component->functions.OnActivate = OnActivate;
	component->functions.OnDeactivate = OnDeactivate;
    component->functions.OnBeginFrame = OnBeginFrame;
	__RegisterComponent(component, "CameraComponent"_atm);

	SetupAcceptedMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::Discard()
{

}

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::SetupAcceptedMessages()
{
    __RegisterMsg(Msg::SetCameraMode, SetMode);
}

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::OnActivate(Game::InstanceId instance)
{
    InstanceData data;
    instanceData.Add(instance, data);
    auto gfxEntity = Graphics::CreateEntity();
    component->Get<Attr::GraphicsEntity>(instance) = gfxEntity.id;
    CameraContext::RegisterEntity(gfxEntity);
    Math::vec4 camProj = component->Get<Attr::CameraProjection>(instance);
    CameraContext::SetupProjectionFov(gfxEntity, camProj.x, camProj.y, camProj.z, camProj.w);
    Visibility::ObserverContext::RegisterEntity(gfxEntity);
    Visibility::ObserverContext::Setup(gfxEntity, Visibility::VisibilityEntityType::Camera);
    GraphicsFeature::GraphicsFeatureUnit::Instance()->defaultView->SetCamera(gfxEntity);
    UpdateCamera(instance);
}

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::OnDeactivate(Game::InstanceId instance)
{
    //FIXME
}

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::SetMode(Game::Entity entity, CameraMode mode)
{
    Game::InstanceId instance = component->GetInstance(entity);
    auto oldMode = component->Get<Attr::CameraMode>(instance);
    if (oldMode == mode) return;

    RenderUtil::MayaCameraUtil & mayaCameraUtil = instanceData[instance].maya;
    RenderUtil::FreeCameraUtil & freeCamUtil = instanceData[instance].free;

    component->Get<Attr::CameraMode>(instance) = mode;
    switch (mode)
    {
    case MayaCamera:
        mayaCameraUtil.Setup(mayaCameraUtil.GetCenterOfInterest(), freeCamUtil.GetTransform().position, Math::vector(0.0f, 1.0f, 0.0f));
        break;
    case FreeCamera:
    {
        Math::point pos = mayaCameraUtil.GetCameraTransform().position;
        freeCamUtil.Setup(pos, Math::normalize(pos - mayaCameraUtil.GetCenterOfInterest()));
    }
    break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::OnBeginFrame()
{
    auto & items = component->data.GetArray<0>();
    for (auto id : items)
    {
        UpdateCamera(component->GetInstance(id));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::UpdateCamera(Game::InstanceId instance)
{
    Graphics::GraphicsEntityId cam = component->Get<Attr::GraphicsEntity>(instance);

    if (cam == Graphics::GraphicsEntityId::Invalid())
    {
        return;
    }

    CameraMode mode = (CameraMode)component->Get<Attr::CameraMode>(instance);

    Ptr<Input::InputServer> inputServer = Input::InputServer::Instance();
    const Ptr<Input::Keyboard>& keyboard = inputServer->GetDefaultKeyboard();
    const Ptr<Input::Mouse>& mouse = inputServer->GetDefaultMouse();
    switch (mode)
    {
    case MayaCamera:
    {
        RenderUtil::MayaCameraUtil & mayaCameraUtil = instanceData[instance].maya;

        mayaCameraUtil.SetOrbitButton(mouse->ButtonPressed(Input::MouseButton::LeftButton));
        mayaCameraUtil.SetPanButton(mouse->ButtonPressed(Input::MouseButton::MiddleButton));
        mayaCameraUtil.SetZoomButton(mouse->ButtonPressed(Input::MouseButton::RightButton));
        mayaCameraUtil.SetZoomInButton(mouse->WheelForward());
        mayaCameraUtil.SetZoomOutButton(mouse->WheelBackward());
        mayaCameraUtil.SetMouseMovement(mouse->GetMovement());
        mayaCameraUtil.Update();
        CameraContext::SetTransform(cam, Math::inverse(mayaCameraUtil.GetCameraTransform()));
    }
    break;
    case FreeCamera:
    {
        RenderUtil::FreeCameraUtil & freeCamUtil = instanceData[instance].free;

        freeCamUtil.SetForwardsKey(keyboard->KeyPressed(Input::Key::W));
        freeCamUtil.SetBackwardsKey(keyboard->KeyPressed(Input::Key::S));
        freeCamUtil.SetRightStrafeKey(keyboard->KeyPressed(Input::Key::D));
        freeCamUtil.SetLeftStrafeKey(keyboard->KeyPressed(Input::Key::A));
        freeCamUtil.SetUpKey(keyboard->KeyPressed(Input::Key::Q));
        freeCamUtil.SetDownKey(keyboard->KeyPressed(Input::Key::E));

        freeCamUtil.SetMouseMovement(mouse->GetMovement());
        freeCamUtil.SetAccelerateButton(keyboard->KeyPressed(Input::Key::LeftShift));

        freeCamUtil.SetRotateButton(mouse->ButtonPressed(Input::MouseButton::LeftButton));
        freeCamUtil.Update();
        CameraContext::SetTransform(cam, Math::inverse(freeCamUtil.GetTransform()));
    }
    break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UtilCameraComponent::SetView(Game::Entity entity, Ptr<Graphics::View> const& view)
{
    auto instance = component->GetInstance(entity);
    if (instance != InvalidIndex)
    {
        component->Get<Attr::GraphicsEntity>(instance) = view->GetCamera().id;
        Math::vec4 camProj = component->Get<Attr::CameraProjection>(instance);
        CameraContext::SetupProjectionFov(view->GetCamera(), camProj.x, camProj.y, camProj.z, camProj.w);
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::FourCC
UtilCameraComponent::GetFourCC()
{
	return component->GetIdentifier();
}

} // namespace GraphicsFeature
