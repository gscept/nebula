//------------------------------------------------------------------------------
//  importer.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "importer.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/tools/selectiontool.h"
#include "editor/cmds.h"
#include "physics/debugui.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/scene.h"
#include "graphics/cameracontext.h"

#include "toolkitutil/asset/assetimporter.h"
#include "dynui/imguifiledialog/imguifiledialog.h"
#include "tinyfiledialogs.h"
using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Importer, 'PtIm', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Importer::Importer()
{
    this->additionalFlags = ImGuiWindowFlags_(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
}

//------------------------------------------------------------------------------
/**
*/
Importer::~Importer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Importer::Run(SaveMode save)
{
    static Util::Array<Util::String> Files;
    static Util::String Destination = IO::URI("src:assets").LocalPath();
    if (!Dynui::ImguiDragAndDropFiles.IsEmpty())
    {
        Files = Dynui::ImguiDragAndDropFiles;
    }

    for (const auto& file : Files)
    {
        Util::String ext = file.GetFileExtension();
            
        if (ext == "fbx")
        {
        }
        else if (ext == "gltf" || ext == "glb")
        {
        }
        else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" || ext == "dds" || ext == "exr" || ext == "cube")
        {
            ImGui::Text(file.AsCharPtr());

            ImGui::Text("Destination:");
            ImGui::SameLine();
            char buf[256];
            memcpy(buf, Destination.data(), Destination.Length());
            buf[Destination.Length()] = '\0';
            ImGui::InputText("##destination", buf, sizeof(buf));
            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                IGFD::FileDialogConfig config;
                config.flags = ImGuiFileDialogFlags_Modal;
                config.path = buf;
                ImGuiFileDialog::Instance()->OpenDialog("ChoseFolderDlgKey", "Output directory", nullptr, config);
            }
            if (ImGuiFileDialog::Instance()->Display("ChoseFolderDlgKey"))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    Destination = ImGuiFileDialog::Instance()->GetCurrentPath().c_str();
                }
                ImGuiFileDialog::Instance()->Close();
            }

            ImGui::Spacing();
            ImGui::SameLine();
            if (ImGui::Button("Import"))
            {
                ToolkitUtil::ImportTexture(file, Destination);
            }
        }
    }
}

} // namespace Presentation
