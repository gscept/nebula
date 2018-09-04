#pragma once
//------------------------------------------------------------------------------
/**
	Executes compute shader.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/shader.h"
namespace Frame
{
class FrameCompute : public FrameOp
{
public:
	/// constructor
	FrameCompute();
	/// destructor
	virtual ~FrameCompute();

	/// discard
	void Discard();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		CoreGraphics::ShaderProgramId program;
		CoreGraphics::ShaderStateId state;
		SizeT x, y, z;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);

	CoreGraphics::ShaderProgramId program;
	CoreGraphics::ShaderStateId state;
	SizeT x, y, z;
};

} // namespace Frame2