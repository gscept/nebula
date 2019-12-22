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
	Util::Array<Util::String> features = str.Tokenize("|");
	Mask mask = 0;
	IndexT i;
	for (i = 0; i < features.Size(); i++)
	{
		mask += features[i].HashCode();
	}
	n_assert(mask != 0);
	if (!nameHash.Contains(mask))
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