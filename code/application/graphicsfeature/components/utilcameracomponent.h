#pragma once
//------------------------------------------------------------------------------
/**
    UtilCameraComponent

    Utility Camera mainly for testing and prototyping with maya and freecam 
    support

    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"
#include "game/component/attribute.h"
#include "graphicsfeature/components/graphicsdata.h"
#include "graphics/view.h"

namespace GraphicsFeature
{

enum CameraMode
{
    MayaCamera = 0,
    FreeCamera = 1,
};

class UtilCameraComponent
{
    __DeclareComponent(UtilCameraComponent)
public:
    static void SetupAcceptedMessages();

    static void OnActivate(Game::InstanceId instance);
    static void OnDeactivate(Game::InstanceId instance);
    static void OnBeginFrame();
    
    static void SetView(Game::Entity entity, Ptr<Graphics::View> const& view);

    static void SetMode(Game::Entity entity, CameraMode mode);

    /// Return this components fourcc
    static Util::FourCC GetFourCC();
private:
    static void UpdateCamera(Game::InstanceId instance);    
};

} // namespace GraphicsFeature
