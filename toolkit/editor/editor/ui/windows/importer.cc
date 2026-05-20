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

#include "dynui/nebula_icons.h"
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

    static Util::Array<Util::Pair<Util::String, ToolkitUtil::TextureResourceT>> TextureResources;
    if (!Dynui::ImguiDragAndDropFiles.IsEmpty())
    {
        for (const auto& file : Dynui::ImguiDragAndDropFiles)
        {
            static Util::String ext;
            ext = file.GetFileExtension();

            if (ext == "fbx")
            {
            }
            else if (ext == "gltf" || ext == "glb")
            {
            }
            else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" || ext == "dds" || ext == "exr" || ext == "cube")
            {
                TextureResources.Append({ file, ToolkitUtil::SetupTextureImportSettingsFromPath(file) });
            }
        }
    }



    IndexT textureResourceIndex = 0;
    for (auto& [file, textureResource] : TextureResources)
    {
        ImGui::PushFont(Dynui::ImguiBoldFont, 0.0f);
        ImGui::Text("%s", file.AsCharPtr());
        ImGui::PopFont();
        Destination.ConvertBackslashes();
        if (ImGui::Button(ICON_ttf_FOLDER_OPEN))
        {
            IGFD::FileDialogConfig config;
            config.flags = ImGuiFileDialogFlags_Modal;

            static char absolutePathBuf[256];
            memcpy(absolutePathBuf, Destination.data(), Destination.Length());
            absolutePathBuf[Destination.Length()] = '\0';
            config.path = absolutePathBuf;
            ImGuiFileDialog::Instance()->OpenDialog("ChoseFolderDlgKey", "Output directory", nullptr, config);
        }
        ImGui::SameLine();
        Util::String shortenedPath = Destination.StripSubstring(IO::URI("src:").LocalPath());
        static char buf[256];
        memcpy(buf, shortenedPath.data(), shortenedPath.Length());
        buf[shortenedPath.Length()] = '\0';
        float width = ImGui::CalcTextSize(buf).x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetNextItemWidth(width);
        Util::String fileNameNoExt = file.ExtractFileName();
        fileNameNoExt.StripFileExtension();
        Util::String label = Util::Format("/%s.natex##destination", fileNameNoExt.AsCharPtr());
        ImGui::InputText(label.AsCharPtr(), buf, sizeof(buf), ImGuiInputTextFlags_NoHorizontalScroll);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushFont(Dynui::ImguiIconFont, 0.0f);
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeDir, "", style.Colors[ImGuiCol_TabSelected], ICON_ttf_FOLDER_OPEN);
        if (ImGuiFileDialog::Instance()->Display("ChoseFolderDlgKey"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                Destination = ImGuiFileDialog::Instance()->GetCurrentPath().c_str();
            }
            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::PopFont();

        ImGui::PopStyleVar();
        ImGui::Spacing();

        ImGui::Checkbox("Generate Mipmaps", &textureResource.generate_mipmaps);
        ImGui::Checkbox("Invert Green (Y) Channel", &textureResource.invert_green);
        ImGui::SetItemTooltip("Some formats (like BC5) expect the green channel to be inverted. Enable this if your normal maps look wrong after importing.");
        static const char* pixelFormatOptions[] =
        {
            "R8G8B8A8",
            "R8G8B8",
            "R16",
            "BC1",
            "BC2",
            "BC3",
            "BC5",
            "BC6H",
            "BC7",
        };
        static const char* formatHints[] =
        {
            "Import as a typical 8 bit per channel RGBA texture. This is the most widely supported format, but also the largest on disk and in memory.",
            "Import as a typical 8 bit per channel RGB texture. This is widely supported and smaller than RGBA, but doesn't support transparency.",
            "Import as a single channel 16 bit texture. This is useful for height maps or other data that doesn't require color information.",
            "Import using BC1 compression. This is a common compressed format that is widely supported, but doesn't support alpha or has limited alpha support.",
            "Import using BC2 compression. This is a common compressed format that supports alpha, but has limited alpha quality.",
            "Import using BC3 compression. This is a common compressed format that supports alpha with better quality than BC2, but is larger on disk.",
            "Import using BC5 compression. This is a common compressed format for normal maps, but doesn't support color information.",
            "Import using BC6H compression. This is a common compressed format for HDR textures, but doesn't support alpha or color information.",
            "Import using BC7 compression. This is a modern compressed format that supports high quality alpha and color, but isn't supported on older hardware.",
        };
        if (ImGui::BeginCombo("Format", pixelFormatOptions[textureResource.target_format]))
        {
            for (int i = 0; i < ToolkitUtil::TexturePixelFormat_MAX + 1; i++)
            {
                bool isSelected = (textureResource.target_format == i);
                if (ImGui::Selectable(pixelFormatOptions[i], isSelected))
                {
                    textureResource.target_format = static_cast<ToolkitUtil::TexturePixelFormat>(i);
                }
                ImGui::SetItemTooltip(formatHints[i]);
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        static const char* colorSpaceOptions[] =
        {
            "Linear",
            "sRGB",
        };
        if (ImGui::BeginCombo("Color space", colorSpaceOptions[textureResource.color_space]))
        {
            for (int i = 0; i < ToolkitUtil::TextureColorSpace_MAX + 1; i++)
            {
                bool isSelected = (textureResource.color_space == i);
                if (ImGui::Selectable(colorSpaceOptions[i], isSelected))
                {
                    textureResource.color_space = static_cast<ToolkitUtil::TextureColorSpace>(i);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        static const char* compressionQualityOptions[] =
        {
            "Low",
            "High",
        };
        if (ImGui::BeginCombo("Compression quality", compressionQualityOptions[textureResource.compression_quality]))
        {
            for (int i = 0; i < ToolkitUtil::TextureCompressionQuality_MAX + 1; i++)
            {
                bool isSelected = (textureResource.compression_quality == i);
                if (ImGui::Selectable(compressionQualityOptions[i], isSelected))
                {
                    textureResource.compression_quality = static_cast<ToolkitUtil::TextureCompressionQuality>(i);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        static Util::String saveButton = Util::String::Sprintf("%s Import %s", ICON_ttf_SAVE, file.ExtractFileName().AsCharPtr());
        if (ImGui::Button(saveButton.AsCharPtr()))
        {
            ToolkitUtil::ImportTexture(file, Destination, textureResource);
            TextureResources.EraseIndex(textureResourceIndex);
        }
        textureResourceIndex++;
    }
}

} // namespace Presentation
