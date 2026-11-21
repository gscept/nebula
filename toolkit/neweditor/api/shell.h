#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::Shell

    What the parts of the editor ask for. The model, the input, the widgets, the console and the
    panels ask through this.

    The shell answers the services the core owns. It also answers what no service can: the log, the
    status bar, the queue, and the command line.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "input/key.h"
#include "input/modifier.h"
#include "neweditor/base/editorlog.h"
#include "util/array.h"
#include "util/delegate.h"
#include "util/guid.h"
#include "util/string.h"

namespace Editor
{

class Scene;
class SelectionContext;
class EditCommandManager;
class Clipboard;
class EditorCommandSystem;
class EditorKeybinds;
class ToolContext;
class ConfirmModal;

//------------------------------------------------------------------------------
/**
*/
class Shell : public Core::RefCounted
{
    __DeclareAbstractClass(Shell)
public:
    virtual ~Shell();

    /// the scene the editor is editing, which is the active one. There is always one
    virtual Ptr<Scene> GetScene() const = 0;
    /// the selection of the active scene
    virtual Ptr<SelectionContext> GetSelection() const = 0;
    /// the undo and redo stacks of the active scene
    virtual Ptr<EditCommandManager> GetCommandManager() const = 0;
    /// the editor's copy, paste and duplicate
    virtual Ptr<Clipboard> GetClipboard() const = 0;
    /// the open scenes, in tab order
    virtual const Util::Array<Ptr<Scene>>& GetScenes() const = 0;
    /// the viewport tools
    virtual Ptr<ToolContext> GetTools() const = 0;
    /// the log buffer
    virtual Ptr<EditorLog> GetLog() const = 0;
    /// the command registry
    virtual EditorCommandSystem& GetCommands() = 0;
    /// the keybind registry
    virtual EditorKeybinds& GetKeybinds() = 0;

    /// Queue an action to run after the frame has drawn.
    virtual void QueueAction(Util::Delegate<void()> action) = 0;
    /// Put a message on the status bar. An empty message clears it. Without a duration it stays
    /// until it is replaced.
    virtual void SetStatus(const Util::String& msg, float duration = 0.0f) = 0;
    /// Log a message to the log and to every panel.
    virtual void Log(LogMessageType type, const Util::String& msg) = 0;
    /// Run a command line.
    virtual bool ExecuteCommand(const Util::String& command) = 0;
    /// Run a key the host did not take for a text field. Return true when a keybind handled it.
    /// This is the only thing a host gives the editor.
    virtual bool OnKey(Input::Key::Code key, bool isPress, Input::Modifier::Code mods) = 0;
    /// true when an entity cannot be given this component because of a registered number
    virtual bool IsBlocked(Game::Entity entity, Game::ComponentId component) const = 0;
    /// true while the viewport's look drag holds the camera
    virtual bool IsCameraCapturing() const = 0;
    /// set by the viewport panel when a look drag starts and cleared when it ends
    virtual void SetCameraCapture(bool on) = 0;
    /// create an authored entity and select it, as one macro
    virtual Game::Entity CreateEntity(const Util::String& baseName, const Util::Guid& collection) = 0;
    /// delete authored entities through the command manager
    virtual bool DeleteEntities(const Util::Array<Game::Entity>& entities) = 0;
    /// Read a scene file, but a file that already has a tab goes to that tab
    virtual void OpenOrFocusSceneFile(const Util::String& path) = 0;
    /// the yes or no question
    virtual Ptr<ConfirmModal> GetConfirmModal() const = 0;
    /// One scene has written a prefab file. The other open scenes that instance it re-merge
    virtual void PrefabFileWritten(const Util::String& path, Scene* from) = 0;
};

} // namespace Editor
//------------------------------------------------------------------------------
