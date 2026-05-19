//------------------------------------------------------------------------------
//  toolbar.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "toolbar.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/cmds.h"
#include "editor/entityloader.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "io/filedialog.h"
#include "io/ioserver.h"
#include "basegamefeature/level.h"

#include "editor/tools/selectiontool.h"
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
    static const char* selected = "Empty";
    float h = ImGui::CalcTextSize("Empty").y;
    float avail = ImGui::GetContentRegionAvail().y;

    float oldPosY = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail - h) * 0.5f);

    ImGui::PushItemWidth(300);
    if (ImGui::BeginCombo("##Template", selected))
    {
        auto const& templates = Game::BlueprintManager::ListTemplates();
        for (auto const& tmpl : templates)
        {
            if (ImGui::Selectable(tmpl.name.Value(), selected == tmpl.name.Value()))
                selected = tmpl.name.Value();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Create"))
    {
        if (selected != nullptr)
        {
            Edit::CommandManager::BeginMacro("Create entity", true);
            Editor::Entity newEntity = Edit::CreateEntity(selected);
            Edit::SetSelection({newEntity});
            Edit::CommandManager::EndMacro();
        }
    }
    
    IMGUI_VERTICAL_SEPARATOR;
    ImGui::PushFont(Dynui::ImguiFont, 32.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0.0f, 0.0f });
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(ICON_ttf_PLAY_CIRCLE)) { PlayGame(); }
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
