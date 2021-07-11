#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::KeyboardBase

    An input handler which represents a keyboard for polling.

    @copyright
    (C) 2007 Radon Labs GmbH    
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "input/inputhandler.h"
#include "input/key.h"

//------------------------------------------------------------------------------
namespace Base
{
class KeyboardBase : public Input::InputHandler
{
    __DeclareClass(KeyboardBase);
public:
    /// constructor
    KeyboardBase();
    /// destructor
    virtual ~KeyboardBase();
    
    /// capture input to this event handler
    virtual void BeginCapture();
    /// end input capturing to this event handler
    virtual void EndCapture();

    /// sets a key to be down
    void SetKeyDown(Input::Key::Code keyCode);
    /// sets a key to be up
    void SetKeyUp(Input::Key::Code keyCode);

    /// return true if a key is currently pressed
    bool KeyPressed(Input::Key::Code keyCode) const;
    /// return true if key was pushed down at least once in current frame
    bool KeyDown(Input::Key::Code keyCode) const;
    /// return true if key was released at least once in current frame
    bool KeyUp(Input::Key::Code keyCode) const;
    /// get character input in current frame
    const Util::String& GetCharInput() const;

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
    class KeyState
    {
    public:
        /// constructor
        KeyState() : pressed(false), down(false), up(false) {};

        bool pressed;       // currently pressed
        bool down;          // was down this frame
        bool up;            // was up this frame
    };
    Util::FixedArray<KeyState> keyStates;
    Util::FixedArray<KeyState> nextKeyStates;
    Util::String charInput;
};

//------------------------------------------------------------------------------
/**
*/
inline void
KeyboardBase::SetKeyDown(Input::Key::Code keyCode)
{
    KeyState& keyState = this->keyStates[keyCode];
    if (!keyState.pressed)
    {
        keyState.down = true;
        keyState.pressed = true;
    }   
}

//------------------------------------------------------------------------------
/**
*/
inline void
KeyboardBase::SetKeyUp(Input::Key::Code keyCode)
{
    this->keyStates[keyCode].up = true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
KeyboardBase::KeyPressed(Input::Key::Code keyCode) const
{
    return this->keyStates[keyCode].pressed;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
KeyboardBase::KeyDown(Input::Key::Code keyCode) const
{
    return this->keyStates[keyCode].down;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
KeyboardBase::KeyUp(Input::Key::Code keyCode) const
{
    return this->keyStates[keyCode].up;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
KeyboardBase::GetCharInput() const
{
    return this->charInput;
}

} // namespace Input
//------------------------------------------------------------------------------

