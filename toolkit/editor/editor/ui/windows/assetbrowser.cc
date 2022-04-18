//------------------------------------------------------------------------------
//  assetbrowser.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "assetbrowser.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "io/ioserver.h"
#include "io/fswrapper.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::AssetBrowser, 'AsBw', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::AssetBrowser()
{
    // this->additionalFlags = ImGuiWindowFlags_MenuBar;
}

//------------------------------------------------------------------------------
/**
*/
AssetBrowser::~AssetBrowser()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::Update()
{
    
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::Run()
{
    DisplayFileTree();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowser::DisplayFileTree()
{
    static int selectedFile = -1;
    static int selected = 0;
    static bool isInputtingPath = false;
    static char inputPath[NEBULA_MAXPATH];
    static Util::String outpath = IO::URI("export:").LocalPath();
    static Util::String pattern = "*.*";
    static bool once = true;

    ImGui::PushItemWidth(ImGui::GetWindowWidth());
    if (!isInputtingPath)
    {
        ImGui::Text(outpath.AsCharPtr());
        if (ImGui::IsItemClicked())
        {
            isInputtingPath = true;
            Memory::Copy(outpath.AsCharPtr(), inputPath, outpath.Length());
            inputPath[outpath.Length()] = 0;
            // ImGui::SetKeyboardFocusHere(1);
        }
    }
    else
    {
        if (ImGui::InputText("##pathInput", inputPath, NEBULA_MAXPATH, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            IO::URI uri(inputPath);
            Util::String localPath = uri.LocalPath();
            if (IO::FSWrapper::DirectoryExists(localPath))
            {
                outpath = localPath;
            }
            isInputtingPath = false;
        }
        if ((!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)))
        {
            isInputtingPath = false;
        }
    }
    ImGui::PopItemWidth();


    ImGui::Separator();
    if (outpath.IsEmpty() || !IO::FSWrapper::DirectoryExists(outpath))
    {
        n_warning("Invalid path provided. Fallback to user directory.\n");
        outpath = IO::FSWrapper::GetUserDirectory();
        IO::URI uri = outpath;
        outpath = uri.LocalPath();
    }

    Util::Array<Util::String> fileList = IO::FSWrapper::ListFiles(outpath, pattern);
    Util::Array<Util::String> dirList = IO::FSWrapper::ListDirectories(outpath, "*");

    auto xSize = 0;
    auto ySize = ImGui::GetContentRegionAvail().y;
    auto marginBottom = 10;

    ImGui::BeginChild("FileBrowser", ImVec2(xSize, ySize - marginBottom), true);
    {
        ImGui::Columns(2);
        if (once)
        {
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
            once = false;
        }
        ImGui::BeginChild("ScrollingRegionDirectories", ImVec2(0, ImGui::GetContentRegionMax().y), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::SmallButton("^"))
        {
            if (outpath.EndsWithString("/"))
            {
                outpath = outpath.ExtractRange(0, outpath.Length() - 1);
            }

            auto s = outpath.ExtractFileName();
            if (!s.IsEmpty())
            {
                outpath = outpath.ExtractRange(0, outpath.Length() - s.Length());
                if (outpath.EndsWithString("/"))
                {
                    outpath = outpath.ExtractRange(0, outpath.Length() - 1);
                }
            }
            selectedFile = -1;
            selected = -1;
        }

        for (int i = 0; i < dirList.Size(); ++i)
        {
            ImGui::BeginGroup();
            {
                Util::String name = dirList[i];
                ImGui::SmallButton("D");
                ImGui::SameLine();
                ImGui::Selectable(name.AsCharPtr(), selected == i, ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
                if (ImGui::IsItemClicked())
                {
                    selected = i;
                    selectedFile = -1;
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        IO::URI uri = outpath;
                        uri.AppendLocalPath(dirList[i]);
                        outpath = uri.LocalPath();
                    }
                }
            }
            ImGui::EndGroup();
        }

        for (int i = 0; i < fileList.Size(); ++i)
        {
            ImGui::BeginGroup();
            {
                Util::String const& name = fileList[i];
                ImGui::SmallButton("F");
                ImGui::SameLine();
                ImGui::Selectable(name.AsCharPtr(), selected == i + dirList.Size(), ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
                if (ImGui::IsItemClicked())
                {
                    selected = i + dirList.Size();
                    selectedFile = i;

                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        IO::URI uri = outpath;
                        uri.AppendLocalPath(fileList[i]);
                        //outpath = uri.LocalPath();
                        // TODO: Open file event
                    }
                }
            }
            ImGui::EndGroup();
        }
        ImGui::EndChild();

        ImGui::NextColumn();
        // -- Items column
        ImGuiStyle& style = ImGui::GetStyle();
        int numFiles = fileList.Size();
        float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        static int itemSize = 50;
        ImGui::SliderInt("Zoom", &itemSize, 25, 200);
        ImGui::BeginChild("ScrollingRegionMiniatures", ImVec2(0, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (int n = 0; n < numFiles; n++)
        {
            Util::String const& name = fileList[n];
            ImGui::PushID(n);
            ImGui::BeginGroup();
            ImGui::ImageButton(&UIManager::Icons::game, { (float)itemSize, (float)itemSize });
            ImGui::BeginChild("##filename00", { (float)itemSize, ImGui::GetTextLineHeight()}, false, ImGuiWindowFlags_NoScrollbar);
            ImGui::Text(name.AsCharPtr());
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(name.AsCharPtr());
                ImGui::EndTooltip();
            }
            ImGui::EndChild();
            ImGui::EndGroup();
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                Util::String filePath = outpath + "/" + name;
                ImGui::SetDragDropPayload("resource", filePath.AsCharPtr(), sizeof(char) * filePath.Length() + 1);
                ImGui::EndDragDropSource();
            }

            float lastButtonX = ImGui::GetItemRectMax().x;
            float nextButtonX = lastButtonX + style.ItemSpacing.x + (float)itemSize + 30.0f; // Expected position if next button was on same line
            if (n + 1 < numFiles && nextButtonX < windowVisibleX)
                ImGui::SameLine();

            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

} // namespace Presentation
