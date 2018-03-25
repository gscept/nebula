#pragma once
//------------------------------------------------------------------------------
/**
	A subpass is a subset of attachments declared by pass, and if depth should be used.
	
	Subpasses can be dependent on each other, and can declare which attachments in the pass
	should be passed between them. 

	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "math/rectangle.h"
namespace Frame
{
class FrameSubpass : public FrameOp
{
public:
	/// constructor
	FrameSubpass();
	/// destructor
	virtual ~FrameSubpass();

	/// add frame operation
	void AddOp(Frame::FrameOp* op);

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);

	/// add viewport
	void AddViewport(const Math::rectangle<int>& rect);
	/// add viewport
	void AddScissor(const Math::rectangle<int>& rect);
private:
	Util::Array<Frame::FrameOp*> ops;
	Util::Array<Math::rectangle<int>> viewports;
	Util::Array<Math::rectangle<int>> scissors;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpass::AddViewport(const Math::rectangle<int>& rect)
{
	this->viewports.Append(rect);
}

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpass::AddScissor(const Math::rectangle<int>& rect)
{
	this->scissors.Append(rect);
}
} // namespace Frame2