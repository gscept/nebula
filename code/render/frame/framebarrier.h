#pragma once
//------------------------------------------------------------------------------
/**
	Implements a barrier between frame operations.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/barrier.h"
namespace Frame2
{
class FrameBarrier : public FrameOp
{
	__DeclareClass(FrameBarrier);
public:
	/// constructor
	FrameBarrier();
	/// destructor
	virtual ~FrameBarrier();

	/// set barrier
	void SetBarrier(const Ptr<CoreGraphics::Barrier>& barrier);
	/// run operation
	void Run(const IndexT frameIndex);
private:

	Ptr<CoreGraphics::Barrier> barrier;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameBarrier::SetBarrier(const Ptr<CoreGraphics::Barrier>& barrier)
{
	this->barrier = barrier;
}

} // namespace Frame2