#pragma once
//------------------------------------------------------------------------------
/**
	Implements a signal for swapping backbuffers for window targets
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/rendertexture.h"
namespace Frame
{
class FrameSwapbuffers : public Frame::FrameOp
{
public:
	/// constructor
	FrameSwapbuffers();
	/// destructor
	virtual ~FrameSwapbuffers();

	/// discard operation
	void Discard();
	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		CoreGraphics::RenderTextureId tex;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);

	CoreGraphics::RenderTextureId tex;
};

} // namespace Frame2