//------------------------------------------------------------------------------
//  imguifeatureunit.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "imguiaddon.h"
#include "input/inputserver.h"
#include "imguirenderer.h"

namespace Dynui
{
__ImplementClass(Dynui::ImguiAddon, 'IMFU', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ImguiAddon::ImguiAddon()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ImguiAddon::~ImguiAddon()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiAddon::Setup()
{
    // register context
    Dynui::ImguiContext::Create();

    // create and register input handler
    this->inputHandler = ImguiInputHandler::Create();
    Input::InputServer::Instance()->AttachInputHandler(Input::InputPriority::DynUi, this->inputHandler.upcast<Input::InputHandler>());
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiAddon::Discard()
{
    // remove input handler
    Input::InputServer::Instance()->RemoveInputHandler(this->inputHandler.upcast<Input::InputHandler>());
    this->inputHandler = nullptr;

    Dynui::ImguiContext::Destroy();
}

} // namespace Dynui