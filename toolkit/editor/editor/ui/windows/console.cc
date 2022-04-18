//------------------------------------------------------------------------------
//  console.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "console.h"
#include "imgui.h"
#include "graphics/graphicsserver.h"
#include "frame/frameserver.h"
#include "coregraphics/texture.h"
#include "input/inputserver.h"
#include "input/keyboard.h"

namespace Presentation
{
__ImplementClass(Presentation::Console, 'cosl', Presentation::BaseWindow)

//------------------------------------------------------------------------------
/**
*/
Console::Console()
{
    // Default to false until we have user profiles implemented
    this->open = false;

    if (!Dynui::ImguiConsole::HasInstance())
        this->console = Dynui::ImguiConsole::Create();
    else
        this->console = Dynui::ImguiConsole::Instance();

    this->consoleHandler = Dynui::ImguiConsoleHandler::Create();
    this->console->Setup();
    this->consoleHandler->Setup();
}

//------------------------------------------------------------------------------
/**
*/
Console::~Console()
{
}

//------------------------------------------------------------------------------
/**
*/
void
Console::Run()
{
    this->console->RenderContent();
}

//------------------------------------------------------------------------------
/**
*/
void
Console::Update()
{
    // This should be moved to some centralized location for all keyboard shortcuts.
    ImGuiIO& io = ImGui::GetIO();
    if (Input::InputServer::Instance()->GetDefaultKeyboard()->KeyDown(Input::Key::F8) || io.KeysDownDuration[Input::Key::F8] == 0.0f)
    {
        // Toggle window
        this->open = !open;
    }
}

} // namespace Presentation


