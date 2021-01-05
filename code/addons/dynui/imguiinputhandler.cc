//------------------------------------------------------------------------------
//  imguiinputhandler.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "imguiinputhandler.h"
#include "input/inputserver.h"
#include "imguicontext.h"
#include "imgui.h"

namespace Dynui
{
__ImplementClass(Dynui::ImguiInputHandler, 'IMIH', Input::InputHandler);

//------------------------------------------------------------------------------
/**
*/
ImguiInputHandler::ImguiInputHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ImguiInputHandler::~ImguiInputHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiInputHandler::BeginCapture()
{
    Input::InputServer::Instance()->ObtainMouseCapture(this);
    Input::InputServer::Instance()->ObtainKeyboardCapture(this);
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiInputHandler::EndCapture()
{
    Input::InputServer::Instance()->ReleaseMouseCapture(this);
    Input::InputServer::Instance()->ReleaseKeyboardCapture(this);
}

//------------------------------------------------------------------------------
/**
*/
bool
ImguiInputHandler::OnEvent(const Input::InputEvent& inputEvent)
{   
    switch (inputEvent.GetType())
    {
#ifndef _DEBUG
    case Input::InputEvent::AppObtainFocus:
    case Input::InputEvent::AppLoseFocus:
#endif
    case Input::InputEvent::Reset:
        this->OnReset();
        break;

    default:
        return ImguiContext::HandleInput(inputEvent);
    }
    return false;
}

} // namespace Dynui