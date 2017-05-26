#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::Barrier

	Implements a GPU local barrier

	(C) 2012-2015 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/vkbarrier.h"
namespace CoreGraphics
{
	class Barrier : public Vulkan::VkBarrier
	{
		__DeclareClass(Barrier);
	};
}
#else
#include "coregraphics/base/barrierbase.h"
namespace CoreGraphics
{
	class Barrier : public Base::BarrierBase
	{
		__DeclareClass(Barrier);
	};
}
#endif