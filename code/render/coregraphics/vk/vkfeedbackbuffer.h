#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan feedback vertex buffer.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/feedbackbufferbase.h"
namespace Vulkan
{
class VkFeedbackBuffer : public Base::FeedbackBufferBase
{
	__DeclareClass(VkFeedbackBuffer);
public:
	/// constructor
	VkFeedbackBuffer();
	/// destructor
	virtual ~VkFeedbackBuffer();
private:
};
} // namespace Vulkan