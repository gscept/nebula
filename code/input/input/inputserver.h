#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::InputServer
    
    The InputServer is the central object of the Input subsystem. It 
    mainly manages a prioritized list of input handlers which process
    incoming input events.

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/singleton.h"
#include "input/glfw/glfwinputserver.h"
namespace Input
{
class InputServer : public GLFW::GLFWInputServer
{
    __DeclareClass(InputServer);
    __DeclareInterfaceSingleton(InputServer);
public:
    /// constructor
    InputServer();
    /// destructor
    virtual ~InputServer();
};
}
//------------------------------------------------------------------------------


   