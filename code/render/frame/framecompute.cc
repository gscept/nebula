//------------------------------------------------------------------------------
// framecompute.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
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
FrameCompute::FrameCompute()
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
	this->program = ShaderProgramId::Invalid();

	DestroyResourceTable(this->resourceTable);
	IndexT i;
	for (i = 0; i < this->constantBuffers.Size(); i++)
		DestroyConstantBuffer(this->constantBuffers.ValueAtIndex(i));
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameCompute::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->program = this->program;
	ret->resourceTable = this->resourceTable;
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

	CoreGraphics::SetShaderProgram(this->program);

	// compute
	CoreGraphics::SetResourceTable(this->resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
	CoreGraphics::Compute(this->x, this->y, this->z);
}

} // namespace Frame2