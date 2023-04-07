#pragma once
//------------------------------------------------------------------------------
/**
    A UniqueString is a Util::String of which there can be only one.

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"
#include "util/dictionary.h"
namespace ToolkitUtil
{
class UniqueString
{
public:
    /// Delete constructor
    UniqueString() = delete;

    /// Uniqueify string
    static Util::String New(const Util::String& str);
    /// Uniqueify string
    static Util::String New(const char* str);

    /// Reset strings
    static void Reset();

private:
    static Util::Dictionary<IndexT, SizeT> Lookup;
};
} // namespace ToolkitUtil
