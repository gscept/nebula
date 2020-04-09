#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::GamePad
  
    An input handler which represents a game pad.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/ 
#if __VULKAN__
#include "input/base/gamepadbase.h"
namespace Input
{
	class GamePad : public Base::GamePadBase
	{
		__DeclareClass(GamePad);
	};
}
#elif __WIN32__
#include "input/xinput/xinputgamepad.h"
namespace Input
{
class GamePad : public XInput::XInputGamePad
{
    __DeclareClass(GamePad);
};
}
#else
#error "GamePad class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

