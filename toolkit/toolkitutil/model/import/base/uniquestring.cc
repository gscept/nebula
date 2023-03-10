//------------------------------------------------------------------------------
//  @file uniquestring.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "uniquestring.h"
#include "util/guid.h"
namespace ToolkitUtil
{

Util::Dictionary<IndexT, SizeT> UniqueString::Lookup;
//------------------------------------------------------------------------------
/**
*/
Util::String
ToolkitUtil::UniqueString::New(const Util::String& str)
{
    // If string is empty, return a randomly generated GUID
    if (str.IsEmpty())
    {
        Util::Guid guid;
        guid.Generate();
        return guid.AsString();
    }

    auto hash = str.HashCode();
    IndexT i = UniqueString::Lookup.FindIndex(hash);

    if (i == InvalidIndex)
    {
        UniqueString::Lookup.Add(hash, 0);
        return str;
    }
    else
    {
        Util::String ret;
        SizeT& value = UniqueString::Lookup.ValueAtIndex(i);
        ret.Format("%s_%d", str.AsCharPtr(), value);
        value++;
        return ret;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::String
UniqueString::New(const char* str)
{
    return UniqueString::New(Util::String(str));
}

} // namespace ToolkitUtil