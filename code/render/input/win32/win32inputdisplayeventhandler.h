#pragma once
#ifndef WIN32_WIN32INPUTDISPLAYEVENTHANDLER_H
#define WIN32_WIN32INPUTDISPLAYEVENTHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Win32::Win32InputDisplayEventHandler
  
    Translates DisplayEvents that are relevant for the input system
    into InputEvents.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/threadsafedisplayeventhandler.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32InputDisplayEventHandler : public CoreGraphics::ThreadSafeDisplayEventHandler
{
    __DeclareClass(Win32InputDisplayEventHandler);
public:
    /// called when an event happens
    virtual bool HandleEvent(const CoreGraphics::DisplayEvent& event);
};

} // namespace Win32
//------------------------------------------------------------------------------
#endif
