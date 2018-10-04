#pragma once
//------------------------------------------------------------------------------
/**
	Performs an algorithm used in a subpass, for rendering
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "algorithm/algorithm.h"
namespace Frame
{
class FrameSubpassAlgorithm : public FrameOp
{
public:
	/// constructor
	FrameSubpassAlgorithm();
	/// destructor
	virtual ~FrameSubpassAlgorithm();

	/// setup operation
	void Setup();
	/// discard operation
	void Discard();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		std::function<void(IndexT)> func;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);

	Util::StringAtom funcName;
	Algorithms::Algorithm* alg;
private:
	
	std::function<void(IndexT)> func;
};

} // namespace Frame2