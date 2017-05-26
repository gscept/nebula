#pragma once
//------------------------------------------------------------------------------
/**
	Implements a GPU-side event in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <vulkan/vulkan.h>
#include "coregraphics/base/eventbase.h"
namespace Vulkan
{
class VkCmdEvent : public Base::EventBase
{
	__DeclareClass(VkCmdEvent);
public:
	/// constructor
	VkCmdEvent();
	/// destructor
	virtual ~VkCmdEvent();

	/// setup event
	void Setup();

	/// signal
	void Signal();
	/// wait for event to be set
	void Wait();
	/// reset event
	void Reset();
private:

	VkEvent event;
};
} // namespace Vulkan