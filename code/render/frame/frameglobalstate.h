#pragma once
//------------------------------------------------------------------------------
/**
	Implements a global state which is initialized at the beginning of the frame.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/shader.h"
namespace Frame
{
class FrameGlobalState : public FrameOp
{
public:
	/// constructor
	FrameGlobalState();
	/// destructor
	virtual ~FrameGlobalState();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		CoreGraphics::ShaderStateId state;
		Util::Array<CoreGraphics::ShaderConstantId> constants;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);

	CoreGraphics::ShaderStateId state;
	Util::Array<CoreGraphics::ShaderConstantId> constants;
};

} // namespace Frame2