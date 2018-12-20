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
ShaderFeature::ShaderFeature() :
    uniqueId(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Convert a string feature mask of the form "Depth|Alpha|Skinning" into
    a binary feature bit mask. Bit numbers are assigned dynamically, and may
    be different each time the application is run!
*/
ShaderFeature::Mask
ShaderFeature::StringToMask(const String& str)
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
                n_error("ShaderFeature: more than %d unique shader features requested!", maxId);
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
    Convert a feature bit mask into a string feature mask. Note that this
    works only for bit masks which have been created by StringToMask
    before (since feature bit indices are assigned dynamically and
    may be different every time the application is run).
*/
String
ShaderFeature::MaskToString(Mask mask)
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

} // namespace CoreGraphics