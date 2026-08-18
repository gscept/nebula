//------------------------------------------------------------------------------
//  toolbar.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "toolbar.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/cmds.h"
#include "dynui/nebula_icons.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Toolbar, 'TBWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Toolbar::Toolbar()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Toolbar::~Toolbar()
{
    // empty
}

#define IMGUI_VERTICAL_SEPARATOR \
    ImGui::SameLine();\
    ImGui::Text(" | "); \
    ImGui::SameLine();

//------------------------------------------------------------------------------
/**
*/
void
Toolbar::Run(SaveMode save)
{
    const ImVec2 buttonSize = {32,32};
    
    //if (ImGui::ImageButton("playimage", &UIManager::Icons::play, buttonSize, {0,0}, {1,1})) { PlayGame(); }
    //ImGui::SameLine();
    //if (ImGui::ImageButton("pauseimage", &UIManager::Icons::pause, buttonSize, {0,0}, {1,1})) { PauseGame(); }
    //ImGui::SameLine();
    //if (ImGui::ImageButton("stopimage", &UIManager::Icons::stop, buttonSize, {0,0}, {1,1})) { StopGame(); }

    if (ImGui::Button("Play")) { PlayGame(); }
    ImGui::SameLine();
    if (ImGui::Button(ICON_ttf_PAUSE_CIRCLE)) { PauseGame(); }
    ImGui::SameLine();
    if (ImGui::Button(ICON_ttf_STOP_CIRCLE)) { StopGame(); }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopFont();
}

} // namespace Presentation
