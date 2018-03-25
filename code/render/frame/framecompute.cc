//------------------------------------------------------------------------------
// framecompute.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framecompute.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameCompute::FrameCompute() :
	state(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameCompute::~FrameCompute()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCompute::Discard()
{
	FrameOp::Discard();
	this->program = ShaderProgramId::Invalid();
	this->state = ShaderStateId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCompute::Run(const IndexT frameIndex)
{
	n_assert(this->program != ShaderProgramId::Invalid());
	n_assert(this->state != ShaderStateId::Invalid());

	RenderDevice* dev = RenderDevice::Instance();

	ShaderProgramBind(this->program);
	ShaderStateApply(this->state);

	// compute
	dev->Compute(this->x, this->y, this->z);
}

} // namespace Frame2