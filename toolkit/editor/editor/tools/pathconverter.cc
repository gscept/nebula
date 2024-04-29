//------------------------------------------------------------------------------
//  @file pathconverter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "pathconverter.h"
#include "io/assignregistry.h"
namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
void 
CreatePathConverter(const PathConverterCreateInfo& info)
{
}

//------------------------------------------------------------------------------
/**
*/
Util::String
GetRelativeFolder(const Util::String& path, const char** assign)
{
    Util::String extension = path.GetFileExtension();
    if (extension == "dds")
        *assign = "tex";
    else if (extension == "nvx")
        *assign = "msh";
    else if (extension == "n3")
        *assign = "mdl";
    else if (extension == "nsk")
        *assign = "ske";
    else if (extension == "nax")
        *assign = "ani";
    else if (extension == "sur")
        *assign = "sur";

    IO::URI assignPath = IO::AssignRegistry::Instance()->GetAssign(*assign);
    Util::String assignRoot = assignPath.AsString();
    return path.ExtractToEnd(assignRoot.Length() + 1);
}

//------------------------------------------------------------------------------
/**
*/
Util::String
PathConverter::MapToCompactPath(const Util::String& path)
{
    const char* assign = nullptr;
    Util::String tail = GetRelativeFolder(path, &assign);
    return Util::Format("%s:%s", assign, tail.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
PathConverter::StripAssetName(const Util::String& path)
{
    const char* assign = nullptr;
    Util::String tail = GetRelativeFolder(path, &assign);
    tail.StripFileExtension();
    return tail;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
PathConverter::MapToWork(const Util::String& path)
{
    const char* assign = nullptr;
    Util::String tail = GetRelativeFolder(path, &assign);
    return Util::Format("work:assets/%s", tail.AsCharPtr());
}

} // namespace Editor
