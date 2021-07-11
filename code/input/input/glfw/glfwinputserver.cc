//------------------------------------------------------------------------------
//  glfwinputserver.cc
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input/glfw/glfwinputserver.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/gamepad.h"

namespace GLFW
{
__ImplementClass(GLFW::GLFWInputServer, 'GLIS', Base::InputServerBase);
__ImplementSingleton(GLFW::GLFWInputServer);

using namespace Input;

//------------------------------------------------------------------------------
/**
*/
GLFWInputServer::GLFWInputServer()     
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
GLFWInputServer::~GLFWInputServer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**    
*/
void
GLFWInputServer::Open()
{
    n_assert(!this->IsOpen());
    InputServerBase::Open();

    // create a default keyboard and mouse handler
    this->defaultKeyboard = Keyboard::Create();
    this->AttachInputHandler(InputPriority::Game, this->defaultKeyboard.upcast<InputHandler>());
    this->defaultMouse = Mouse::Create();
    this->AttachInputHandler(InputPriority::Game, this->defaultMouse.upcast<InputHandler>());

    // create 4 default gamepads (none of them have to be connected)
    IndexT playerIndex;
    for (playerIndex = 0; playerIndex < this->maxNumLocalPlayers; playerIndex++)
    {
        this->defaultGamePad[playerIndex] = GamePad::Create();
        this->defaultGamePad[playerIndex]->SetIndex(playerIndex);
        this->AttachInputHandler(InputPriority::Game, this->defaultGamePad[playerIndex].upcast<InputHandler>());
    }
}

//------------------------------------------------------------------------------
/**    
*/
void
GLFWInputServer::Close()
{
    n_assert(this->IsOpen());

    // call parent class
    InputServerBase::Close();
}

//------------------------------------------------------------------------------
/**    
*/
void
GLFWInputServer::OnFrame()
{
    //this->eventHandler->HandlePendingEvents();    
    InputServerBase::OnFrame();
}

} // namespace GLFW