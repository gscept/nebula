//------------------------------------------------------------------------------
//  barrier.cc
//  (C) 2012-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/barrier.h"
#if __VULKAN__
namespace CoreGraphics
{
	__ImplementClass(CoreGraphics::Barrier, 'BARR', Vulkan::VkBarrier);
}
#else
namespace CoreGraphics
{
	__ImplementClass(CoreGraphics::Barrier, 'BARR', Barrier::BarrierBase);
}
#endif