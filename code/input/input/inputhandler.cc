//------------------------------------------------------------------------------
//  inputhandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input/inputhandler.h"

namespace Input
{
__ImplementClass(Input::InputHandler, 'INPH', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
InputHandler::InputHandler() :
    isAttached(false),
    isCapturing(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
InputHandler::~InputHandler()
{
    n_assert(!this->IsAttached());
}

//------------------------------------------------------------------------------
/**
    Begin capturing input to this input handler. This method must be
    overriden in a subclass, the derived method must call 
    ObtainMouseCapture(), ObtainKeyboardCapture(), or both, depending
    on what type input events you want to capture. An input handler
    which captures input gets all input events of the given type exclusively.
*/
void
InputHandler::BeginCapture()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    End capturing input to this input handler. Override this method
    in a subclass and release the captures obtained in BeginCapture().
*/
void
InputHandler::EndCapture()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
InputHandler::OnAttach()
{
    n_assert(!this->IsAttached());
    this->isAttached = true;
}

//------------------------------------------------------------------------------
/**
*/
void
InputHandler::OnRemove()
{
    n_assert(this->IsAttached());
    this->isAttached = false;
}

//------------------------------------------------------------------------------
/**
*/
void
InputHandler::OnBeginFrame()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
InputHandler::OnEndFrame()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
InputHandler::OnObtainCapture()
{
    this->isCapturing = true;
}

//------------------------------------------------------------------------------
/**
*/
void
InputHandler::OnReleaseCapture()
{
    this->isCapturing = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
InputHandler::OnEvent(const InputEvent& inputEvent)
{
    switch (inputEvent.GetType())
    {
#ifndef _DEBUG
        case InputEvent::AppObtainFocus:
        case InputEvent::AppLoseFocus:
#endif
        case InputEvent::Reset:
            this->OnReset();
            break;

        default:
            break;
    }        
    return true;
}

//------------------------------------------------------------------------------
/**
    OnReset is called when the app loses or gains focus (amongst other
    occasions). The input handler should reset its  state
    to prevent keys from sticking down, etc...
*/
void
InputHandler::OnReset()
{
    // empty
}

} // namespace Input


