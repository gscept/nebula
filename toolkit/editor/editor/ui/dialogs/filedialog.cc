//------------------------------------------------------------------------------
//  filedialog.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "filedialog.h"
#include "util/string.h"
#include "imgui.h"
#include "io/fswrapper.h"
#include "io/console.h"

namespace Presentation
{
namespace Dialogs
{

static bool isInputtingPath = false;
static char inputPath[NEBULA_MAXPATH];

//-----------------------------------------------------------------------------------------------------------
/**
    Opens a file dialog.

    Returns -1 if no input was given.

    @todo   Support for navigation using tabs/arrows and return key.
    @todo   Support for multiple file selection
    @todo   List URIs in a column (mdl:, bin:, etc.)
*/
FileResult OpenFileDialog(const Util::String& title, Util::String& outpath, const char* pattern, bool& open)
{
    FileResult retval = FileResult::NOINPUT;
    static int selectedFile = -1;
    static int selected = 0;

    if (open)
    {
        ImGui::OpenPopup(title.AsCharPtr());
    }
    if (ImGui::BeginPopupModal(title.AsCharPtr(), &open))
    {
        ImGui::SetWindowSize(ImVec2(600, 600), ImGuiCond_Once);
        
        
        // Path at top of window
        ImGui::SetKeyboardFocusHere(0);
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
            if (!ImGui::IsAnyItemActive() || (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)))
            {
                isInputtingPath = false;
            }
        }
        ImGui::PopItemWidth();


        ImGui::Separator();
        if (outpath.IsEmpty() || !IO::FSWrapper::DirectoryExists(outpath))
        {
            IO::Console::Instance()->Warning("Invalid path provided. Fallback to user directory.\n");
            outpath = IO::FSWrapper::GetUserDirectory();
            IO::URI uri = outpath;
            outpath = uri.LocalPath();
        }

        Util::Array<Util::String> fileList = IO::FSWrapper::ListFiles(outpath, pattern);
        Util::Array<Util::String> dirList = IO::FSWrapper::ListDirectories(outpath, "*");

        auto xSize = 0;
        auto ySize = ImGui::GetWindowSize().y;
        auto marginBottom = 100;

        ImGui::BeginChild("FileBrowser", ImVec2(xSize, ySize - marginBottom), true);
        {
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
                    ImGui::Selectable(name.AsCharPtr(), selected == i, ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_::ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
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
                    Util::String name = fileList[i];
                    ImGui::SmallButton("F");
                    ImGui::SameLine();
                    ImGui::Selectable(name.AsCharPtr(), selected == i + dirList.Size(), ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_::ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
                    if (ImGui::IsItemClicked())
                    {
                        selected = i + dirList.Size();
                        selectedFile = i;

                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            IO::URI uri = outpath;
                            uri.AppendLocalPath(fileList[i]);
                            outpath = uri.LocalPath();
                            retval = FileResult::OKAY;
                            open = false;
                        }
                    }
                }
                ImGui::EndGroup();
            }

        }
        ImGui::EndChild();

        ImGui::Separator();

        //Unique id
        int id = title.HashCode();
        ImGui::PushID(id);
        if (selectedFile > -1)
        {
            if (ImGui::Button("Ok", ImVec2(120, 0)))
            {
                IO::URI uri = outpath;
                uri.AppendLocalPath(fileList[selectedFile]);
                outpath = uri.LocalPath();
                retval = FileResult::OKAY;
                open = false;
            }
        }
        else
        {
            // Disabled button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3, 0.3, 0.3, 0.5));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3, 0.3, 0.3, 0.5));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3, 0.3, 0.3, 0.5));
            ImGui::Button("Ok", ImVec2(120, 0));
            ImGui::PopStyleColor(3);
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(id + 1);
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            retval = FileResult::CANCEL;
            open = false;
        }
        ImGui::PopID();

        ImGui::EndPopup();
    }

    return retval;
}

} // namespace Dialogs
} // namespace Presentation
