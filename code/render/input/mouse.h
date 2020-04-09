#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::Mouse
    
    An input handler which represents a mouse for polling.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "input/base/mousebase.h"
namespace Input
{
class Mouse : public Base::MouseBase
{
    __DeclareClass(Mouse);
};
}
#elif __WIN32__
#include "input/win32/win32mouse.h"
namespace Input
{
class Mouse : public Win32::Win32Mouse
{
    __DeclareClass(Mouse);
};
}
#else
#error "Mouse class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

