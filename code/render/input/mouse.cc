//------------------------------------------------------------------------------
//  mouse.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "input/mouse.h"
#if (__DX9__ || __DX11__)
namespace Input
{
__ImplementClass(Input::Mouse, 'MOUS', Win32::Win32Mouse);
}
#elif (__OGL4__ || __VULKAN__)
namespace Input
{
__ImplementClass(Input::Mouse, 'MOUS', Base::MouseBase);
}
#else
#error "Mouse class not implemented on this platform!"
#endif
