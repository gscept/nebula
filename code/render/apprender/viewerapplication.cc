//------------------------------------------------------------------------------
//  viewerapplication.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "apprender/viewerapplication.h"
#include "visibility/visibilitysystems/visibilityquadtreesystem.h"
#include "visibility/visibilitysystems/visibilityclustersystem.h"
#include "visibility/visibilitysystems/visibilityboxsystem.h"
#include "graphics/view.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/gamepad.h"
#include "graphics/view.h"
#include "frame2/frameserver.h"



namespace App
{
using namespace Math;
using namespace Graphics;
using namespace Util;
using namespace Resources;
using namespace Input;
using namespace Debug;
using namespace Visibility;


//------------------------------------------------------------------------------
/**
*/
ViewerApplication::ViewerApplication() :
    useResolveRect(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ViewerApplication::~ViewerApplication()
{
    n_assert(!this->stage.isvalid());
}

//------------------------------------------------------------------------------
/**
*/
bool
ViewerApplication::Open()
{
    if (RenderApplication::Open())
    {
        StringAtom defaultStageName("DefaultStage");
        StringAtom defaultViewName("DefaultView");
                                                         
        // create a GraphicServer, Stage and View
        this->graphicsServer = GraphicsServer::Instance();
        this->debugShapeRenderer = DebugShapeRenderer::Create();
        this->debugTextRenderer = DebugTextRenderer::Create();

        // create a default stage
        // attach visibility systems to checker
        Ptr<Visibility::VisibilityQuadtreeSystem> visQuadtreeSystem = Visibility::VisibilityQuadtreeSystem::Create();
        visQuadtreeSystem->SetQuadTreeSettings(4, Math::bbox(Math::point(0,0,0), Math::vector(200.0f, 200.0f, 200.0f)));
        Ptr<Visibility::VisibilityClusterSystem> visClusterSystem = Visibility::VisibilityClusterSystem::Create();
        Ptr<Visibility::VisibilityBoxSystem> visBoxSystem = Visibility::VisibilityBoxSystem::Create();
        
        Util::Array<Ptr<VisibilitySystemBase> > visSystems;
        visSystems.Append(visQuadtreeSystem.cast<VisibilitySystemBase>());
        //visSystems.Append(visClusterSystem.cast<VisibilitySystemBase>());
		//visSystems.Append(visBoxSystem.cast<VisibilitySystemBase>());
        this->stage = this->graphicsServer->CreateStage(defaultStageName, visSystems);

        // create a default view
        // FIXME: sucks that I have to use Graphics here!
        this->view = this->graphicsServer->CreateView(Graphics::View::RTTI,
                                                      defaultViewName,
													  0,
                                                      true);

		Frame::FrameServer::Instance()->SetWindowTexture(CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow()->GetRenderTexture());
		Ptr<Frame::FrameScript> frameScript = Frame::FrameServer::Instance()->LoadFrameScript("test", "frame:vkdebug.json");

		// set stage
		this->view->SetStage(this->stage);
		this->view->SetFrameScript(frameScript);
		if (this->useResolveRect) this->view->SetResolveRect(this->resolveRect);

        // create a camera entity
        this->camera = CameraEntity::Create();
        this->stage->AttachEntity(this->camera.cast<GraphicsEntity>());
        this->view->SetCameraEntity(this->camera);

#ifndef FREECAM
        // setup the camera util object
        this->mayaCameraUtil.Setup(point(0.0f, 0.0f, 0.0f), point(5.0f, 5.0f, 5.0f), vector(0.0f, 1.0f, 0.0f));
        this->mayaCameraUtil.Update();
        this->camera->SetTransform(this->mayaCameraUtil.GetCameraTransform());
#else
        this->freeCameraUtil.Setup(point(100,5,5), vector(0,0,0));
        this->freeCameraUtil.Update();
        this->camera->SetTransform(this->freeCameraUtil.GetTransform());
#endif
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ViewerApplication::Close()
{
    this->stage->RemoveEntity(this->camera.cast<GraphicsEntity>());
    this->camera = 0;

    this->graphicsServer->DiscardView(this->view);
    this->view = 0;
    
    this->graphicsServer->DiscardStage(this->stage);
    this->stage = 0;

    this->debugTextRenderer = 0;
    this->debugShapeRenderer = 0;
    this->graphicsServer = 0;

    // call parent class
    RenderApplication::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
ViewerApplication::OnProcessInput()
{
    // update the camera from input
    InputServer* inputServer = InputServer::Instance();
    const Ptr<Keyboard>& keyboard = inputServer->GetDefaultKeyboard();
    const Ptr<Mouse>& mouse = inputServer->GetDefaultMouse();
    const Ptr<GamePad>& gamePad = inputServer->GetDefaultGamePad(0);

#ifndef FREECAM
    // standard input handling: manipulate camera
    this->mayaCameraUtil.SetOrbitButton(mouse->ButtonPressed(MouseButton::LeftButton));
    this->mayaCameraUtil.SetPanButton(mouse->ButtonPressed(MouseButton::MiddleButton));
    this->mayaCameraUtil.SetZoomButton(mouse->ButtonPressed(MouseButton::RightButton));
    this->mayaCameraUtil.SetZoomInButton(mouse->WheelForward());
    this->mayaCameraUtil.SetZoomOutButton(mouse->WheelBackward());
    this->mayaCameraUtil.SetMouseMovement(mouse->GetMovement());
    
    // process gamepad input
    float zoomIn = 0.0f;
    float zoomOut = 0.0f;
    float2 panning(0.0f, 0.0f);
    float2 orbiting(0.0f, 0.0f);
    if (gamePad->IsConnected())
    {
        const float gamePadZoomSpeed = 5.0f;
        const float gamePadOrbitSpeed = 1.0f;
        const float gamePadPanSpeed = 10.0f;
        if (gamePad->ButtonDown(GamePad::AButton))
        {
            this->mayaCameraUtil.Reset();
        }
        if (gamePad->ButtonDown(GamePad::StartButton) ||
            gamePad->ButtonDown(GamePad::BackButton))
        {
            this->SetQuitRequested(true);
        }
        float frameTime = (float) this->GetFrameTime();
        zoomIn       += gamePad->GetAxisValue(GamePad::RightTriggerAxis) * frameTime * gamePadZoomSpeed;
        zoomOut      += gamePad->GetAxisValue(GamePad::LeftTriggerAxis) * frameTime * gamePadZoomSpeed;
        panning.x()  -= gamePad->GetAxisValue(GamePad::RightThumbXAxis) * frameTime * gamePadPanSpeed;
        panning.y()  += gamePad->GetAxisValue(GamePad::RightThumbYAxis) * frameTime * gamePadPanSpeed;
        orbiting.x() -= gamePad->GetAxisValue(GamePad::LeftThumbXAxis) * frameTime * gamePadOrbitSpeed;
        orbiting.y() += gamePad->GetAxisValue(GamePad::LeftThumbYAxis) * frameTime * gamePadOrbitSpeed;
    }

    // process keyboard input
    if (keyboard->KeyDown(Key::Escape))
    {
        this->SetQuitRequested(true);
    }
    if (keyboard->KeyDown(Key::Space))
    {
        this->mayaCameraUtil.Reset();
    }
    if (keyboard->KeyPressed(Key::Left))
    {
        panning.x() -= 0.1f;
    }
    if (keyboard->KeyPressed(Key::Right))
    {
        panning.x() += 0.1f;
    }
    if (keyboard->KeyPressed(Key::Up))
    {
        panning.y() -= 0.1f;
    }
    if (keyboard->KeyPressed(Key::Down))
    {
        panning.y() += 0.1f;
    }

    this->mayaCameraUtil.SetPanning(panning);
    this->mayaCameraUtil.SetOrbiting(orbiting);
    this->mayaCameraUtil.SetZoomIn(zoomIn);
    this->mayaCameraUtil.SetZoomOut(zoomOut);
    this->mayaCameraUtil.Update();
    this->camera->SetTransform(this->mayaCameraUtil.GetCameraTransform());
#else
	this->freeCameraUtil.SetRotateButton(mouse->ButtonPressed(MouseButton::LeftButton));
	this->freeCameraUtil.SetAccelerateButton(keyboard->KeyPressed(Key::Shift));
	this->freeCameraUtil.SetForwardsKey(keyboard->KeyPressed(Key::W));
	this->freeCameraUtil.SetBackwardsKey(keyboard->KeyPressed(Key::S));
	this->freeCameraUtil.SetLeftStrafeKey(keyboard->KeyPressed(Key::A));
	this->freeCameraUtil.SetRightStrafeKey(keyboard->KeyPressed(Key::D));
	this->freeCameraUtil.SetUpKey(keyboard->KeyPressed(Key::Space));
	this->freeCameraUtil.SetDownKey(keyboard->KeyPressed(Key::C));
	this->freeCameraUtil.SetMouseMovement(mouse->GetMovement());
	this->freeCameraUtil.Update();
	this->camera->SetTransform(this->freeCameraUtil.GetTransform());
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
ViewerApplication::OnUpdateFrame()
{
    this->debugShapeRenderer->OnFrame();
    this->debugTextRenderer->OnFrame();
}

} // namespace App

    
