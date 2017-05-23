#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Cpu
    
    CPU related definitions for the Win32 platform.  
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32Cpu
{
public:
    typedef DWORD CoreId;
    
    /// core identifiers, under Win32, we basically don't care...
    static const CoreId InvalidCoreId       = 0xffffffff;
    static const CoreId MainThreadCore      = 0;
    static const CoreId IoThreadCore        = 2;
    static const CoreId RenderThreadCore    = 1;
    static const CoreId AudioThreadCore     = 3;
    static const CoreId MiscThreadCore      = 4;
    static const CoreId NetworkThreadCore   = 5;

    static const CoreId JobThreadFirstCore  = 6;
	static const CoreId RenderThreadFirstCore = 16;
};

} // namespace Win32    
//------------------------------------------------------------------------------
