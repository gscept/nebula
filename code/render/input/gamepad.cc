//------------------------------------------------------------------------------
//  gamepad.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "input/gamepad.h"
#if (__OGL4__ || __VULKAN__)
namespace Input
{
__ImplementClass(Input::GamePad, 'GMPD', Base::GamePadBase);
}
#elif (__DX9__ ||__DX11__ || __XBOX360__||WIN32)
namespace Input
{
__ImplementClass(Input::GamePad, 'GMPD', XInput::XInputGamePad);
}
#else
#error "GamePad class not implemented on this platform!"
#endif
