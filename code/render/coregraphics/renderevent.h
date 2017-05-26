#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::RenderEvent
    
    Render events are sent by the RenderDevice to registered render 
    event handlers.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class RenderEvent
{
public:
    /// event codes
    enum Code
    {
        InvalidCode,
        DeviceOpen,
        DeviceClose,
        DeviceLost,         // D3D9 only: the render device has been lost
        DeviceRestored,     // D3D9 only: the render device has been restored
    };

    /// default constructor
    RenderEvent();
    /// constructor with event code
    RenderEvent(Code c);
    /// get event code
    Code GetEventCode() const;

private:
    Code code;
};

//------------------------------------------------------------------------------
/**
*/
inline
RenderEvent::RenderEvent() :
    code(InvalidCode)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
RenderEvent::RenderEvent(Code c) :
    code(c)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
RenderEvent::Code
RenderEvent::GetEventCode() const
{
    return this->code;
}

} // namespace CoreGraphics
//------------------------------------------------------------------------------

