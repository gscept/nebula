//------------------------------------------------------------------------------
//  inputserverbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "input/base/inputserverbase.h"
#include "input/inputhandler.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/gamepad.h"
#include "coregraphics/displaydevice.h"

namespace Base
{
__ImplementClass(Base::InputServerBase, 'IPSB', Core::RefCounted);

using namespace CoreGraphics;
using namespace Input;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
InputServerBase::InputServerBase() :
    isOpen(false),
    inBeginFrame(false),
    isQuitRequested(false),
    hasFocus(false),
    inputHandlersLockCount(0),
    maxNumLocalPlayers(4),
    defaultGamePad(GamePad::GetMaxNumControllers()),
    creatorThreadId(Threading::Thread::GetMyThreadId())
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
InputServerBase::~InputServerBase()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
    Setup the maximum number of local players for this application. The
    default number is 1. This defines the number of game pad objects
    created and queried.
*/
void
InputServerBase::SetMaxNumLocalPlayers(SizeT num)
{
    n_assert(!this->isOpen);
    n_assert(num <= GamePad::GetMaxNumControllers());
    this->maxNumLocalPlayers = num;
}

//------------------------------------------------------------------------------
/**
*/
void
InputServerBase::Open()
{
    n_assert(!this->isOpen);
    n_assert(!this->inBeginFrame);
    this->isOpen = true;
    this->hasFocus = true; // Assume we have focus when opening
}

//------------------------------------------------------------------------------
/**
*/
void
InputServerBase::Close()
{
    n_assert(this->IsOpen());

    // remove all input handlers
    IndexT i;
    for (i = 0; i < this->inputHandlers.Size(); i++)
    {
        this->inputHandlers[i].Value()->OnRemove();
    }
    this->inputHandlers.Clear();
    this->keyboardCaptureHandler = nullptr;
    this->mouseCaptureHandler = nullptr;

    // release default input handlers
    this->defaultKeyboard = nullptr;
    this->defaultMouse = nullptr;
    this->defaultGamePad.Fill(nullptr);

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
InputServerBase::Reset()
{
    InputEvent resetEvent;
    resetEvent.SetType(InputEvent::Reset);
    this->PutEvent(resetEvent);
    if (this->mouseCaptureHandler.isvalid())
    {
        this->ReleaseMouseCapture(this->mouseCaptureHandler);
    }
    if (this->keyboardCaptureHandler.isvalid())
    {
        this->ReleaseKeyboardCapture(this->keyboardCaptureHandler);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
InputServerBase::AttachInputHandler(InputPriority::Code pri, const Ptr<InputHandler>& inputHandler)
{
    n_assert(0 == this->inputHandlersLockCount);

    // make sure the input handler isn't attached yet
    IndexT i;
    for (i = 0; i < this->inputHandlers.Size(); i++)
    {
        n_assert(inputHandler != this->inputHandlers[i].Value());
    }    

    // add a new entry to the sorted input handler array
    KeyValuePair<InputPriority::Code,Ptr<InputHandler> > kvp(pri, inputHandler);
    this->inputHandlers.InsertSorted(kvp);
    inputHandler->OnAttach();
}

//------------------------------------------------------------------------------
/**
*/
void
InputServerBase::RemoveInputHandler(const Ptr<InputHandler>& inputHandler)
{
    n_assert(0 == this->inputHandlersLockCount);

    // check if the input handler currently captures input
    if (this->mouseCaptureHandler.isvalid() && (this->mouseCaptureHandler == inputHandler))
    {
        this->ReleaseMouseCapture(inputHandler);
    }
    if (this->keyboardCaptureHandler.isvalid() && (this->keyboardCaptureHandler == inputHandler))
    {
        this->ReleaseKeyboardCapture(inputHandler);
    }

    // remove input handler
    IndexT i;
    for (i = 0; i < this->inputHandlers.Size(); i++)
    {
        if (inputHandler == this->inputHandlers[i].Value())
        {
            this->inputHandlers[i].Value()->OnRemove();
            this->inputHandlers.EraseIndex(i);
            return;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
InputServerBase::BeginFrame()
{
    n_assert(!this->inBeginFrame);
    this->inBeginFrame = true;

    // notify input handlers
    this->inputHandlersLockCount++;
    IndexT i;
    for (i = 0; i < this->inputHandlers.Size(); i++)
    {
        this->inputHandlers[i].Value()->OnBeginFrame();
    }
    this->inputHandlersLockCount--;

    // update events
    DisplayDevice::Instance()->ProcessWindowMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
InputServerBase::OnFrame()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Call this somewhere towards the end of frame, when it is guaraneteed
    that noone needs input anymore.
*/
void
InputServerBase::EndFrame()
{
    n_assert(this->inBeginFrame);

    // notify input handlers
    this->inputHandlersLockCount++;
    IndexT i;
    for (i = 0; i < this->inputHandlers.Size(); i++)
    {
        this->inputHandlers[i].Value()->OnEndFrame();
    }
    this->inputHandlersLockCount--;
    this->inBeginFrame = false;
}

//------------------------------------------------------------------------------
/**
    NOTE: MouseMove and RawMouseMove events will be distributed to all
    input handlers regardless of mouse capture state!
*/
void
InputServerBase::PutEvent(const InputEvent& inputEvent)
{
    n_assert2(creatorThreadId == Threading::Thread::GetMyThreadId(), "PutEvent can't be called from any thread but the creator thread!");
    
    // check for mouse capture
    if (this->mouseCaptureHandler.isvalid())
    {
        // route mouse events exclusively to mouse capture handler
        switch (inputEvent.GetType())
        {
            // do not route this events exclusively
            case InputEvent::BeginMouseCapture:
            case InputEvent::EndMouseCapture:
                break;

            case InputEvent::AppLoseFocus:
            case InputEvent::Reset:
                this->ReleaseMouseCapture(this->mouseCaptureHandler);
                break;

            case InputEvent::MouseButtonDown:
            case InputEvent::MouseButtonUp:
            case InputEvent::MouseButtonDoubleClick:
            case InputEvent::MouseWheelForward:
            case InputEvent::MouseWheelBackward:
                this->mouseCaptureHandler->OnEvent(inputEvent);
                return;
            default:
                break;
        }
    }

    // check for keyboard capture
    if (this->keyboardCaptureHandler.isvalid())
    {
        // route keyboard events exclusively to keyboard capture handler
        switch (inputEvent.GetType())
        {
            // do not route this events exclusively
            case InputEvent::BeginKeyboardCapture:
            case InputEvent::EndKeyboardCapture:
                break;

            case InputEvent::AppLoseFocus:
            case InputEvent::Reset:
                this->ReleaseKeyboardCapture(this->keyboardCaptureHandler);
                break;

            case InputEvent::KeyDown:
            case InputEvent::KeyUp:
            case InputEvent::Character:
                this->keyboardCaptureHandler->OnEvent(inputEvent);
                return;

            default:
                break;
        }
    }

    switch (inputEvent.GetType())
    {
        case InputEvent::AppLoseFocus:
            this->hasFocus = false;
            break;

        case InputEvent::AppObtainFocus:
            this->hasFocus = true;
            break;
    }

    // normal input event handling
    this->inputHandlersLockCount++;
    bool handled = false;
    IndexT i;
    for (i = 0; i < this->inputHandlers.Size(); i++)
    {
        if (this->inputHandlers[i].Value()->OnEvent(inputEvent))
        {
            handled = true;
            break;
        }
    }
    //n_assert(handled);
    this->inputHandlersLockCount--;
}

//------------------------------------------------------------------------------
/**
    Obtain the mouse capture. All mouse input will go exclusively to the
    capture input handler until ReleaseMouseCapture() is called.
*/
void
InputServerBase::ObtainMouseCapture(const Ptr<InputHandler>& inputHandler)
{
    if (this->mouseCaptureHandler.isvalid())
    {
        this->mouseCaptureHandler->OnReleaseCapture();
        this->mouseCaptureHandler = nullptr;
    }
    this->mouseCaptureHandler = inputHandler;
    this->mouseCaptureHandler->OnObtainCapture();
}

//------------------------------------------------------------------------------
/**
    Release the mouse capture. The provided pointer must match the current
    capture input handler.
*/
void
InputServerBase::ReleaseMouseCapture(const Ptr<InputHandler>& inputHandler)
{
    if (this->mouseCaptureHandler.isvalid() && (this->mouseCaptureHandler == inputHandler))
    {
        this->mouseCaptureHandler->OnReleaseCapture();
        this->mouseCaptureHandler = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<InputHandler>&
InputServerBase::GetMouseCaptureHandler() const
{
    return this->mouseCaptureHandler;
}

//------------------------------------------------------------------------------
/**
    This clears the currently set mouse capture (if exists).
*/
void
InputServerBase::ClearMouseCapture()
{
    if (this->mouseCaptureHandler.isvalid())
    {
        this->mouseCaptureHandler->OnReleaseCapture();
        this->mouseCaptureHandler = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
    This clears the currently set keyboard capture (if exists).
*/
void
InputServerBase::ClearKeyboardCapture()
{
    if (this->keyboardCaptureHandler.isvalid())
    {
        this->keyboardCaptureHandler->OnReleaseCapture();
        this->keyboardCaptureHandler = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
    This clears the mouse and keyboards captures, if set.
*/
void
InputServerBase::ClearCapture()
{
    this->ClearMouseCapture();
    this->ClearKeyboardCapture();
}

//------------------------------------------------------------------------------
/**
    Obtain the keyboard capture. All keyboard input will go exclusively to the
    capture input handler until ReleaseKeyboardCapture() is called.
*/
void
InputServerBase::ObtainKeyboardCapture(const Ptr<InputHandler>& inputHandler)
{
    if (this->keyboardCaptureHandler.isvalid())
    {
        this->keyboardCaptureHandler->OnReleaseCapture();
        this->keyboardCaptureHandler = nullptr;
    }
    this->keyboardCaptureHandler = inputHandler;
    this->keyboardCaptureHandler->OnObtainCapture();
}

//------------------------------------------------------------------------------
/**
    Release the mouse capture. The provided pointer must match the current
    capture input handler.
*/
void
InputServerBase::ReleaseKeyboardCapture(const Ptr<InputHandler>& inputHandler)
{
    if (this->keyboardCaptureHandler.isvalid() && (this->keyboardCaptureHandler == inputHandler))
    {
        this->keyboardCaptureHandler->OnReleaseCapture();
        this->keyboardCaptureHandler = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<InputHandler>&
InputServerBase::GetKeyboardCaptureHandler() const
{
    return this->keyboardCaptureHandler;
}
    
//------------------------------------------------------------------------------
/**
*/
Ptr<Input::GamePad>
InputServerBase::GetDefaultGamePad(IndexT playerIndex) const
{
    this->critSect.Enter();
    n_assert(playerIndex < this->maxNumLocalPlayers);
    Ptr<Input::GamePad> result = this->defaultGamePad[playerIndex];
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
void 
InputServerBase::AddGamePad(const Ptr<Input::GamePad> pad, IndexT playerIndex)
{
    n_assert(playerIndex < this->maxNumLocalPlayers);
    this->critSect.Enter();
    n_assert(!this->defaultGamePad[playerIndex].isvalid());
    this->defaultGamePad[playerIndex] = pad;
    this->critSect.Leave();
    
}

} // namespace Base
