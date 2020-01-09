//------------------------------------------------------------------------------
//  glfwinputdisplayeventhandler.cc
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "input/glfw/glfwinputdisplayeventhandler.h"
#include "input/inputserver.h"
#include "input/inputevent.h"

namespace GLFW
{
__ImplementClass(GLFW::GLFWInputDisplayEventHandler, 'WIEH', CoreGraphics::DisplayEventHandler);

using namespace Input;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
bool
GLFWInputDisplayEventHandler::HandleEvent(const DisplayEvent& displayEvent)
{
    InputEvent inputEvent;
    InputServer* inputServer = InputServer::Instance();
    switch (displayEvent.GetEventCode())
    {
        case DisplayEvent::CloseRequested:
            inputServer->SetQuitRequested(true);
            break;

        case DisplayEvent::WindowMinimized:
        case DisplayEvent::KillFocus:
            inputEvent.SetType(InputEvent::AppLoseFocus);
            inputServer->PutEvent(inputEvent);            
            return true;

        case DisplayEvent::WindowRestored:
        case DisplayEvent::SetFocus:
            inputEvent.SetType(InputEvent::AppObtainFocus);
            inputServer->PutEvent(inputEvent);            
            return true;

        case DisplayEvent::KeyDown:
            inputEvent.SetType(InputEvent::KeyDown);
            inputEvent.SetKey(displayEvent.GetKey());
            inputServer->PutEvent(inputEvent);
            return true;

        case DisplayEvent::KeyUp:
            inputEvent.SetType(InputEvent::KeyUp);
            inputEvent.SetKey(displayEvent.GetKey());
            inputServer->PutEvent(inputEvent);
            return true;

        case DisplayEvent::Character:
            inputEvent.SetType(InputEvent::Character);
            inputEvent.SetChar(displayEvent.GetChar());
            inputServer->PutEvent(inputEvent);
            return true;

        case DisplayEvent::MouseButtonDown:
            inputEvent.SetType(InputEvent::MouseButtonDown);
            inputEvent.SetMouseButton(displayEvent.GetMouseButton());
            inputEvent.SetAbsMousePos(displayEvent.GetAbsMousePos());
            inputEvent.SetNormMousePos(displayEvent.GetNormMousePos());
            inputServer->PutEvent(inputEvent);
            return true;

        case DisplayEvent::MouseButtonUp:
            inputEvent.SetType(InputEvent::MouseButtonUp);
            inputEvent.SetMouseButton(displayEvent.GetMouseButton());
            inputEvent.SetAbsMousePos(displayEvent.GetAbsMousePos());
            inputEvent.SetNormMousePos(displayEvent.GetNormMousePos());
            inputServer->PutEvent(inputEvent);
            return true;

        case DisplayEvent::MouseButtonDoubleClick:
            inputEvent.SetType(InputEvent::MouseButtonDoubleClick);
            inputEvent.SetMouseButton(displayEvent.GetMouseButton());
            inputEvent.SetAbsMousePos(displayEvent.GetAbsMousePos());
            inputEvent.SetNormMousePos(displayEvent.GetNormMousePos());
            inputServer->PutEvent(inputEvent);
            return true;

        case DisplayEvent::MouseMove:
            inputEvent.SetType(InputEvent::MouseMove);
            inputEvent.SetAbsMousePos(displayEvent.GetAbsMousePos());
            inputEvent.SetNormMousePos(displayEvent.GetNormMousePos());
            inputServer->PutEvent(inputEvent);
            return true;

        case DisplayEvent::MouseWheelForward:
            inputEvent.SetType(InputEvent::MouseWheelForward);
            inputServer->PutEvent(inputEvent);
            return true;

        case DisplayEvent::MouseWheelBackward:
            inputEvent.SetType(InputEvent::MouseWheelBackward);
            inputServer->PutEvent(inputEvent);
            return true;
    }
    return false;
}
    
} // namespace GLFW