#pragma once
//------------------------------------------------------------------------------
/**
    @class System::Cpu
    
    Provides information about the system's CPU(s).
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/rttimacros.h"
namespace System
{
class Cpu
{
public:
    enum CoreId : uint32_t
    {
        Core0  =        0x1,
        Core1  =        0x2,
        Core2  =        0x4,
        Core3  =        0x8, // << Ordinary quad-core
        Core4  =       0x10,
        Core5  =       0x20, // << Intel extreme level
        Core6  =       0x40,            
        Core7  =       0x80,
        Core8  =      0x100,
        Core9  =      0x200,
        Core10 =      0x400,
        Core11 =      0x800,
        Core12 =     0x1000,
        Core13 =     0x2000,
        Core14 =     0x4000,
        Core15 =     0x8000,
        Core16 =    0x10000,
        Core17 =    0x20000,
        Core18 =    0x40000,
        Core19 =    0x80000,
        Core20 =   0x100000,
        Core21 =   0x200000,
        Core22 =   0x400000,
        Core23 =   0x800000,
        Core24 =  0x1000000,
        Core25 =  0x2000000,
        Core26 =  0x4000000,
        Core27 =  0x8000000,
        Core28 = 0x10000000,
        Core29 = 0x20000000,
        Core30 = 0x40000000,
        Core31 = 0x80000000  // << Threadripper gen 1 level
    };
};

__ImplementEnumBitOperators(System::Cpu::CoreId);
}
//------------------------------------------------------------------------------
    
