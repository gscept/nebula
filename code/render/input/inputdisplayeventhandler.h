#pragma once

//------------------------------------------------------------------------------
/**
    @class Input::InputDisplayEventHandler
  
    Translates DisplayEvents that are relevant for the input system
    into InputEvents.
        
    @copyright
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/displayeventhandler.h"

//------------------------------------------------------------------------------
namespace Input
{
class InputDisplayEventHandler : public CoreGraphics::DisplayEventHandler
{
    __DeclareClass(InputDisplayEventHandler);
public:
    /// called when an event happens
    virtual bool HandleEvent(const CoreGraphics::DisplayEvent& event);
};

} // namespace GLFW
//------------------------------------------------------------------------------
