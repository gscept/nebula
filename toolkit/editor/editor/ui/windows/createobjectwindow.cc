//------------------------------------------------------------------------------
//  createobjectwindow.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "createobjectwindow.h"
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

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::CreateObjectWindow, 'COWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
CreateObjectWindow::CreateObjectWindow()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
CreateObjectWindow::~CreateObjectWindow()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
CreateObjectWindow::Run(SaveMode save)
{
    const ImVec2 buttonSize = {32,32};
    
    ImGui::Text("Blueprint");
    ImVec2 avail = ImGui::GetContentRegionAvail();
    static const char* selected = "Empty";
    if (ImGui::BeginListBox("##Template", ImVec2(0, avail.y)))
    {
        auto const& templates = Game::BlueprintManager::ListTemplates();
        for (auto const& tmpl : templates)
        {
            if (ImGui::Selectable(tmpl.name.Value(), selected == tmpl.name.Value()))
                selected = tmpl.name.Value();

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                Edit::CommandManager::BeginMacro("Create entity", true);
                Editor::Entity newEntity = Edit::CreateEntity(selected);
                Edit::SetSelection({ newEntity });
                Edit::CommandManager::EndMacro();
            }
        }
        ImGui::EndListBox();
    }
}

} // namespace Presentation
