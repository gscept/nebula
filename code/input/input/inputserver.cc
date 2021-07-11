//------------------------------------------------------------------------------
//  inputserver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input/inputserver.h"

namespace Input
{
__ImplementClass(Input::InputServer, 'INPS', GLFW::GLFWInputServer);
__ImplementInterfaceSingleton(Input::InputServer);

//------------------------------------------------------------------------------
/**
*/
InputServer::InputServer()
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
InputServer::~InputServer()
{
    __DestructInterfaceSingleton;
}

} // namespace Input
