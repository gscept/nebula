//------------------------------------------------------------------------------
// algorithm.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "algorithm.h"

namespace Algorithms
{

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
	FIXME: implement resize
*/
void
Algorithm::Resize()
{
	IndexT i;
	for (i = 0; i < this->renderTextures.Size(); i++)		RenderTextureWindowResized(this->renderTextures[i]);
	for (i = 0; i < this->readWriteTextures.Size(); i++)	ShaderRWTextureWindowResized(this->readWriteTextures[i]);
}

} // namespace Base