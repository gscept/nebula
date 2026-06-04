//------------------------------------------------------------------------------
//  windowmanager.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "windowserver.h"
#include "imgui.h"
#include "io/jsonwriter.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "app/application.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "windows/scriptedwindow.h"
#include "io/filedialog.h"
#include "editor/entityloader.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/editor.h"
#include "editor/cmds.h"
#include "uimanager.h"

#include "toolkit-common/logger.h"
#include "io/ioserver.h"

#include "dynui/imguicontext.h"
#include "toolkitutil/asset/assetimporter.h"
#include "toolkitutil/asset/assetpackager.h"
#include "dynui/imguifiledialog/imguifiledialog.h"
#include "tinyfiledialogs.h"

#include "dynui/nebula_icons.h"

using namespace Util;

namespace Presentation
{

__ImplementClass(Presentation::WindowServer, 'wSrv', Core::RefCounted);
__ImplementSingleton(Presentation::WindowServer);

//------------------------------------------------------------------------------
/**
*/
WindowServer::WindowServer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
WindowServer::~WindowServer()
{
    __DestructSingleton;
}


//------------------------------------------------------------------------------
/**
*/
void
RunImportWindows(ToolkitUtil::Logger* logger)
{
    static Util::Array<Util::String> Files;
    static Util::String Destination = IO::URI("src:assets").LocalPath();

    struct ModelImportSettings
    {
        bool removeRedundantVertices = 1;
        bool calculateNormals = 0;
        bool flipUVs = 0;
        bool importColors = 1;
        bool importSecondaryUVs = 1;
        bool calculateTangents = 0;
        bool calculateRigidSkin = 0;
        bool replaceExistingMesh = 0;
        int scale = 1; // 0 means cm, 1 means m, 2 means km
    };

    static Util::Array<Util::Tuple<Util::String, ToolkitUtil::TextureResourceT, CoreGraphics::TextureId, Ids::Id32, Resources::ResourceId>> TextureResources;
    static Util::Array<Util::Tuple<Util::String, ModelImportSettings>> FbxFiles;
    static Util::Array<Util::Tuple<Util::String, ModelImportSettings>> GltfFiles;
    if (!Dynui::ImguiDragAndDropFiles.IsEmpty())
    {
        for (const auto& file : Dynui::ImguiDragAndDropFiles)
        {
            static Util::String ext;
            ext = file.GetFileExtension();

            if (ext == "fbx")
            {
                FbxFiles.Append({ file, {} });
            }
            else if (ext == "gltf" || ext == "glb")
            {
                GltfFiles.Append({ file, {} });
            }
            else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" || ext == "dds" || ext == "exr" || ext == "cube")
            {
                TextureResources.Append(
                    {
                        file,
                        ToolkitUtil::SetupTextureImportSettingsFromPath(file),
                        CoreGraphics::InvalidTextureId,
                        Dynui::AllocateImguiTextureId({}),
                        Resources::InvalidResourceId
                    }
                );
            }
        }
    }

    IndexT textureResourceIndex = 0;
    for (auto& [file, textureResource, texture, imguiId, resId] : TextureResources)
    {
        Util::String title = Util::Format("Import Texture: %s", file.ExtractFileName().AsCharPtr());
        if (ImGui::Begin(title.AsCharPtr()))
        {
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
            ImGuiFileDialog::Instance()->SetFileStyle(
                IGFD_FileStyleByTypeDir, "", style.Colors[ImGuiCol_TabSelected], ICON_ttf_FOLDER_OPEN
            );
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

            if (ImGui::BeginTable("TextureImportTable", 2))
            {
                ImGui::TableSetupScrollFreeze(1, 1);
                ImGui::TableNextColumn();
                ImTextureRef ref;
                ref._TexID = imguiId;
                ImGui::Image(ref, ImVec2(300, 300));
                ImGui::TableNextColumn();
                ImGui::Spacing();

                bool changed = false;

                changed |= ImGui::Checkbox("Generate Mipmaps", &textureResource.generate_mipmaps);
                changed |= ImGui::Checkbox("Invert Green (Y) Channel", &textureResource.invert_green);
                ImGui::SetItemTooltip("Some formats (like BC5) expect the green channel to be inverted. Enable this if your normal "
                                      "maps look wrong after importing.");
                static const char* pixelFormatOptions[] = {
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
                static const char* formatHints[] = {
                    "Import as a typical 8 bit per channel RGBA texture. This is the most widely supported format, but also the "
                    "largest on disk and in memory.",
                    "Import as a typical 8 bit per channel RGB texture. This is widely supported and smaller than RGBA, but doesn't "
                    "support transparency.",
                    "Import as a single channel 16 bit texture. This is useful for height maps or other data that doesn't require "
                    "color information.",
                    "Import using BC1 compression. This is a common compressed format that is widely supported, but doesn't support "
                    "alpha or has limited alpha support.",
                    "Import using BC2 compression. This is a common compressed format that supports alpha, but has limited alpha "
                    "quality.",
                    "Import using BC3 compression. This is a common compressed format that supports alpha with better quality than "
                    "BC2, but is larger on disk.",
                    "Import using BC5 compression. This is a common compressed format for normal maps, but doesn't support color "
                    "information.",
                    "Import using BC6H compression. This is a common compressed format for HDR textures, but doesn't support alpha "
                    "or color information.",
                    "Import using BC7 compression. This is a modern compressed format that supports high quality alpha and color, "
                    "but isn't supported on older hardware.",
                };
                if (ImGui::BeginCombo("Format", pixelFormatOptions[textureResource.target_format]))
                {
                    for (int i = 0; i < ToolkitUtil::TexturePixelFormat_MAX + 1; i++)
                    {
                        bool isSelected = (textureResource.target_format == i);
                        if (ImGui::Selectable(pixelFormatOptions[i], isSelected))
                        {
                            textureResource.target_format = static_cast<ToolkitUtil::TexturePixelFormat>(i);
                            changed = true;
                        }
                        ImGui::SetItemTooltip(formatHints[i]);
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                static const char* colorSpaceOptions[] = {
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
                            changed = true;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                static const char* compressionQualityOptions[] = {
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
                            changed = true;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (changed || resId == Resources::InvalidResourceId)
                {
                    int hashCode = file.ExtractFileName().HashCode();
                    Util::String outputFileName = Util::Format("temp:importer/%d.dds", hashCode);
                    if (resId != Resources::InvalidResourceId)
                    {
                        Resources::DiscardResource(resId);
                        resId = Resources::InvalidResourceId;
                    }
                    Ptr<IO::Stream> sourceStream = IO::CreateStream(file);
                    if (sourceStream->Open())
                    {
                        std::vector<uint8_t> data;
                        data.resize(sourceStream->GetSize());
                        memcpy(data.data(), sourceStream->MemoryMap(), sourceStream->GetSize());
                        textureResource.data = data;

                        if (ToolkitUtil::PackageTexture(
                            &textureResource,
                            file.ExtractFileName(),
                            "temp:importer/",
                            ToolkitUtil::Platform::Code::Win32,
                            logger
                        ))
                        {
                            resId = Resources::CreateResource(
                                outputFileName,
                                "editor",
                                [&resId, &texture, imguiId](Resources::ResourceId id)
                            {
                                resId = id;
                                texture = id.resourceId;
                                Dynui::ImguiTextureId tex;
                                tex.nebulaHandle = texture;
                                Dynui::SetImguiTextureIdData(imguiId, tex);
                            }
                            );
                            texture = resId.resourceId;
                        }

                        sourceStream->Close();
                    }
                }

                static Util::String saveButton =
                    Util::String::Sprintf("%s Import %s", ICON_ttf_SAVE, fileNameNoExt.ExtractFileName().AsCharPtr());
                if (ImGui::Button(saveButton.AsCharPtr()))
                {
                    ToolkitUtil::ImportTexture(file, Destination, textureResource);
                    TextureResources.EraseIndex(textureResourceIndex);
                }

                ImGui::EndTable();
            } // End of table
            textureResourceIndex++;
            ImGui::End();
        }
    }

    IndexT fbxFileIndex = 0;
    for (auto& [file, settings] : FbxFiles)
    {
        Util::String title = Util::Format("Import FBX: %s", file.ExtractFileName().AsCharPtr());
        if (ImGui::Begin(title.AsCharPtr()))
        {
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
            Util::String label = Util::Format("##destination %s", fileNameNoExt.AsCharPtr());
            ImGui::InputText(label.AsCharPtr(), buf, sizeof(buf), ImGuiInputTextFlags_NoHorizontalScroll);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushFont(Dynui::ImguiIconFont, 0.0f);
            const ImGuiStyle& style = ImGui::GetStyle();
            ImGuiFileDialog::Instance()->SetFileStyle(
                IGFD_FileStyleByTypeDir, "", style.Colors[ImGuiCol_TabSelected], ICON_ttf_FOLDER_OPEN
            );
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

            ImGui::Checkbox("Replace existing mesh", &settings.replaceExistingMesh);
            ImGui::Checkbox("Remove redundant vertices", &settings.removeRedundantVertices);
            ImGui::Checkbox("Calculate normals", &settings.calculateNormals);
            ImGui::Checkbox("Flip UVs", &settings.flipUVs);
            ImGui::Checkbox("Import vertex colors", &settings.importColors);
            ImGui::Checkbox("Import secondary UVs", &settings.importSecondaryUVs);
            ImGui::Checkbox("Calculate tangents", &settings.calculateTangents);
            ImGui::Checkbox("Calculate rigid skinning", &settings.calculateRigidSkin);
            ImGui::Combo("Scale", &settings.scale, "centimeters\000meters\000kilometers\000");

            Util::String saveButton =
                Util::String::Sprintf("%s Import %s", ICON_ttf_SAVE, file.ExtractFileName().AsCharPtr());

            if (ImGui::Button(saveButton.AsCharPtr()))
            {
                uint flags = 0x0;
                flags |= settings.replaceExistingMesh ? ToolkitUtil::ImportFlags::ReplaceExistingMesh : ToolkitUtil::ImportFlags::None;
                flags |= settings.removeRedundantVertices ? ToolkitUtil::ImportFlags::RemoveRedundant : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateNormals ? ToolkitUtil::ImportFlags::CalcNormals : ToolkitUtil::ImportFlags::None;
                flags |= settings.flipUVs ? ToolkitUtil::ImportFlags::FlipUVs : ToolkitUtil::ImportFlags::None;
                flags |= settings.importColors ? ToolkitUtil::ImportFlags::ImportColors : ToolkitUtil::ImportFlags::None;
                flags |= settings.importSecondaryUVs ? ToolkitUtil::ImportFlags::ImportSecondaryUVs : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateTangents ? ToolkitUtil::ImportFlags::CalcTangents : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateRigidSkin ? ToolkitUtil::ImportFlags::CalcRigidSkin : ToolkitUtil::ImportFlags::None;
                float scale = 1.0f;
                switch (settings.scale)
                {
                    case 0: scale = 0.01f; break; // cm to m
                    case 1: scale = 1.0f; break; // m to m
                    case 2: scale = 1000.0f; break; // km to m
                }
                ToolkitUtil::ImportFBX(file, Destination, (ToolkitUtil::ImportFlags)flags, scale, logger);
                FbxFiles.EraseIndex(fbxFileIndex);
            }

            fbxFileIndex++;
            ImGui::End();
        }
    }

    IndexT gltfFileIndex = 0;
    for (auto& [file, settings] : GltfFiles)
    {
        Util::String title = Util::Format("Import GLTF: %s", file.ExtractFileName().AsCharPtr());
        if (ImGui::Begin(title.AsCharPtr()))
        {

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
            Util::String label = Util::Format("##destination %s", fileNameNoExt.AsCharPtr());
            ImGui::InputText(label.AsCharPtr(), buf, sizeof(buf), ImGuiInputTextFlags_NoHorizontalScroll);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushFont(Dynui::ImguiIconFont, 0.0f);
            const ImGuiStyle& style = ImGui::GetStyle();
            ImGuiFileDialog::Instance()->SetFileStyle(
                IGFD_FileStyleByTypeDir, "", style.Colors[ImGuiCol_TabSelected], ICON_ttf_FOLDER_OPEN
            );
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

            ImGui::Checkbox("Replace existing mesh", &settings.replaceExistingMesh);
            ImGui::Checkbox("Remove redundant vertices", &settings.removeRedundantVertices);
            ImGui::Checkbox("Calculate normals", &settings.calculateNormals);
            ImGui::Checkbox("Flip UVs", &settings.flipUVs);
            ImGui::Checkbox("Import vertex colors", &settings.importColors);
            ImGui::Checkbox("Import secondary UVs", &settings.importSecondaryUVs);
            ImGui::Checkbox("Calculate tangents", &settings.calculateTangents);
            ImGui::Checkbox("Calculate rigid skinning", &settings.calculateRigidSkin);
            ImGui::Combo("Scale", &settings.scale, "centimeters\000meters\000kilometers\000");

            Util::String saveButton =
                Util::String::Sprintf("%s Import %s.nasset", ICON_ttf_SAVE, fileNameNoExt.AsCharPtr());

            if (ImGui::Button(saveButton.AsCharPtr()))
            {
                uint flags = 0x0;
                flags |= settings.replaceExistingMesh ? ToolkitUtil::ImportFlags::ReplaceExistingMesh : ToolkitUtil::ImportFlags::None;
                flags |= settings.removeRedundantVertices ? ToolkitUtil::ImportFlags::RemoveRedundant : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateNormals ? ToolkitUtil::ImportFlags::CalcNormals : ToolkitUtil::ImportFlags::None;
                flags |= settings.flipUVs ? ToolkitUtil::ImportFlags::FlipUVs : ToolkitUtil::ImportFlags::None;
                flags |= settings.importColors ? ToolkitUtil::ImportFlags::ImportColors : ToolkitUtil::ImportFlags::None;
                flags |= settings.importSecondaryUVs ? ToolkitUtil::ImportFlags::ImportSecondaryUVs : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateTangents ? ToolkitUtil::ImportFlags::CalcTangents : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateRigidSkin ? ToolkitUtil::ImportFlags::CalcRigidSkin : ToolkitUtil::ImportFlags::None;
                float scale = 1.0f;
                switch (settings.scale)
                {
                    case 0: scale = 0.01f; break; // cm to m
                    case 1: scale = 1.0f; break; // m to m
                    case 2: scale = 1000.0f; break; // km to m
                }
                ToolkitUtil::ImportGLTF(file, Destination, (ToolkitUtil::ImportFlags)flags, scale, logger);
                GltfFiles.EraseIndex(gltfFileIndex);
            }

            gltfFileIndex++;
            ImGui::End();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RunAll()
{
    //List all windows in windows menu
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("    File    "))
        {
            if (ImGui::MenuItem("Load", "Ctrl+O"))
            {
                static Util::String localpath = IO::URI("proj:").LocalPath();
                Util::String path;
                if (IO::FileDialog::OpenFile("Select Nebula Level", localpath, { "*.json" }, path))
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
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveActive);
            }

            if (ImGui::MenuItem("Save All", "Ctrl+Shift+S"))
            {
                Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveAll);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("    Edit    "))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
            {
                Edit::CommandManager::Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z"))
            {
                Edit::CommandManager::Redo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("    Create    "))
        {
            if (ImGui::MenuItem("Entity", "Ctrl+C"))
            {
                Presentation::WindowServer::Instance()->GetWindow("Create Object")->Open() = true;
            }
            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Point Light"))
                {
                    Edit::CommandManager::BeginMacro("Create point light", true);
                    Editor::Entity newEntity = Edit::CreateEntity("PointLight");
                    Edit::SetSelection({ newEntity });
                    Edit::CommandManager::EndMacro();
                }
                if (ImGui::MenuItem("Spot Light"))
                {
                    Edit::CommandManager::BeginMacro("Create spot light", true);
                    Editor::Entity newEntity = Edit::CreateEntity("SpotLight");
                    Edit::SetSelection({ newEntity });
                    Edit::CommandManager::EndMacro();
                }
                if (ImGui::MenuItem("Area Light"))
                {
                    Edit::CommandManager::BeginMacro("Create area light", true);
                    Editor::Entity newEntity = Edit::CreateEntity("AreaLight");
                    Edit::SetSelection({ newEntity });
                    Edit::CommandManager::EndMacro();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("    Game    "))
        {
            if (ImGui::MenuItem("Play", "Ctrl+P"))
            {
                Editor::PlayGame();
            }
            if (ImGui::MenuItem("Pause", "Ctrl+Shift+P"))
            {
                Editor::PauseGame();
            }
            if (ImGui::MenuItem("Stop", "Ctrl+S"))
            {
                Editor::StopGame();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("    Window    "))
        {
            if (ImGui::BeginMenu("Show"))
            {
                // TODO: there's WAY better ways to do this.

                // First, show all sub categories
                for (SizeT i = 0; i < this->categories.Size(); i++)
                {
                    auto& category = this->categories[i];
                    if (ImGui::BeginMenu(category.AsCharPtr()))
                    {
                        for (SizeT j = 0; j < this->windows.Size(); j++)
                        {
                            auto it = this->windows[j];
                            if (it->GetCategory() == category)
                            {
                                ImGui::MenuItem(it->GetName().AsCharPtr(), NULL, &it->Open());
                            }
                        }
                        ImGui::EndMenu();
                    }
                }

                // last, show categoryless windows.
                for (SizeT i = 0; i < this->windows.Size(); i++)
                {
                    auto it = this->windows[i];
                    if (it->GetCategory().IsEmpty())
                        ImGui::MenuItem(it->GetName().AsCharPtr(), NULL, &it->Open());
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Shortcuts"))
            {
                for (SizeT i = 0; i < this->commands.Size(); i++)
                {
                    auto const& shortcutStr = this->commands.ValueAtIndex(i).shortcut;
                    const char* shortcut = shortcutStr.IsEmpty() ? NULL : shortcutStr.AsCharPtr();
                    if (ImGui::MenuItem(this->commands.ValueAtIndex(i).label.AsCharPtr(), shortcut))
                    {
                        this->commands.ValueAtIndex(i).func();
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save Window Layout"))
            {
                const IO::URI path(Editor::UIManager::GetEditorUIIniPath());
                ImGui::SaveIniSettingsToDisk(path.LocalPath().c_str());
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    RunImportWindows(&this->logger);

    //Run all windows
    for (SizeT i = 0; i < this->windows.Size(); i++)
    {
        auto it = this->windows[i];
        
        N_SCOPE_DYN(it->name.AsCharPtr(), UI)
        if (it->Open())
        {
            if (it->usesCustomWindowPadding)
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {it->windowPadding.x, it->windowPadding.y});

            if (ImGui::Begin(it->GetName().AsCharPtr(), &it->Open(), it->GetAdditionalFlags()))
            {
                it->Run(this->save);
            }
            ImGui::End();

            if (it->usesCustomWindowPadding)
                ImGui::PopStyleVar();
        }
    }
    this->save = BaseWindow::SaveMode::None;
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::Update()
{
    for (SizeT i = 0; i < this->windows.Size(); i++)
    {
        auto it = this->windows[i];
        it->Update();
    }

    auto const& keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();

    // check for shortcuts
    IndexT cmdIndex = -1;
    int longestShortcut = 0;
    for (IndexT i = 0; i < this->commands.Size(); ++i)
    {
        CommandInfo const& cmd = this->commands.ValueAtIndex(i);
        bool exec = true;
        for (IndexT k = 0; k < cmd.keys.Size(); ++k)
        {
            auto key = cmd.keys[k];

            // if we're processing the last key, we check if its been pressed once, other keys if they're held down.
            if (k == (cmd.keys.Size() - 1))
            {
                if (!keyboard->KeyDown(key))
                {
                    exec = false;
                    break;
                }
            }
            else
            {
                bool pressed = keyboard->KeyPressed(key);
                if (!pressed)
                {
                    if (key == Input::Key::Code::Control)
                    {
                        // special case, check both left and right key
                        if (!(keyboard->KeyPressed(Input::Key::Code::LeftControl) || keyboard->KeyPressed(Input::Key::Code::RightControl)))
                        {
                            exec = false;
                            break;
                        }
                    }
                    else if (key == Input::Key::Code::Shift)
                    {
                        // special case, check both left and right key
                        if (!(keyboard->KeyPressed(Input::Key::Code::LeftShift) || keyboard->KeyPressed(Input::Key::Code::RightShift)))
                        {
                            exec = false;
                            break;
                        }
                    }
                    else if (key == Input::Key::Code::Menu)
                    {
                        // special case, check both left and right key
                        if (!(keyboard->KeyPressed(Input::Key::Code::LeftMenu) || keyboard->KeyPressed(Input::Key::Code::RightMenu)))
                        {
                            exec = false;
                            break;
                        }
                    }
                    else
                    {
                        exec = false;
                        break;
                    }
                }
            }
        }

        // run command
        if (exec && cmd.keys.Size() > longestShortcut)
        {
            longestShortcut = cmd.keys.Size();
            cmdIndex = i;
        }
    }

    if (cmdIndex != -1)
    {
        this->commands.ValueAtIndex(cmdIndex).func();
    }

}

//------------------------------------------------------------------------------
/**
*/
void 
WindowServer::BroadcastSave(BaseWindow::SaveMode mode)
{
    this->save = mode;
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RegisterWindow(const Util::String & className, const char * label, const char* category)
{
    Ptr<BaseWindow> intFace((BaseWindow*)Core::Factory::Instance()->Create(className));
    n_assert2(intFace != nullptr, "Interface could not be found by provided class name!");

    intFace->SetName(label);
    intFace->SetCategory(category);
    this->windowByName.Add(label, intFace);
    this->windows.Append(intFace);
    this->AddCategory(category);
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RegisterWindow(const Util::FourCC fourcc, const char * label, const char* category)
{
    Ptr<BaseWindow> intFace((BaseWindow*)Core::Factory::Instance()->Create(fourcc));
    n_assert2(intFace != nullptr, "Interface could not be found by provided FourCC");

    intFace->SetName(label);
    intFace->SetCategory(category);
    this->windowByName.Add(label, intFace);
    this->windows.Append(intFace);
    this->AddCategory(category);
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RegisterWindow(const Ptr<BaseWindow>& base)
{
    this->windowByName.Add(base->GetName(), base);
    this->windows.Append(base);
    this->AddCategory(base->GetCategory());
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RegisterWindowScript(const char* script, const char* label)
{
	Ptr<ScriptedWindow> wnd = ScriptedWindow::Create();
	wnd->SetName(label);
	if (wnd->LoadModule(script))
	{
		this->RegisterWindow(wnd.upcast<BaseWindow>());
	}
}

//------------------------------------------------------------------------------
/**
    If menu is NULL, the command won't show up in the menu.
    If category is NULL, the command is placed directly in the menu tab
*/
void
WindowServer::RegisterCommand(Util::Delegate<void()> func, Util::String const& label, Util::String const& shortcut, const char* menu, const char* category)
{
    if (this->commands.Contains(label))
    {
        n_warning("Command delegate with label %s already exists!", label.AsCharPtr());
        return;
    }

    // Split the shortcut into keycodes and validate
    Util::Array<Util::String> keyTokens = shortcut.Tokenize("+");

    Util::FixedArray<Input::Key::Code> keys;
    keys.SetSize(keyTokens.Size());

    for (IndexT i = 0; i < keyTokens.Size(); ++i)
    {
        if (!Input::Key::IsValid(keyTokens[i]))
        {
            n_warning("Command: \"%s\", Shortcut: \"%s\" - %s is not a valid keycode!", label.AsCharPtr(), shortcut.AsCharPtr(), keyTokens[i].AsCharPtr());
            return;
        }

        auto key = Input::Key::FromString(keyTokens[i]);
        keys[i] = key;
    }

    CommandInfo info = {
        func,
        label,
        shortcut,
        keys,
        menu,
        category
    };

    this->commands.Add(label, info);
}

//------------------------------------------------------------------------------
/**
*/
Ptr<BaseWindow>
WindowServer::GetWindow(const Util::String & name)
{
    auto i = this->windowByName.FindIndex(name);
    if (i != InvalidIndex)
    {
        return this->windowByName.ValueAtIndex(name, i);
    }

    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::AddCategory(const Util::String & category)
{
    if (!category.IsEmpty())
    {
        if (this->categories.FindIndex(category) == InvalidIndex)
        {
            this->categories.Append(category);
        }
    }
}

} // namespace Presentation
