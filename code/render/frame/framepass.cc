//------------------------------------------------------------------------------
// framepass.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framepass.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Frame
{

__ImplementClass(Frame::FramePass, 'FRPA', Frame::FrameOp);
//------------------------------------------------------------------------------
/**
*/
FramePass::FramePass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FramePass::~FramePass()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::AddSubpass(const Ptr<FrameSubpass>& subpass)
{
	this->subpasses.Append(subpass);
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::Discard()
{
	FrameOp::Discard();

	DiscardPass(this->pass);
	this->pass = 0;
	IndexT i;
	for (i = 0; i < this->subpasses.Size(); i++) this->subpasses[i]->Discard();
	this->subpasses.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::Run(const IndexT frameIndex)
{
	RenderDevice* renderDev = RenderDevice::Instance();

	// begin pass
	renderDev->BeginPass(this->pass);

	// run subpasses
	IndexT i;
	for (i = 0; i < this->subpasses.Size(); i++)
	{
		// progress to next subpass if not on first iteration
		if (i > 0) renderDev->SetToNextSubpass();

		// execute contents of this subpass
		this->subpasses[i]->Run(frameIndex);		
	}

	// end pass
	renderDev->EndPass();
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::OnWindowResized()
{
	// resize pass
	PassWindowResizeCallback(this->pass);
}

} // namespace Frame2