//------------------------------------------------------------------------------
//  @file filebrowserpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/panels/filebrowserpanel.h"
#include "neweditor/widgets/confirmmodal.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/model/scene.h"
#include "neweditor/widgets/filebrowser.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/base/iconregistry.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/uri.h"
#include "system/process.h"
#include "neweditor/base/palette.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::FileBrowserPanel, 'FLBP', Editor::EditorPanel);

// The popup the menu draws as. The open and the begin have to say the same thing, so the name is
// kept here
static const char* FileContextPopup = "file_context";

// The command the OS has for opening a folder in the file manager. The engine's process launch
// looks the name
// up in PATH, so it has to be one the platform installs
#ifdef __WIN32__
static const char* const FileManagerCommand = "explorer";
#elif defined(__APPLE__)
static const char* const FileManagerCommand = "open";
#else
static const char* const FileManagerCommand = "xdg-open";
#endif

//------------------------------------------------------------------------------
/**
*/
FileBrowserPanel::FileBrowserPanel() :
    contextPopupRequested(false),
    openProcess(System::InvalidProcessId)
{
    this->name = "files";
    this->browser = FileBrowser::Create();
}

//------------------------------------------------------------------------------
/**
*/
FileBrowserPanel::~FileBrowserPanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
FileBrowserPanel::OnSetup()
{
    this->browser->Setup(this->core);

    EditorCommandSystem& commands = this->core->GetCommands();
    commands.Register("cd", "print the folder the file browser shows, or change it. '..' goes one folder up. A path with spaces goes in quotes",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<FileBrowserPanel, &FileBrowserPanel::CommandCd>(this));
    commands.Register("refresh", "re-read the folder the file browser shows",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<FileBrowserPanel, &FileBrowserPanel::CommandRefresh>(this));

    this->core->GetKeybinds().Register("file.parent", Input::Key::Back, Input::Modifier::NoModifier, "cd ..");

    return true;
}

//------------------------------------------------------------------------------
/**
    The browser is what reads and what draws the address bar and the table. What is here is the
    window's own chrome: the title row, the scroll region between the pinned header and the footer,
    and the count.
*/
void
FileBrowserPanel::Draw()
{
    // The title row, with the refresh button right aligned on it
    float avail = ImGui::GetContentRegionAvail().x;
    float xStart = ImGui::GetCursorPosX();

    ImGui::Text("FILES");
    ImGui::SameLine();

    const char* refreshGlyph = IconGlyph("refresh-cw");
    const char* refreshText = refreshGlyph != nullptr ? refreshGlyph : "refresh";
    const float buttonWidth = ImGui::CalcTextSize(refreshText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float off = avail - buttonWidth;
    if (off > 0.0f)
        ImGui::SetCursorPosX(xStart + off);

    if (IconButton(refreshGlyph, "re-read the folder"))
        this->browser->MarkDirty();

    ImGui::Separator();

    this->browser->DrawAddressBar();

    // The listing is the scroll region, so the address bar stays put and the footer keeps the count
    // at the
    // bottom. The child gets everything in between
    const float footer = ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("listing", ImVec2(0, -footer));

    this->browser->DrawEntryTable();

    // A file the browser reports from a double click becomes a scene tab through the core, at the
    // action
    // flush after the panels have drawn: opening a scene during the draw would change the arrays
    // the
    // panels are still walking. Only scene files go through, other formats have nothing to do yet
    const Util::String opened = this->browser->ConsumeFileOpen();
    if (!opened.IsEmpty() && this->IsSceneFile(opened))
        this->OpenDoubleClickedFile(opened);

    ImGui::EndChild();

    // The menu is a window of its own, so it is drawn after the listing rather than inside it, like
    // the
    // outline's
    this->DrawContextMenu();

    int numFolders = 0;
    int numFiles = 0;
    const Util::Array<FileBrowser::Entry>& entries = this->browser->GetEntries();
    for (IndexT i = 0; i < entries.Size(); i++)
    {
        if (entries[i].isDir)
            numFolders++;
        else
            numFiles++;
    }

    Util::String footerText;
    footerText.Format("%d folders, %d files", numFolders, numFiles);
    if (!this->browser->GetSelectedName().IsEmpty())
    {
        footerText.Append("  selected: ");
        footerText.Append(this->browser->GetSelectedName());
    }
    ImGui::TextDisabled("%s", footerText.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserPanel::DrawContextMenu()
{
    const Util::String clicked = this->browser->ConsumeContextEntry();
    if (!clicked.IsEmpty())
    {
        this->contextName = clicked;
        this->contextPopupRequested = true;
    }

    if (this->openProcess != System::InvalidProcessId && !System::IsProcessRunning(this->openProcess))
        this->openProcess = System::InvalidProcessId;

    if (this->contextPopupRequested)
    {
        ImGui::OpenPopup(FileContextPopup);
        this->contextPopupRequested = false;
    }

    if (ImGui::BeginPopup(FileContextPopup))
    {
        const FileBrowser::Entry* entry = this->EntryNamed(this->contextName);
        if (entry != nullptr)
        {
            ImGui::TextUnformatted(entry->name.AsCharPtr());
            ImGui::Separator();

            const bool open = this->SceneOpenFor(this->browser->ChildPath(entry->name));

            if (ImGui::MenuItem("Open"))
                this->OpenEntry(*entry);

            if (ImGui::MenuItem("Open containing folder"))
                this->OpenContainingFolder();

            if (ImGui::MenuItem("Rename", nullptr, false, !open))
                this->browser->OpenRenameField(entry->name);
            if (open)
                ImGui::SetItemTooltip("Close this file before renaming");

            ImGui::Separator();
            if (ImGui::MenuItem("Delete", nullptr, false, !open))
                this->AskDelete(*entry);
            if (open)
                ImGui::SetItemTooltip("Close this file before deleting");
        }

        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserPanel::OpenEntry(const FileBrowser::Entry& entry)
{
    if (entry.isDir)
    {
        this->browser->Navigate(this->browser->ChildPath(entry.name));
        return;
    }

    if (this->IsSceneFile(entry.name))
    {
        this->OpenDoubleClickedFile(entry.name);
        return;
    }

    this->SetStatus("Nothing to open for " + entry.name, 3.0f);
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserPanel::AskDelete(const FileBrowser::Entry& entry)
{
    this->pendingDelete = entry.name;

    Util::String question;
    if (entry.isDir)
        question.Format("Delete the folder %s and everything in it?", entry.name.AsCharPtr());
    else
        question.Format("Delete %s?", entry.name.AsCharPtr());

    this->core->GetConfirmModal()->Show("Delete", question, "Delete", "Cancel",
        Util::Delegate<void()>::FromMethod<FileBrowserPanel, &FileBrowserPanel::ApplyDelete>(this),
        Util::Delegate<void()>());
}

//------------------------------------------------------------------------------
/**
    The answer runs after the frame has drawn, which is where the filesystem is allowed to change.
*/
void
FileBrowserPanel::ApplyDelete()
{
    const Util::String name = this->pendingDelete;
    this->pendingDelete.Clear();
    if (name.IsEmpty())
        return;

    if (this->browser->DeleteEntry(name))
        this->SetStatus("Deleted " + name, 3.0f);
    else
        this->SetStatus("Could not delete " + name, 3.0f);
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserPanel::OpenContainingFolder()
{
    const IO::URI dir(this->browser->GetDir());
    if (!dir.IsValid())
    {
        this->SetStatus("No folder to open", 3.0f);
        return;
    }

    System::ProcessStartInfo startInfo;
    startInfo.exePath = IO::URI(FileManagerCommand);
    startInfo.workingDir = IO::URI();
    startInfo.consoleWindow = false;
    startInfo.outputStream = nullptr;
    startInfo.errorStream = nullptr;

    // The launch tokenizes the argument string on spaces, so a path with spaces has to come in
    // quotes
    Util::String args;
    args.AppendChar('"');
    args.Append(dir.LocalPath());
    args.AppendChar('"');
    startInfo.args = args;

    this->openProcess = System::StartProcess(startInfo);
    if (this->openProcess == System::InvalidProcessId)
        this->SetStatus("Could not open " + dir.LocalPath() + " in the file manager", 3.0f);
}

//------------------------------------------------------------------------------
/**
*/
const FileBrowser::Entry*
FileBrowserPanel::EntryNamed(const Util::String& name) const
{
    const Util::Array<FileBrowser::Entry>& entries = this->browser->GetEntries();
    for (IndexT i = 0; i < entries.Size(); i++)
    {
        if (entries[i].name == name)
            return &entries[i];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileBrowserPanel::SceneOpenFor(const Util::String& path) const
{
    const Util::Array<Ptr<Scene>>& scenes = this->core->GetScenes();
    for (IndexT i = 0; i < scenes.Size(); i++)
    {
        if (scenes[i]->GetFilePath() == path)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserPanel::CommandCd(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty())
    {
        this->core->Log(LogMessageType::N_MESSAGE, "File browser shows " + this->browser->GetDir());
        return;
    }

    // A path with spaces is one word when it is in quotes. Without them the command line takes the
    // first word of
    // it and makes a folder out of that, so the warning says so rather than following it
    if (args.Size() > 1)
    {
        this->core->Log(LogMessageType::N_WARNING, "cd takes one path, or '..'. A path with spaces goes in quotes");
        return;
    }

    if (args[0] == "..")
    {
        this->browser->Navigate(FileBrowser::ParentDir(this->browser->GetDir()));
        return;
    }

    // A path with no assign and no leading slash is read below the folder being shown, which is
    // what makes
    // 'cd data' from 'work:' land on 'work:data'
    Util::String path = args[0];
    if (!path.IsEmpty() && path[0] != '/' && path.FindCharIndex(':') == InvalidIndex)
        path = this->browser->ChildPath(args[0]);
    this->browser->Navigate(path);
}

//------------------------------------------------------------------------------
/**
    The read happens here rather than by marking the entries dirty, which is what the button does,
    so that a refresh also works when nothing draws. The logged count is what a command script
    checks against.
*/
void
FileBrowserPanel::CommandRefresh(const Util::Array<Util::String>& args)
{
    this->browser->RefreshNow();

    int numFolders = 0;
    int numFiles = 0;
    const Util::Array<FileBrowser::Entry>& entries = this->browser->GetEntries();
    for (IndexT i = 0; i < entries.Size(); i++)
    {
        if (entries[i].isDir)
            numFolders++;
        else
            numFiles++;
    }

    Util::String msg;
    msg.Format("File browser read %s: %d folders, %d files", this->browser->GetDir().AsCharPtr(), numFolders, numFiles);
    this->core->Log(LogMessageType::N_MESSAGE, msg);
}

//------------------------------------------------------------------------------
/**
    The extension decides, because the browser window lists every file and only scene files have a
    use. The check is on the last dot, so a name with dots in the middle still reads right.
*/
bool
FileBrowserPanel::IsSceneFile(const Util::String& name) const
{
    const IndexT dot = name.FindCharIndexReverse('.');
    if (dot == InvalidIndex)
        return false;

    Util::String ext = name.ExtractRange(dot + 1, (IndexT)name.Length() - dot - 1);
    ext.ToLower();
    return ext == "json";
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserPanel::OpenDoubleClickedFile(const Util::String& name)
{
    this->sceneFileName = name;
    this->core->QueueAction(Util::Delegate<void()>::FromMethod<FileBrowserPanel, &FileBrowserPanel::OpenQueuedScene>(this));
}

//------------------------------------------------------------------------------
/**
    The name waits in the member because Util::Delegate takes a method, not a closure over the name.
    One frame at most sits between the double click and the flush that runs this.
*/
void
FileBrowserPanel::OpenQueuedScene()
{
    if (this->sceneFileName.IsEmpty())
        return;

    this->core->OpenOrFocusSceneFile(this->browser->ChildPath(this->sceneFileName));
    this->sceneFileName.Clear();
}

} // namespace Editor
