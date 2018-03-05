#pragma once
//------------------------------------------------------------------------------
/**
	Implements a barrier between frame operations.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/barrier.h"
namespace Frame
{
class FrameBarrier : public FrameOp
{
	__DeclareClass(FrameBarrier);
public:
	/// constructor
	FrameBarrier();
	/// destructor
	virtual ~FrameBarrier();

	/// run operation
	void Run(const IndexT frameIndex);

	CoreGraphics::BarrierId barrier;
};

} // namespace Frame2