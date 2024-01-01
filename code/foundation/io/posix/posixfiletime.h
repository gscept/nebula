#pragma once
#ifndef POSIX_POSIXFILETIME_H
#define POSIX_POSIXFILETIME_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixFileTime
    
    Implements a Posix-specific file-access time stamp.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file    
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixFileTime
{
public:
    /// constructor
    PosixFileTime();
    /// construct from string
    PosixFileTime(const Util::String& str);
    /// operator ==
    friend bool operator==(const PosixFileTime& a, const PosixFileTime& b);
    /// operator !=
    friend bool operator!=(const PosixFileTime& a, const PosixFileTime& b);
    /// operator >
    friend bool operator>(const PosixFileTime& a, const PosixFileTime& b);
    /// operator <
    friend bool operator<(const PosixFileTime& a, const PosixFileTime& b);
    
    /// get high bits of internal time structure (resolution is platform dependent!)
    uint GetHighBits() const;
    /// get log bits of internal time structure (resolution is platform dependent!)
    uint GetLowBits() const;
    /// set both parts of time (platform dependent)
    void SetBits(uint lowbits, uint highbits);

    /// convert to string
    Util::String AsString() const;

private:
    friend class PosixFSWrapper;
    friend class PosixCalendarTime;

    timespec time;
};

//------------------------------------------------------------------------------
/**
*/
inline
PosixFileTime::PosixFileTime()
{
    time.tv_sec = 0;
    time.tv_nsec = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
PosixFileTime::GetHighBits() const
{
    return this->time.tv_sec;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
PosixFileTime::GetLowBits() const
{
    return this->time.tv_nsec;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PosixFileTime::SetBits(uint lowbits, uint highbits)
{
    this->time.tv_nsec = lowbits;
    this->time.tv_sec = highbits;
}
//------------------------------------------------------------------------------
/**
*/
inline bool 
operator==(const PosixFileTime& a, const PosixFileTime& b)
{
    return (a.time.tv_sec == b.time.tv_sec) && (a.time.tv_nsec == b.time.tv_nsec);
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
operator!=(const PosixFileTime& a, const PosixFileTime& b)
{
    return (a.time.tv_sec != b.time.tv_sec) || (a.time.tv_nsec != b.time.tv_nsec);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator>(const PosixFileTime& a, const PosixFileTime& b)
{
    if (a.time.tv_sec < b.time.tv_sec) return false;
    if (a.time.tv_sec > b.time.tv_sec) return true;
    if (a.time.tv_nsec > b.time.tv_nsec) return true;
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator <(const PosixFileTime& a, const PosixFileTime& b)
{
    if (a.time.tv_sec > b.time.tv_sec) return false;
    if (a.time.tv_sec < b.time.tv_sec) return true;
    if (a.time.tv_nsec < b.time.tv_nsec) return true;
    return false;
}

}; // namespace Posix
//------------------------------------------------------------------------------
#endif

