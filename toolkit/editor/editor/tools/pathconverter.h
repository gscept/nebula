#pragma once
//------------------------------------------------------------------------------
/**
    Tool used to convert paths from export to work

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"
#include "io/uri.h"
namespace Editor
{

struct PathConverterCreateInfo
{
};

/// Setup path converter
void CreatePathConverter(const PathConverterCreateInfo& info);

struct PathConverter
{
    /// Convert a full path to a Nebula URL
    static Util::String MapToCompactPath(const Util::String& path);
    /// Convert to only display asset name
    static Util::String StripAssetName(const Util::String& path);
    /// Convert an export relative path to it's work correspondent
    static Util::String MapToWork(const Util::String& path);
};

} // namespace Editor
