#pragma once
//------------------------------------------------------------------------------
/**
	This is just a container
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/barrier.h"
namespace Frame
{
class FrameBarrier : public FrameOp
{
public:
	/// constructor
	FrameBarrier();
	/// destructor
	virtual ~FrameBarrier();

	struct CompiledImpl : public FrameOp::Compiled
	{
#if NEBULA_GRAPHICS_DEBUG
		Util::StringAtom name;
#endif
		/// running does nothing
		void Run(const IndexT frameIndex) override;
		void Discard();
	};

private:
	virtual FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);
};

} // namespace Frame2