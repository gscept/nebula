//------------------------------------------------------------------------------
//  TBUIinputhandler.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "tbuiinputhandler.h"
#include "input/inputserver.h"
#include "tbuicontext.h"

namespace TBUI
{
__ImplementClass(TBUI::TBUIInputHandler, 'TBIH', Input::InputHandler);

//------------------------------------------------------------------------------
/**
*/
TBUIInputHandler::TBUIInputHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TBUIInputHandler::~TBUIInputHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIInputHandler::BeginCapture()
{
    Input::InputServer::Instance()->ObtainMouseCapture(this);
    Input::InputServer::Instance()->ObtainKeyboardCapture(this);
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIInputHandler::EndCapture()
{
    Input::InputServer::Instance()->ReleaseMouseCapture(this);
    Input::InputServer::Instance()->ReleaseKeyboardCapture(this);
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIInputHandler::OnBeginFrame()
{
}

//------------------------------------------------------------------------------
/**
*/
bool
TBUIInputHandler::OnEvent(const Input::InputEvent& inputEvent)
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
        return TBUIContext::ProcessInput(inputEvent);
    }
    return false;
}

} // namespace TBUI
