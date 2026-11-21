#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::FileBrowserPanel

    The file browser window. It shows one folder detail view style:

    the name, size, type and modified columns, with the parent folder as the first row. There is no
    tree and no folding, so the address field is what moves around the filesystem.

    The browsing itself lives in Editor::FileBrowser, which the save dialog uses the same way, so
    this panel is what is specific to being a window: the title row, the refresh button, the footer,
    the 'cd' and 'refresh' commands that drive the browser from the console, opening a scene file on
    a double click - a file that already has a tab goes to that tab instead of reading a second
    copy.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "neweditor/core/editorpanel.h"
#include "neweditor/widgets/filebrowser.h"
#include "io/uri.h"
#include "system/process.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class FileBrowserPanel final : public EditorPanel
{
    __DeclareClass(FileBrowserPanel)
public:
    FileBrowserPanel();
    virtual ~FileBrowserPanel();

    /// draw the address bar and the entry table
    void Draw() override;

protected:
    bool OnSetup() override;

private:
    /// commands
    void CommandCd(const Util::Array<Util::String>& args);
    void CommandRefresh(const Util::Array<Util::String>& args);

    /// true when the name ends in the scene file extension
    bool IsSceneFile(const Util::String& name) const;
    /// the entry the menu is about, by name, or null when the listing no longer has it
    const FileBrowser::Entry* EntryNamed(const Util::String& name) const;

    /// a double clicked file waits for the action flush, which is where the model can change
    void OpenDoubleClickedFile(const Util::String& name);
    void OpenQueuedScene();

    /// the context menu, drawn after the table. The entry it applies to is the one the browser
    /// reported
    void DrawContextMenu();

    /// what the menu items do
    void OpenEntry(const FileBrowser::Entry& entry);
    void AskDelete(const FileBrowser::Entry& entry);
    void ApplyDelete();
    /// the folder the entry is in, opened in the OS file manager
    void OpenContainingFolder();
    /// true when a tab holds the path. The menu cannot do its two disk things to a file a tab is
    /// reading
    bool SceneOpenFor(const Util::String& path) const;

    /// the browsing: the folder, its entries, the address field and the table
    Ptr<FileBrowser> browser;

    /// the double clicked file, from the draw that saw the click to the action that opens it
    Util::String sceneFileName;

    /// the entry the menu is open on. The popup is a window of its own, so the entry it applies to
    /// has to be
    /// remembered rather than read from where the click landed
    Util::String contextName;
    bool contextPopupRequested;

    /// the entry the delete question is about, which is what the answer runs on
    Util::String pendingDelete;

    /// the file manager the last menu item launched. The launch is not waited on, but the process
    /// still has to
    /// be polled for its exit, or the editor keeps a zombie for every folder it opened
    System::ProcessId openProcess;
};

} // namespace Editor
