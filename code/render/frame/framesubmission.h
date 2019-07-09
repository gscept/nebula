#pragma once
//------------------------------------------------------------------------------
/**
	Handles frame submissions start and end

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
namespace Frame
{

class FrameSubmission : public FrameOp
{
public:
	/// constructor
	FrameSubmission();
	/// destructor
	~FrameSubmission();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		CoreGraphicsQueueType queue;
		CoreGraphicsQueueType waitQueue;
		CoreGraphics::BarrierId* endOfFrameBarrier;
		char startOrEnd;
#if NEBULA_GRAPHICS_DEBUG
		Util::StringAtom name;
#endif
	};

	/// allocate new instance
	FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

	CoreGraphicsQueueType queue;
	CoreGraphicsQueueType waitQueue;
	CoreGraphics::BarrierId* endOfFrameBarrier;
	char startOrEnd; // 0 if start, 1 if end
};

} // namespace Frame
