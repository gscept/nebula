#pragma once
//------------------------------------------------------------------------------
/**
	A subpass batch performs batch rendering of geometry.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/batchgroup.h"
namespace Frame
{
class FrameSubpassBatch : public FrameOp
{
public:
	/// constructor
	FrameSubpassBatch();
	/// destructor
	virtual ~FrameSubpassBatch();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		CoreGraphics::BatchGroup::Code batch;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);
	CoreGraphics::BatchGroup::Code batch;
};

} // namespace Frame2