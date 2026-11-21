#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::FileBrowserModal

    This class is the file dialog. It shows Editor::FileBrowser in a popup, with a name field and
    two buttons. A save or an open needs those, and browsing alone does not.

    The core keeps one dialog and shows it every frame. A caller opens it with one of these
    functions:

    ShowSave(title, startDir, defaultName, filter, onAccepted, onCanceled) ShowOpen(title, startDir,
    filter, onAccepted) The callback runs after the frame has drawn, through the core's action
    queue. A dialog must not change the model while a panel is walking it.

    For a save, the dialog adds the extension to a name that has none. It asks before it writes over
    a file that exists. The question is on the button row, not in another popup.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "core/refcounted.h"
#include "util/delegate.h"
#include "util/string.h"

namespace Editor
{

class Shell;
class FileBrowser;

class FileBrowserModal final : public Core::RefCounted
{
    __DeclareClass(FileBrowserModal)
public:
    FileBrowserModal();
    virtual ~FileBrowserModal();

    /// Prepare the dialog. The core calls this function at Setup.
    bool Setup(Shell* core);
    /// Release what the dialog holds.
    void Discard();

    /// Open the dialog and ask where to save. The filter is the pattern the folder lists. The
    /// filter's extension is added to a typed name that has none. defaultName is what the field
    /// starts with. onCanceled runs when the dialog closes with no answer.
    void ShowSave(const Util::String& title, const Util::String& startDir, const Util::String& defaultName,
        const Util::String& filter, Util::Delegate<void(const Util::String&)> onAccepted,
        Util::Delegate<void()> onCanceled = Util::Delegate<void()>());
    /// Open the dialog and ask what to open. The field shows the selected file. A double click on a
    /// file accepts it.
    void ShowOpen(const Util::String& title, const Util::String& startDir,
        const Util::String& filter, Util::Delegate<void(const Util::String&)> onAccepted);

    /// Show the popup. The core calls this function once per frame, inside the ImGui frame.
    void Draw();

private:
    enum Mode
    {
        ModeSave,
        ModeOpen,
    };

    /// Return the extension the filter stands for. '*.json' gives '.json'. Return empty when it has
    /// none.
    static Util::String ExtensionOfFilter(const Util::String& filter);

    /// Return the path the folder and the field make together, with the extension completed.
    Util::String TargetPath() const;
    /// Close the popup and queue the callback with the chosen path.
    void Accept(const Util::String& path);
    /// Run the queued callback.
    void RunAccepted();
    /// Run the queued cancel.
    void RunCanceled();
    /// Queue the cancel, unless an accept already ran. Both popup close paths call this function.
    void CancelUnlessAccepted();

    Shell* core;
    Ptr<FileBrowser> browser;

    /// what the dialog is for
    Mode mode;
    /// the popup name. ImGui uses it to open and close the popup.
    Util::String title;
    /// the pattern the folder lists
    Util::String filter;
    /// the filter's extension. A name without one gets this.
    Util::String extension;
    Util::Delegate<void(const Util::String&)> onAccepted;
    /// what runs when the dialog closes with no answer
    Util::Delegate<void()> onCanceled;
    /// true after Accept queued its callback, so the close does not also run the cancel
    bool acceptedFired;

    /// true when the popup is open
    bool visible;
    /// true when the popup must open on the next draw
    bool openRequested;
    /// true when the popup must close on the next draw
    bool closeRequested;

    /// The name field needs a buffer that stays between frames.
    static const IndexT MaxNameLength = 256;
    char nameBuffer[MaxNameLength];
    /// true when the name field gets focus on the next draw
    bool nameFocusRequested;

    /// true when the target file exists and Save was pressed. The row asks instead of writing.
    bool confirmOverwrite;
    /// the folder the question belongs to. It is dropped when the folder changes.
    Util::String confirmDir;

    /// the selection the field last showed. The field is only written when the selection changed.
    Util::String mirroredSelection;

    /// the path Accept passed, for the queued run
    Util::String acceptedPath;
};

} // namespace Editor
