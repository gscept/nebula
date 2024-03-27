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
Toolbar::Run()
{
    const ImVec2 buttonSize = {32,32};

    if (ImGui::Button("Save")) 
    {
        static Util::String localpath = IO::URI("bin:").LocalPath();
        Util::String path;
        if (IO::FileDialog::SaveFile("Select Nebula Level", localpath, { "*.json" }, path))
            Editor::SaveEntities(path.AsCharPtr());
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        static Util::String localpath = IO::URI("bin:").LocalPath();
        Util::String path;
        if (IO::FileDialog::OpenFile("Select Nebula Level", localpath, {"*.json"}, path))
        {
            Ptr<Editor::EntityLoader> loader = Editor::EntityLoader::Create();
            loader->SetWorld(Editor::state.editorWorld);
            Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
            reader->SetStream(IO::IoServer::Instance()->CreateStream(path));
            if (reader->Open())
            {
                loader->LoadJsonLevel(reader);
            }
            reader->Close();
        }
    }

    IMGUI_VERTICAL_SEPARATOR;

    if (ImGui::Button("Undo")) { Edit::CommandManager::Undo(); }
    ImGui::SameLine();
    if (ImGui::Button("Redo")) { Edit::CommandManager::Redo(); }
    
    IMGUI_VERTICAL_SEPARATOR;
    
    static const char* selected = "Empty";
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
            Edit::CreateEntity(selected);
        }
    }
    
    IMGUI_VERTICAL_SEPARATOR;
    
    //if (ImGui::ImageButton("playimage", &UIManager::Icons::play, buttonSize, {0,0}, {1,1})) { PlayGame(); }
    //ImGui::SameLine();
    //if (ImGui::ImageButton("pauseimage", &UIManager::Icons::pause, buttonSize, {0,0}, {1,1})) { PauseGame(); }
    //ImGui::SameLine();
    //if (ImGui::ImageButton("stopimage", &UIManager::Icons::stop, buttonSize, {0,0}, {1,1})) { StopGame(); }

    if (ImGui::Button("Play")) { PlayGame(); }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) { PauseGame(); }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) { StopGame(); }

    IMGUI_VERTICAL_SEPARATOR;

    bool snapToGridIncrements = Tools::SelectionTool::SnapToGridIncrements();
    if (ImGui::Checkbox("Grid snap", &snapToGridIncrements))
    {
        Tools::SelectionTool::SnapToGridIncrements(snapToGridIncrements);
    }

}

} // namespace Presentation
