#pragma once

//------------------------------------------------------------------------------
/**
    @class OpenGL4::GLFWInputDisplayEventHandler
  
    Translates DisplayEvents that are relevant for the input system
    into InputEvents.
        
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/displayeventhandler.h"

//------------------------------------------------------------------------------
namespace GLFW
{
class GLFWInputDisplayEventHandler : public CoreGraphics::DisplayEventHandler
{
    __DeclareClass(GLFWInputDisplayEventHandler);
public:
    /// called when an event happens
    virtual bool HandleEvent(const CoreGraphics::DisplayEvent& event);
};

} // namespace GLFW
//------------------------------------------------------------------------------
