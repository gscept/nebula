#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::MouseBase
  
    An input handler which represents a mouse for polling.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "input/inputhandler.h"
#include "input/mousebutton.h"

//------------------------------------------------------------------------------
namespace Base
{
class MouseBase : public Input::InputHandler
{
    __DeclareClass(MouseBase);
public:
    /// constructor
    MouseBase();
    /// destructor
    virtual ~MouseBase();
    
    /// capture input to this event handler
    virtual void BeginCapture();
    /// end input capturing to this event handler
    virtual void EndCapture();

    /// return true if button is currently pressed
    bool ButtonPressed(Input::MouseButton::Code btn) const;
    /// return true if button was pushed down at least once in current frame
    bool ButtonDown(Input::MouseButton::Code btn) const;
    /// return true if button was released at least once in current frame
    bool ButtonUp(Input::MouseButton::Code btn) const;
    /// return true if a button has been double clicked
    bool ButtonDoubleClicked(Input::MouseButton::Code btn) const;
    /// return true if mouse wheel rotated forward
    bool WheelForward() const;
    /// return true if mouse wheel rotated backward
    bool WheelBackward() const;
    /// get current absolute mouse position (in pixels)
    const Math::float2& GetPixelPosition() const;
    /// get current screen space mouse position (0.0 .. 1.0)
    const Math::float2& GetScreenPosition() const;
    /// get mouse movement
    const Math::float2& GetMovement() const;

	/// locks the mouse to a screen position (0.0 .. 1.0)
	virtual void LockToPosition(bool locked, Math::float2& position);
	/// return true if the mouse is locked
	bool IsLocked() const;    

protected:
    /// called when the handler is attached to the input server
    virtual void OnAttach();
    /// called on InputServer::BeginFrame()
    virtual void OnBeginFrame();
    /// called when an input event should be processed
    virtual bool OnEvent(const Input::InputEvent& inputEvent);
    /// called when input handler obtains capture
    virtual void OnObtainCapture();
    /// called when input handler looses capture
    virtual void OnReleaseCapture();
    /// reset the input handler
    virtual void OnReset();

private:
    /// update mouse position members
    void UpdateMousePositions(const Math::float2& pixelPos, const Math::float2& screenPos);

    struct ButtonState
    {
    public:
        /// constructor
        ButtonState() : pressed(false), down(false), up(false), doubleClicked(false) {};

        bool pressed;
        bool down;
        bool up;
        bool doubleClicked;
    };

    Util::FixedArray<ButtonState> buttonStates;
    Math::float2 beginFramePixelPosition;
    Math::float2 beginFrameScreenPosition;
    Math::float2 pixelPosition;
    Math::float2 screenPosition;
    Math::float2 movement;
	Math::float2 mouseLockedPosition;
    bool wheelForward;
    bool wheelBackward;
    bool initialMouseMovement;
	bool mouseLocked;
	bool mouseWasLocked;
};

//------------------------------------------------------------------------------
/**
*/
inline bool 
MouseBase::ButtonPressed(Input::MouseButton::Code btn) const
{
    return this->buttonStates[btn].pressed;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MouseBase::ButtonDown(Input::MouseButton::Code btn) const
{
    return this->buttonStates[btn].down;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MouseBase::ButtonUp(Input::MouseButton::Code btn) const
{
    return this->buttonStates[btn].up;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MouseBase::ButtonDoubleClicked(Input::MouseButton::Code btn) const
{
    return this->buttonStates[btn].doubleClicked;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
MouseBase::GetPixelPosition() const
{
    return this->pixelPosition;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
MouseBase::GetScreenPosition() const
{
    return this->screenPosition;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
MouseBase::GetMovement() const
{
    return this->movement;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MouseBase::WheelForward() const
{
    return this->wheelForward;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MouseBase::WheelBackward() const
{
    return this->wheelBackward;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MouseBase::LockToPosition(bool locked, Math::float2& position)
{
	mouseLockedPosition = position;
	mouseLocked = locked;
	mouseWasLocked = locked;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MouseBase::IsLocked() const
{
	return mouseLocked;
}

} // namespace Base
//------------------------------------------------------------------------------

