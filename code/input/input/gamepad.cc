//------------------------------------------------------------------------------
//  gamepad.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input/gamepad.h"
#if __VULKAN__
namespace Input
{
__ImplementClass(Input::GamePad, 'GMPD', Base::GamePadBase);
}
#elif __WIN32__
namespace Input
{
__ImplementClass(Input::GamePad, 'GMPD', XInput::XInputGamePad);
}
#else
#error "GamePad class not implemented on this platform!"
#endif
