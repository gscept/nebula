#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::InputServer
    
    The InputServer is the central object of the Input subsystem. It 
    mainly manages a prioritized list of input handlers which process
    incoming input events.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/singleton.h"
#if (__DX11__ || __DX9__ )
#include "input/win32/win32inputserver.h"
namespace Input
{
class InputServer : public Win32::Win32InputServer
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
#elif (__OGL4__ || __VULKAN__)
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
#else
#error "InputServer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


   