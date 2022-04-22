#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::Platform
    
    Platform identifiers (Xbox360, Wii, etc...).
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "system/byteorder.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class Platform
{
public:
    /// platform enum
    enum Code
    {        
        Win32,
        Linux,

        InvalidPlatform,
    };
    /// get byte order for a platform
    static System::ByteOrder::Type GetPlatformByteOrder(Code c);
    /// convert from string
    static Code FromString(const Util::String& str);
    /// convert to string
    static Util::String ToString(Code c);
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    