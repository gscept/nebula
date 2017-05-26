//------------------------------------------------------------------------------
// algorithm.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "algorithm.h"

namespace Algorithms
{

__ImplementClass(Algorithms::Algorithm, 'ALBA', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Algorithm::Algorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Algorithm::~Algorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
Algorithm::Setup()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
Algorithm::Discard()
{
	this->nameToType.Clear();
	this->functions.Clear();
	// override in subclass for any special discard behavior
}

//------------------------------------------------------------------------------
/**
*/
const std::function<void(IndexT)>
Algorithm::GetFunction(const Util::StringAtom& str)
{
	std::function<void(IndexT)> func = this->functions[str];
	return func;
}

//------------------------------------------------------------------------------
/**
*/
void
Algorithm::AddFunction(const Util::StringAtom& name, const FunctionType type, const std::function<void(IndexT)>& func)
{
	this->nameToType.Add(name, type);
	this->functions.Add(name, func);
}

//------------------------------------------------------------------------------
/**
*/
void
Algorithm::Resize()
{
	IndexT i;
	for (i = 0; i < this->renderTextures.Size(); i++)		this->renderTextures[i]->Resize();
	for (i = 0; i < this->readWriteTextures.Size(); i++)	this->readWriteTextures[i]->Resize();
}

} // namespace Base