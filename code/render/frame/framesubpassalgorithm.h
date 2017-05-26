#pragma once
//------------------------------------------------------------------------------
/**
	Performs an algorithm used in a subpass, for rendering
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "algorithm/algorithm.h"
namespace Frame2
{
class FrameSubpassAlgorithm : public FrameOp
{
	__DeclareClass(FrameSubpassAlgorithm);
public:
	/// constructor
	FrameSubpassAlgorithm();
	/// destructor
	virtual ~FrameSubpassAlgorithm();

	/// set algorithm
	void SetAlgorithm(const Ptr<Algorithms::Algorithm>& alg);
	/// add function to run
	void SetFunction(const Util::StringAtom& func);
	
	/// setup operation
	void Setup();
	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);
private:
	Util::StringAtom funcName;
	Ptr<Algorithms::Algorithm> alg;
	std::function<void(IndexT)> func;
};


//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpassAlgorithm::SetAlgorithm(const Ptr<Algorithms::Algorithm>& alg)
{
	this->alg = alg;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpassAlgorithm::SetFunction(const Util::StringAtom& func)
{
	n_assert(this->alg.isvalid());
	n_assert(this->alg->GetFunctionType(func) == Algorithms::Algorithm::Graphics);
	this->funcName = func;
}
} // namespace Frame2