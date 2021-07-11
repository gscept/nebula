//------------------------------------------------------------------------------
//  mouse.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input/mouse.h"
#if __VULKAN__
namespace Input
{
__ImplementClass(Input::Mouse, 'MOUS', Base::MouseBase);
}
#elif __WIN32__
namespace Input
{
__ImplementClass(Input::Mouse, 'MOUS', Win32::Win32Mouse);
}
#else
#error "Mouse class not implemented on this platform!"
#endif
