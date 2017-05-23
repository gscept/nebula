#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::SystemInfoBase
    
    Provide runtime-system-information.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Base
{
class SystemInfoBase
{
public:
    /// host platforms
    enum Platform
    {
        Win32,
        Xbox360,
        Wii,
        PS3,
	Linux,
        
        UnknownPlatform,        
    };
    
    /// CPU types
    enum CpuType
    {
        X86_32,             // any 32-bit x86
        X86_64,             // any 64-bit x86
        PowerPC_Xbox360,    // Xbox 360 CPU
        PowerPC_PS3,        // PS3 Cell CPU
        PowerPC_Wii,        // Wii CPU

        UnknownCpuType,
    };

    /// constructor
    SystemInfoBase();

    /// get host platform
    Platform GetPlatform() const;
    /// get cpu type
    CpuType GetCpuType() const;
    /// get number of processors
    SizeT GetNumCpuCores() const;
    /// get page size
    SizeT GetPageSize() const;
    
    /// convert platform to string
    static Util::String PlatformAsString(Platform p);
    /// convert CpuType to string
    static Util::String CpuTypeAsString(CpuType cpu);

protected:
    Platform platform;
    CpuType cpuType;
    SizeT numCpuCores;
    SizeT pageSize;
};

//------------------------------------------------------------------------------
/**
*/
inline SystemInfoBase::Platform
SystemInfoBase::GetPlatform() const
{
    return this->platform;
}

//------------------------------------------------------------------------------
/**
*/
inline SystemInfoBase::CpuType
SystemInfoBase::GetCpuType() const
{
    return this->cpuType;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
SystemInfoBase::GetNumCpuCores() const
{
    return this->numCpuCores;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
SystemInfoBase::GetPageSize() const
{
    return this->pageSize;
}

} // namespace Base
//------------------------------------------------------------------------------
    
    
