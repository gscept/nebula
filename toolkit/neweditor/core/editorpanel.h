#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditorPanel

    Base class for an editor window. A panel is a function that draws itself, called once per frame
    inside the ImGui frame.

    Immediate mode has no element tree to hold state, so everything the panel needs between frames
    lives on the panel: the filter buffer, which nodes are open, the scroll, the drag, the window
    flags.

    Draw is a function of the model and the panel's own state. It reads and it queues, it does not
    mutate the model, because the arrays a panel draws are built from the scene and a command run
    during the draw would change them while they are being walked. The core runs what a panel queued
    after the frame has drawn.

    The status bar is the exception: it is view state, so a panel writes to it directly through
    SetStatus().

    Adding a window requires two things: 
        1. a class deriving from this, registered with the editor core
        2. its Draw() function

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "neweditor/base/editorlog.h"
#include "util/delegate.h"
#include "util/string.h"
#include "imgui.h"

namespace Editor
{

class Shell;

class EditorPanel : public Core::RefCounted
{
    __DeclareClass(EditorPanel)
public:
    EditorPanel();
    virtual ~EditorPanel();

    /// the name of the panel, which is also the name its window is opened with
    const Util::String& GetName() const;

    /// prepare the panel. Called by the core at Open
    bool Setup(Shell* core);

    /// open the panel's window. Returns false when the panel is closed, and then no window is
    /// opened at all. For
    /// an open panel it returns false when the window is collapsed or docked behind another tab,
    /// which only means
    /// its contents are not drawn this frame: EndWindow still has to be called for every panel that
    /// is open,
    /// whatever this returns
    virtual bool BeginWindow();
    /// close it
    virtual void EndWindow();

    /// draw the panel. Called inside the window, once per frame
    virtual void Draw();

    /// the panel gained or lost focus
    virtual void OnFocus(bool focused);
    /// the editor logged something
    virtual void OnLog(LogMessageType type, const Util::String& msg);

    /// focus the panel's window
    void Focus();

    /// put a message on the status bar. An empty message clears it. Without a duration the message
    /// stays until it
    /// is replaced
    void SetStatus(const Util::String& msg, float duration = 0.0f);

    /// true when the panel's window is open
    bool IsOpen() const;
    /// open or close the panel's window. The layout keeps the position, this only decides whether
    /// it is drawn
    void SetOpen(bool open);

    Shell* GetShell() const;

    /// put a command into the panel's own command entry, for the panels that have one. A menu item
    /// that
    /// wants the path typed and edited rather than picked in the file dialog uses this
    virtual void SeedCommand(const Util::String& command);

protected:
    /// resolve what the panel needs and register its commands and keybinds. Called before the first
    /// draw
    virtual bool OnSetup() { return true; }

    /// queue an action instead of mutating the model while the panel is drawing
    void QueueAction(Util::Delegate<void()> action);

    Shell* core;
    Util::String name;
    bool open;
};

} // namespace Editor
