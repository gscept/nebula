//------------------------------------------------------------------------------
//  settings.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "settings.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/cmds.h"
#include "editor/tools/selectiontool.h"
#include "imgui_internal.h"
#include "im3d/im3dcontext.h"
#include "io/ioserver.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Settings, 'SeWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Settings::Settings()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Settings::~Settings()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Settings::Update()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Settings::Run(SaveMode save)
{
    //bool snapToGridIncrements = Tools::SelectionTool::SnapToGridIncrements();
    //if (ImGui::Checkbox("Grid snap", &snapToGridIncrements))
    //{
    //    //Tools::SelectionTool::SnapToGridIncrements(snapToGridIncrements);
    //}

    bool showGrid = Im3d::Im3dContext::GetGridStatus();
    if (ImGui::Checkbox("Show Grid", &showGrid))
    {
        Im3d::Im3dContext::SetGridStatus(showGrid);
    }
    float cellSize;
    int cellCount;
    ImGui::BeginDisabled(!showGrid);
    Im3d::Im3dContext::GetGridSize(cellSize, cellCount);
    bool gridChanged = ImGui::DragFloat("Cell size", &cellSize);
    gridChanged |= ImGui::DragInt("Cell count", &cellCount);
    if (showGrid && gridChanged)
    {
        Im3d::Im3dContext::SetGridSize(cellSize, cellCount);
    }
    Math::vec4 gridColour = Im3d::Im3dContext::GetGridColor();
    if (ImGui::ColorEdit4("Grid colour", gridColour.v))
    {
        Im3d::Im3dContext::SetGridColor(gridColour);
    }
    Math::vec2 gridOffset = Im3d::Im3dContext::GetGridOffset();
    if (ImGui::DragFloat2("Grid offset", &gridOffset.x))
    {
        Im3d::Im3dContext::SetGridOffset(gridOffset);
    }
    ImGui::EndDisabled();

    //bool use3DGizmo = Tools::SelectionTool::UseIm3DGizmo();
    //if (ImGui::Checkbox("Use 3D Gizmo for selection", &use3DGizmo))
    //{
    //    Tools::SelectionTool::UseIm3DGizmo(use3DGizmo);
    //}
    //int gizmoSize;
    //int gizmoWidth;
    //ImGui::BeginDisabled(!use3DGizmo);
    //Im3d::Im3dContext::GetGizmoSize(gizmoSize, gizmoWidth);
    //bool sizeChange = ImGui::DragInt("Gizmo size", &gizmoSize);
    //sizeChange |= ImGui::DragInt("Gizmo width", &gizmoWidth);
    //if (use3DGizmo && sizeChange)
    //{
    //    Im3d::Im3dContext::SetGizmoSize(gizmoSize, gizmoWidth);
    //}
    //ImGui::EndDisabled();
    const IO::URI path(Editor::UIManager::GetEditorUIIniPath());
    if (ImGui::Button("Save editor layout"))
    { 
        ImGui::SaveIniSettingsToDisk(path.LocalPath().c_str());
    }
    if (ImGui::Button("Load editor layout"))
    {
        ImGui::LoadIniSettingsFromDisk(path.LocalPath().c_str());
    }
    if (ImGui::Button("Reset to default layout"))
    {
        const Util::String defaultIni = "tool:syswork/data/editor/defaultui.ini";
        IO::IoServer::Instance()->CreateDirectory("user:nebula/editor/");
        if (IO::IoServer::Instance()->FileExists(defaultIni))
        {
            IO::IoServer::Instance()->CopyFile(defaultIni, path);
        }
        ImGui::LoadIniSettingsFromDisk(path.LocalPath().c_str());
    }
}

} // namespace Presentation
