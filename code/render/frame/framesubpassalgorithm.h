#pragma once
//------------------------------------------------------------------------------
/**
	Performs an algorithm used in a subpass, for rendering
	
	(C) 2016 Individual contributors, see AUTHORS file
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
	/// run operation
	void Run(const IndexT frameIndex);

	Util::StringAtom funcName;
	Algorithms::Algorithm* alg;
private:
	
	std::function<void(IndexT)> func;
};

} // namespace Frame2