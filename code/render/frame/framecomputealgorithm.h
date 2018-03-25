#pragma once
//------------------------------------------------------------------------------
/**
	Performs an algorithm used for computations.
	
	(C) 2016 Individual contributors, see AUTHORS file
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

	/// setup operation
	void Setup();
	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);

	Util::StringAtom funcName;
	Algorithms::Algorithm* alg;
private:
	
	std::function<void(IndexT)> func;
};

} // namespace Frame2