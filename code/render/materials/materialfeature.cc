//------------------------------------------------------------------------------
//  materialtype.cc
//  (C) 2014-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materialfeature.h"

using namespace Util;
namespace Materials
{

//------------------------------------------------------------------------------
/**
*/
MaterialFeature::MaterialFeature() :
	uniqueId(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
MaterialFeature::Mask 
MaterialFeature::StringToMask( const Util::String& str )
{
	Mask mask = 0;

	// first split the string mask into tokens
	Array<String> tokens = str.Tokenize("\t |");
	IndexT tokenIndex;
	for (tokenIndex = 0; tokenIndex < tokens.Size(); tokenIndex++)
	{
		Name featureName = tokens[tokenIndex];
		if (this->stringToIndex.Contains(featureName))
		{
			// feature name already has bit number assigned
			mask |= (1 << this->stringToIndex[featureName]);
		}
		else
		{
			// new feature name, assign a new bit number
			IndexT bitIndex = uniqueId++;
			if (uniqueId >= maxId)
			{
				n_error("MaterialType: more than %d unique shader features requested!", maxId);
				return 0;
			}
			this->stringToIndex.Add(featureName, bitIndex);
			this->indexToString.Append(featureName);

			// finally update the mask
			mask |= (1<<bitIndex);
		}
	}
	return mask;
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
MaterialFeature::MaskToString( Mask mask )
{
	String str;
	IndexT bitIndex;
	for (bitIndex = 0; bitIndex < maxId; bitIndex++)
	{
		if (0 != (mask & (1<<bitIndex)))
		{
			if (!str.IsEmpty())
			{
				str.Append("|");
			}
			str.Append(this->indexToString[bitIndex].Value());
		}
	}
	return str;
}
} // namespace Materials