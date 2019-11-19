//------------------------------------------------------------------------------
//  shaderfeature.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/shaderfeature.h"

namespace CoreGraphics
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ShaderFeature::ShaderFeature()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Convert a string feature mask of the form "Depth|Alpha|Skinning" into
    a unique hash.
*/
ShaderFeature::Mask
ShaderFeature::StringToMask(const String& str)
{
    Mask mask = str.HashCode();
	IndexT i = nameHash.FindIndex(mask);
	if (i == InvalidIndex)
	{
		nameHash.Add(mask, str);
	}
	return mask;
}

//------------------------------------------------------------------------------
/**
	Reverse lookup mask to retrieve the string used to create it.
*/
String
ShaderFeature::MaskToString(Mask mask)
{
	IndexT i = nameHash.FindIndex(mask);
	n_assert(i != InvalidIndex);
	return nameHash.ValueAtIndex(mask, i).AsString();
}

} // namespace CoreGraphics