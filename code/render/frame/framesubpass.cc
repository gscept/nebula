//------------------------------------------------------------------------------
// framesubpass.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framesubpass.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpass::FrameSubpass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpass::~FrameSubpass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::AddOp(Frame::FrameOp* op)
{
	this->ops.Append(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::Discard()
{
	FrameOp::Discard();

	IndexT i;
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Discard();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::Run(const IndexT frameIndex)
{
	RenderDevice* renderDev = RenderDevice::Instance();
	IndexT i;

	// bind scissors and viewports, if any
	for (i = 0; i < this->viewports.Size(); i++) renderDev->SetViewport(this->viewports[i], i);
	for (i = 0; i < this->scissors.Size(); i++) renderDev->SetScissorRect(this->scissors[i], i);

	// run ops
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Run(frameIndex);
	}
}

} // namespace Frame2