#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::DisplayEvent
    
    Display events are sent by the display device to registered display
    event handlers.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "input/key.h"
#include "input/char.h"
#include "input/mousebutton.h"
#include "math/float2.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class DisplayEvent
{
public:
    /// event codes
    enum Code
    {
        InvalidCode,
        WindowOpen,
        WindowClose,
		WindowReopen,
        CloseRequested,
        WindowMinimized,
        WindowRestored,
        ToggleFullscreenWindowed,
		WindowResized,
        SetCursor,
        Paint,
        SetFocus,
        KillFocus,
        KeyDown,
        KeyUp,
        Character,
        MouseMove,
        MouseButtonDown,
        MouseButtonUp,
        MouseButtonDoubleClick,
        MouseWheelForward,
        MouseWheelBackward,
    };

    /// default constructor
    DisplayEvent();
    /// constructor with event code
    DisplayEvent(Code c);
	/// constructor with event code and window id
	DisplayEvent(Code c, IndexT wnd);
    /// constructor with event code and mouse pos
    DisplayEvent(Code c, const Math::float2& absPos, const Math::float2& normPos);
    /// constructor with key code
    DisplayEvent(Code c, Input::Key::Code k);
    /// constructor with character
    DisplayEvent(Code c, Input::Char chr);
    /// constructor with mouse button and mouse pos
    DisplayEvent(Code c, Input::MouseButton::Code b, const Math::float2& absPos, const Math::float2& normPos);

    /// get event code
    Code GetEventCode() const;
	/// get window id
	IndexT GetWindowId() const;
    /// get absolute mouse pos (in pixels)
    const Math::float2& GetAbsMousePos() const;
    /// get normalized mouse pos (from 0.0 to 1.0)
    const Math::float2& GetNormMousePos() const;
    /// get key code
    Input::Key::Code GetKey() const;
    /// get character code
    Input::Char GetChar() const;
    /// get mouse button code
    Input::MouseButton::Code GetMouseButton() const;

private:
	IndexT windowId;
    Code code;
    Math::float2 absMousePos;
    Math::float2 normMousePos;
    Input::Key::Code keyCode;
    Input::Char charCode;
    Input::MouseButton::Code mouseButtonCode;
};

//------------------------------------------------------------------------------
/**
*/
inline
DisplayEvent::DisplayEvent() :
    code(InvalidCode),
    absMousePos(0.0f, 0.0f),
    normMousePos(0.0f, 0.0f),
    keyCode(Input::Key::InvalidKey),
    charCode(0),
    mouseButtonCode(Input::MouseButton::InvalidMouseButton)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
DisplayEvent::DisplayEvent(Code c) :
    code(c),
	windowId(InvalidIndex),
    absMousePos(0.0f, 0.0f),
    normMousePos(0.0f, 0.0f),
    keyCode(Input::Key::InvalidKey),
    charCode(0),
    mouseButtonCode(Input::MouseButton::InvalidMouseButton)
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
inline
DisplayEvent::DisplayEvent(Code c, IndexT wnd) :
	code(c),
	windowId(wnd),
	absMousePos(0.0f, 0.0f),
	normMousePos(0.0f, 0.0f),
	keyCode(Input::Key::InvalidKey),
	charCode(0),
	mouseButtonCode(Input::MouseButton::InvalidMouseButton)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline
DisplayEvent::DisplayEvent(Code c, const Math::float2& absPos, const Math::float2& normPos) :
    code(c),
	windowId(InvalidIndex),
    absMousePos(absPos),
    normMousePos(normPos),
    keyCode(Input::Key::InvalidKey),
    charCode(0),
    mouseButtonCode(Input::MouseButton::InvalidMouseButton)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
DisplayEvent::DisplayEvent(Code c, Input::Key::Code k) :
    code(c),
	windowId(InvalidIndex),
    absMousePos(0.0f, 0.0f),
    normMousePos(0.0f, 0.0f),
    keyCode(k),
    charCode(0),
    mouseButtonCode(Input::MouseButton::InvalidMouseButton)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
DisplayEvent::DisplayEvent(Code c, Input::Char chr) :
    code(c),
	windowId(InvalidIndex),
    absMousePos(0.0f, 0.0f),
    normMousePos(0.0f, 0.0f),
    keyCode(Input::Key::InvalidKey),
    charCode(chr),
    mouseButtonCode(Input::MouseButton::InvalidMouseButton)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
DisplayEvent::DisplayEvent(Code c, Input::MouseButton::Code b, const Math::float2& absPos, const Math::float2& normPos) :
    code(c),
	windowId(InvalidIndex),
    absMousePos(absPos),
    normMousePos(normPos),
    keyCode(Input::Key::InvalidKey),
    charCode(0),
    mouseButtonCode(b)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline DisplayEvent::Code
DisplayEvent::GetEventCode() const
{
    return this->code;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
DisplayEvent::GetWindowId() const
{
	return this->windowId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
DisplayEvent::GetAbsMousePos() const
{
    return this->absMousePos;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
DisplayEvent::GetNormMousePos() const
{
    return this->normMousePos;
}

//------------------------------------------------------------------------------
/**
*/
inline Input::Key::Code
DisplayEvent::GetKey() const
{
    return this->keyCode;
}

//------------------------------------------------------------------------------
/**
*/
inline Input::Char
DisplayEvent::GetChar() const
{
    return this->charCode;
}

//------------------------------------------------------------------------------
/**
*/
inline Input::MouseButton::Code
DisplayEvent::GetMouseButton() const
{
    return this->mouseButtonCode;
}

} // namespace CoreGraphics
//------------------------------------------------------------------------------

