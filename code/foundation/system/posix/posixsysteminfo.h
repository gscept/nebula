#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixSystemInfo
    
    Provide information about the system we're running on.
    
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "system/base/systeminfobase.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixSystemInfo : public Base::SystemInfoBase
{
public:
    /// constructor
    PosixSystemInfo();
};

} // namespace Posix
//------------------------------------------------------------------------------
