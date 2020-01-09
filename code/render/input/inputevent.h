#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::InputEvent
    
    The input events of the Input subsystems. Input events are generated
    by the InputServer and travel through the input handler chain where
    they are processed.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "input/key.h"
#include "input/char.h"
#include "input/mousebutton.h"
#include "math/float2.h"

//------------------------------------------------------------------------------
namespace Input
{
class InputEvent
{
public:
    /// input event types
    enum Type
    {
        InvalidType = 0,

        AppObtainFocus,         // application window obtained focus (or minimized, etc)
        AppLoseFocus,           // application window lost focus
        Reset,                  // InputServer::Reset() has been called
        KeyDown,                // a key has been pressed
        KeyUp,                  // a key has been released
        Character,              // a translated character has been input
        MouseMove,              // mouse has been moved (limited by screen borders)
        RawMouseMove,           // raw mouse movement (not limited by screen borders)
        MouseButtonDown,        // a mouse button has been pressed
        MouseButtonUp,          // a mouse button has been released
        MouseButtonDoubleClick, // a mouse button has been double-clicked
        MouseWheelForward,      // mouse wheel moved forward
        MouseWheelBackward,     // mouse wheel moved backward
        BeginMouseCapture,      // a mouse input handler begin to capture
        EndMouseCapture,        // a mouse input handler end his capture
        BeginKeyboardCapture,   // a keyboard input handler begin to capture
        EndKeyboardCapture,     // a keyboard input handler end his capture

        GamePadButtonDown,      // a game pad button has been pressed
        GamePadButtonUp,        // a game pad button has been released
    };

    /// default constructor
    InputEvent();

    /// set event type
    void SetType(Type t);
    /// get event type
    Type GetType() const;
    /// set key code
    void SetKey(Key::Code key);
    /// get key code
    Key::Code GetKey() const;
    /// set character code
    void SetChar(Char chr);
    /// get character code
    Char GetChar() const;
    /// set button code
    void SetMouseButton(MouseButton::Code button);
    /// get button code
    MouseButton::Code GetMouseButton() const;
    /// set optional device index (e.g. player index for game pads)
    void SetDeviceIndex(IndexT i);
    /// get device index
    IndexT GetDeviceIndex() const;
    /// set absolute pixel mouse position
    void SetAbsMousePos(const Math::float2& p);
    /// get absolute pixel mouse position
    const Math::float2& GetAbsMousePos() const;
    /// set normalized mouse position
    void SetNormMousePos(const Math::float2& p);
    /// get normalized mouse position
    const Math::float2& GetNormMousePos() const;
    /// convert type to string
    static const char* TypeToString(Type t);

private:
    Type type;
    Key::Code keyCode;
    Char character;
    IndexT deviceIndex;
    MouseButton::Code mouseButton;
    Math::float2 absMousePos;
    Math::float2 normMousePos;
};

//------------------------------------------------------------------------------
/**
*/
inline
InputEvent::InputEvent() :
    type(InvalidType),
    keyCode(Key::InvalidKey),
    character(0),
    deviceIndex(0),
    mouseButton(MouseButton::InvalidMouseButton)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
InputEvent::SetType(Type t)
{
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline InputEvent::Type
InputEvent::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InputEvent::SetKey(Key::Code key)
{
    this->keyCode = key;
}

//------------------------------------------------------------------------------
/**
*/
inline Key::Code
InputEvent::GetKey() const
{
    return this->keyCode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InputEvent::SetChar(Char chr)
{
    this->character = chr;
}

//------------------------------------------------------------------------------
/**
*/
inline Char
InputEvent::GetChar() const
{
    return this->character;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InputEvent::SetMouseButton(MouseButton::Code btn)
{
    this->mouseButton = btn;
}

//------------------------------------------------------------------------------
/**
*/
inline MouseButton::Code
InputEvent::GetMouseButton() const
{
    return this->mouseButton;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InputEvent::SetAbsMousePos(const Math::float2& p)
{
    this->absMousePos = p;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
InputEvent::GetAbsMousePos() const
{
    return this->absMousePos;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InputEvent::SetNormMousePos(const Math::float2& p)
{
    this->normMousePos = p;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
InputEvent::GetNormMousePos() const
{
    return this->normMousePos;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InputEvent::SetDeviceIndex(IndexT i)
{
    this->deviceIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
InputEvent::GetDeviceIndex() const
{
    return this->deviceIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline const char*
InputEvent::TypeToString(Type t)
{
    switch (t)
    {
        case InvalidType:               return "InvalidTyp";
        case AppObtainFocus:            return "AppObtainFocus";
        case AppLoseFocus:              return "AppLoseFocus";
        case Reset:                     return "Reset";
        case KeyDown:                   return "KeyDown";
        case KeyUp:                     return "KeyUp";
        case Character:                 return "Character";
        case MouseMove:                 return "MouseMove";
        case RawMouseMove:              return "RawMouseMove";
        case MouseButtonDown:           return "MouseButtonDown";
        case MouseButtonUp:             return "MouseButtonUp";
        case MouseButtonDoubleClick:    return "MouseButtonDoubleClick";
        case MouseWheelForward:         return "MouseWheelForward";
        case MouseWheelBackward:        return "MouseWheelBackward";
        case BeginMouseCapture:         return "BeginMouseCapture";
        case EndMouseCapture:           return "EndMouseCapture";
        case BeginKeyboardCapture:      return "BeginKeyboardCapture";
        case EndKeyboardCapture:        return "EndKeyboardCapture";
        case GamePadButtonDown:         return "GamePadButtonDown";
        case GamePadButtonUp:           return "GamePadButtonUp";
        default:
            n_error("InputEvent::TypeToString(): invalid type!\n");
            return "<INVALID>";
    }
}

} // namespace Input
//------------------------------------------------------------------------------

