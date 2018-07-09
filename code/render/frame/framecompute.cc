//------------------------------------------------------------------------------
// framecompute.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framecompute.h"
#include "coregraphics/graphicsdevice.h"

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
	ShaderDestroyState(this->state);
	this->program = ShaderProgramId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameCompute::AllocCompiled(Memory::ChunkAllocator<0xFFFF>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->program = this->program;
	ret->state = this->state;
	ret->x = this->x;
	ret->y = this->y;
	ret->z = this->z;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCompute::CompiledImpl::Run(const IndexT frameIndex)
{
	n_assert(this->program != ShaderProgramId::Invalid());
	n_assert(this->state != ShaderStateId::Invalid());

	CoreGraphics::SetShaderProgram(this->program);
	CoreGraphics::SetShaderState(this->state);

	// compute
	CoreGraphics::Compute(this->x, this->y, this->z);
}

} // namespace Frame2