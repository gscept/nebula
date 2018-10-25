#pragma once
//------------------------------------------------------------------------------
/**
	Performs an algorithm used for computations.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "algorithm/algorithm.h"
namespace Frame
{
class FrameComputeAlgorithm : public FrameOp
{
public:
	/// constructor
	FrameComputeAlgorithm();
	/// destructor
	virtual ~FrameComputeAlgorithm();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);
		void Discard();

		std::function<void(IndexT)> func;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);

	Util::StringAtom funcName;
	Algorithms::Algorithm* alg;
private:
};

} // namespace Frame2