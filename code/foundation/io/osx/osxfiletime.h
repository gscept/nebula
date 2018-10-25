#pragma once
//------------------------------------------------------------------------------
/**
    @class OSX::OSXFileTime

    Wraps file-system related timestamps on OSX.
 
    (C) 2010 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file    
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace OSX
{
class OSXFileTime
{
public:
    /// constructor
    OSXFileTime();
    /// operator ==
    friend bool operator==(const OSXFileTime& a, const OSXFileTime& b);
    /// operator !=
    friend bool operator!=(const OSXFileTime& a, const OSXFileTime& b);
    /// operator >
    friend bool operator>(const OSXFileTime& a, const OSXFileTime& b);
    /// operator <
    friend bool operator<(const OSXFileTime& a, const OSXFileTime& b);
    
    time_t fileTime;
};
    
//------------------------------------------------------------------------------
/**
 */
inline
OSXFileTime::OSXFileTime() :
    fileTime(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
 */
inline bool 
operator==(const OSXFileTime& a, const OSXFileTime& b)
{
    return (a.fileTime == b.fileTime);
}

//------------------------------------------------------------------------------
/**
 */
inline bool 
operator!=(const OSXFileTime& a, const OSXFileTime& b)
{
    return (a.fileTime != b.fileTime);
}

//------------------------------------------------------------------------------
/**
 */
inline bool
operator>(const OSXFileTime& a, const OSXFileTime& b)
{
    return (a.fileTime > b.fileTime);
}

//------------------------------------------------------------------------------
/**
 */
inline bool
operator <(const OSXFileTime& a, const OSXFileTime& b)
{
    return (a.fileTime < b.fileTime);
}
    
} // namespace OSX
