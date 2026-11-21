#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::KeybindsModal

    This class shows the keybinds as a table. Each row shows the action, the key combo, and the
    command line. Each row can do two things. It can take the next key press as the new keybind, and
    it can set the action back to the default that the code registered.

    The core keeps one modal. It shows the modal every frame, after the panels, so the popup is on
    top of them. The modal opens through a command. The keybind, the toolbar button and the console
    all use that command. Anything that runs a command line can open the modal.

    The modal takes the next key press for the row that waits for a key. The modal gives that key to
    the keybind registry. It does not give the key to the command the key would have run. The modal
    writes the key to the file, in the same way as a key from the console.

    A keybind runs only when ImGui does not take the key. The modal reads a key press only when no
    text field has focus. That is true while the modal is the window in use.

    Two actions can have the same key combo. The modal shows such a row in the warning color. The
    order of the list decides which action gets the key. A row cannot show that by itself.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "neweditor/input/editorkeybinds.h"
#include "util/array.h"
#include "util/delegate.h"
#include "util/string.h"

namespace Editor
{

class Shell;

class KeybindsModal final : public Core::RefCounted
{
    __DeclareClass(KeybindsModal)
public:
    KeybindsModal();
    virtual ~KeybindsModal();

    /// Prepare the modal. The core calls this function at Setup. The core registers the command and
    /// the keybind
    /// there.
    bool Setup(Shell* core);
    /// Release what the modal holds.
    void Discard();

    /// Open the modal.
    void Show();

    /// Show the popup. The core calls this function once per frame, inside the ImGui frame.
    void Draw();

private:
    /// Open the modal. The keybind and the toolbar button call this function.
    void HandleShow(const Util::Array<Util::String>& args);

    /// Show one row. The row has the action, the key combo and the command. It also has the key it
    /// waits for,
    /// and the default it can go back to.
    void DrawRow(const EditorKeyBind& keybind);

    /// Return true when another action has the same key combo.
    bool Conflicts(const EditorKeyBind& keybind);

    Shell* core;

    /// true when the modal is open
    bool visible;
    /// true when the modal must open the popup on the next draw
    bool openRequested;
    /// true when the modal must close the popup on the next draw
    bool closeRequested;
};

} // namespace Editor
