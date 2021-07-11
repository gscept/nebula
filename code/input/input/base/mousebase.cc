//------------------------------------------------------------------------------
//  mousebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input/base/mousebase.h"
#include "input/inputserver.h"

namespace Base
{
__ImplementClass(Base::MouseBase, 'MSBS', Input::InputHandler);

using namespace Input;

//------------------------------------------------------------------------------
/**
*/
MouseBase::MouseBase() :
    buttonStates(MouseButton::NumMouseButtons),
    wheelForward(false),
    wheelBackward(false),
    initialMouseMovement(false),
    mouseLocked(false),
    mouseWasLocked(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MouseBase::~MouseBase()
{
    n_assert(!this->IsAttached());
}

//------------------------------------------------------------------------------
/**
*/
void
MouseBase::BeginCapture()
{
    InputHandler::BeginCapture();
    InputServer::Instance()->ObtainMouseCapture(this);
}

//------------------------------------------------------------------------------
/**
*/
void
MouseBase::EndCapture()
{
    InputHandler::EndCapture();
    InputServer::Instance()->ReleaseMouseCapture(this);
}

//------------------------------------------------------------------------------
/**
*/
void
MouseBase::OnAttach()
{
    InputHandler::OnAttach();

    ButtonState initialButtonState;
    this->buttonStates.Fill(initialButtonState);
    this->beginFramePixelPosition.set(0.0f, 0.0f);
    this->beginFrameScreenPosition.set(0.0f, 0.0f);
    this->pixelPosition.set(0.0f, 0.0f);
    this->screenPosition.set(0.0f, 0.0f);
    this->movement.set(0.0f, 0.0f);
    this->wheelForward = false;
    this->wheelBackward = false;
    this->initialMouseMovement = true;
}

//------------------------------------------------------------------------------
/**
*/
void
MouseBase::OnBeginFrame()
{
    InputHandler::OnBeginFrame();

    // reset button up/down/doubleclick flags, but retain the pressed state
    IndexT i;
    for (i = 0; i < this->buttonStates.Size(); i++)
    {
        ButtonState& state = this->buttonStates[i];

        // NOTE: if the button was released in the previous frame,
        // clear the pressed state (see the ButtonUp-handling
        // code in OnEvent for details why this is a good thing)
        if (state.up)
        {
            state.pressed = false;
        }
        state.down = false;
        state.up = false;
        state.doubleClicked = false;
    }
    this->wheelForward = false;
    this->wheelBackward = false;
    this->beginFramePixelPosition = this->pixelPosition;
    this->beginFrameScreenPosition = this->screenPosition;
    this->movement.set(0.0f, 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
bool
MouseBase::OnEvent(const InputEvent& inputEvent)
{
    switch (inputEvent.GetType())
    {
        // reset input handler if another one begins to capture
        case InputEvent::BeginMouseCapture:
            if (!this->IsCapturing())
            {
                this->OnReset();
            }
            return true;

        case InputEvent::MouseMove:
            this->UpdateMousePositions(inputEvent.GetAbsMousePos(), inputEvent.GetNormMousePos());
            return true;

        case InputEvent::RawMouseMove:
            this->movement = inputEvent.GetAbsMousePos();
            return true;

        case InputEvent::MouseButtonDown:
            {
                MouseButton::Code btn = inputEvent.GetMouseButton();
                if (MouseButton::InvalidMouseButton != btn)
                {
                    this->buttonStates[btn].down = true;
                    this->buttonStates[btn].pressed = true;
                    this->UpdateMousePositions(inputEvent.GetAbsMousePos(), inputEvent.GetNormMousePos());
                }
            }
            return true;

        case InputEvent::MouseButtonUp:
            {
                MouseButton::Code btn = inputEvent.GetMouseButton();
                if (MouseButton::InvalidMouseButton != btn)
                {
                    this->buttonStates[btn].up = true;
                    // NOTE: we don't clear the pressed flag here because
                    // it may happen that a button was only tapped shortly
                    // within a frame, and in this case we still want the
                    // pressed flag to be set for the current frame (otherwise an application
                    // may miss the pressed state) instead, the pressed state will
                    // be cleared at the beginning of the next frame
                    // when the button up flag was set
                    this->UpdateMousePositions(inputEvent.GetAbsMousePos(), inputEvent.GetNormMousePos());
                }
            }
            return true;
       
        case InputEvent::MouseButtonDoubleClick:
            {
                MouseButton::Code btn = inputEvent.GetMouseButton();
                if (MouseButton::InvalidMouseButton != btn)
                {
                    this->buttonStates[btn].doubleClicked = true;
                    this->UpdateMousePositions(inputEvent.GetAbsMousePos(), inputEvent.GetNormMousePos());
                }
            }
            return true;

        case InputEvent::MouseWheelForward:
            this->wheelForward = true;
            return true;

        case InputEvent::MouseWheelBackward:
            this->wheelBackward = true;
            return true;

        case InputEvent::AppLoseFocus:
            {
                if (this->IsLocked())
                {
                    this->LockToPosition(false, mouseLockedPosition);
                    mouseWasLocked = true;
                }
                break;
            }

        case InputEvent::AppObtainFocus:
            {
                if (mouseWasLocked)
                {
                    this->LockToPosition(true, mouseLockedPosition);
                    mouseWasLocked = false;
                }
                break;
            }

        default:
            break;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
MouseBase::OnObtainCapture()
{
    InputHandler::OnObtainCapture();
    
    // put begin mouse capture event
    InputEvent beginCaptureEvent;
    beginCaptureEvent.SetType(InputEvent::BeginMouseCapture);
    InputServer::Instance()->PutEvent(beginCaptureEvent);
}

//------------------------------------------------------------------------------
/**
*/
void
MouseBase::OnReleaseCapture()
{
    // put end mouse capture event
    InputEvent endCaptureEvent;
    endCaptureEvent.SetType(InputEvent::EndMouseCapture);
    InputServer::Instance()->PutEvent(endCaptureEvent);

    InputHandler::OnReleaseCapture();
}

//------------------------------------------------------------------------------
/**
*/
void
MouseBase::UpdateMousePositions(const Math::vec2& pixelPos, const Math::vec2& screenPos)
{    
    // prevent a mouse-jump at the beginning
    if (this->initialMouseMovement)
    {
        this->beginFramePixelPosition = pixelPos;
        this->beginFrameScreenPosition = screenPos;
        this->initialMouseMovement = false;
    }
    this->pixelPosition = pixelPos;
    this->screenPosition = screenPos;
    this->movement = this->beginFramePixelPosition - this->pixelPosition;
}

//------------------------------------------------------------------------------
/**
*/
void
MouseBase::OnReset()
{
    // reset buttons
    IndexT i;
    for (i = 0; i < MouseButton::NumMouseButtons; i++)
    {
        ButtonState& state = this->buttonStates[i];
        if (state.pressed)
        {
            state.up = true;
        }
        else
        {
            state.up = false;
        }
        state.down = false;
        state.pressed = false;
    }
    this->pixelPosition.set(0.0f, 0.0f);
    this->screenPosition.set(0.0f, 0.0f);
    this->movement.set(0.0f, 0.0f);
    this->wheelForward = false;
    this->wheelBackward = false;
    this->initialMouseMovement = true;
}

} // namespace Base