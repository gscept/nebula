//------------------------------------------------------------------------------
//  platform.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "platform.h"

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
System::ByteOrder::Type
Platform::GetPlatformByteOrder(Platform::Code c)
{
    switch (c)
    {
        case Win32:
        case Linux:
            return System::ByteOrder::LittleEndian;
        default:        
            return System::ByteOrder::BigEndian;
    }
}

//------------------------------------------------------------------------------
/**
*/
Platform::Code
Platform::FromString(const Util::String& str)
{
    if (str == "win32")   return Win32;    
    else if (str == "linux")     return Linux;
    else
    {
        n_error("Platform::FromString(): invalid platform string '%s'!", str.AsCharPtr());
        return InvalidPlatform;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::String
Platform::ToString(Platform::Code code)
{
    switch (code)
    {        
        case Win32:     return "win32";     
        case Linux:       return "linux";
        default:
            n_error("Platform::ToString(): invalid platform code!");
            return "";
    }
}

} // namespace ToolkitUtil
