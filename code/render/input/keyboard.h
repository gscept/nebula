#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::Keyboard
    
    An input handler which represents a keyboard for polling.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __WIN32__ || __LINUX__
#include "input/base/keyboardbase.h"
namespace Input
{
class Keyboard : public Base::KeyboardBase
{
    __DeclareClass(Keyboard);
};
}
#elif __XBOX360__
#include "input/xbox360/xbox360keyboard.h"
namespace Input
{
class Keyboard : public Xbox360::Xbox360Keyboard
{
    __DeclareClass(Keyboard);
};
}
#elif __WII__
#include "input/wii/wiikeyboard.h"
namespace Input
{
class Keyboard : public Wii::WiiKeyboard
{
    __DeclareClass(Keyboard);
};
}
#elif __PS3__
#include "input/base/keyboardbase.h"
namespace Input
{
class Keyboard : public Base::KeyboardBase
{
    __DeclareClass(Keyboard);
};
}
#else
#error "Keyboard class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

