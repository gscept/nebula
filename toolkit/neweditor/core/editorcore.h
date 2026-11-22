#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditorCore

    The editor framework. The core owns the panels, the log, the command registry, the keybinds and
    the open scenes, and draws the panels once per frame. The core is the shell the parts ask
    through.

    The core does not know what a feature is. A feature registers what it has, and the core uses
    what is registered. A panel only draws, and what it wants to change is queued and run after the
    frame.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/clipboard.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/shell.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/widgets/statusbar.h"
#include "neweditor/viewtools/toolcontext.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/delegate.h"
#include "util/string.h"

namespace Editor
{

class EditorPanel;
class FileBrowserModal;
class ConfirmModal;
class CommandPalette;
class KeybindsModal;

class EditorCore : public Shell
{
    __DeclareClass(EditorCore)
public:
    EditorCore();
    virtual ~EditorCore();

    /// Prepare the editor. Put the style on the draw context and create the services.
    void Setup();

    /// Return the editor a feature contributes to. It is null in a build with no editor.
    static EditorCore* Instance();

    // What a feature registers goes through EditorFeatureRegistry, and Open adopts it. What is not
    // registered is never asked for.

    // -- What a feature polls while it draws --

    /// true when an entity cannot be given this component because of a registered number
    bool IsBlocked(Game::Entity entity, Game::ComponentId component) const override;

    /// Load the layout, prepare the panels and open the shell.
    bool Open();
    /// Close the panels.
    void Close();

    /// Draw the shell and the panels, then run what the panels queued.
    void Draw();

    // -- The interface a host has with the editor --

    /// Run a key the host did not take for a text field. Return true when a keybind handled it.
    bool OnKey(Input::Key::Code key, bool isPress, Input::Modifier::Code mods) override;

    /// a panel by name
    EditorPanel* GetPanel(const char* name) const;

    /// the log buffer
    Ptr<EditorLog> GetLog() const override;

    // -- Scenes. One per tab, in the order the tabs show them --

    /// Read a scene file into a new tab. The path can hold spaces, which a command line only
    /// carries in quotes.
    void OpenSceneFile(const Util::String& path);

    /// The same, but a file that already has a tab goes to that tab. It is not read a second time.
    void OpenOrFocusSceneFile(const Util::String& path) override;
    /// Write the active scene to a path. Return false when the file could not be written.
    bool SaveSceneFile(const Util::String& path);
    /// The same write for a chosen tab. Use it for a close or an exit about a scene that is not the
    /// active one.
    bool SaveSceneFile(IndexT index, const Util::String& path);
    /// Ask to exit. 'force' goes straight out, otherwise unsaved work is asked about first. The
    /// window's X and Alt-F4 come through here too.
    void RequestQuit(bool force);

    /// Create a scene, add its tab and make it the active one. Create a scene, add its tab and make
    /// it the active one.
    IndexT AddScene(const Util::String& name);
    /// Close a scene and remove its tab. Closing the last one opens a new scene, because the editor
    /// always has a scene to draw.
    void RemoveScene(IndexT index);
    /// Show a scene, which is what a tab click does. The sim world is rebuilt from it.
    void SetActiveScene(IndexT index);
    /// the tab that is showing
    IndexT GetActiveSceneIndex() const;
    /// the open scenes, in tab order
    const Util::Array<Ptr<Scene>>& GetScenes() const override;

    /// the scene the editor is editing, which is the active one. There is always one, so a panel
    /// does not check it.
    Ptr<Scene> GetScene() const override;

    /// the selection of the active scene
    Ptr<SelectionContext> GetSelection() const override;

    /// the viewport tools: the select, translate, rotate and scale tools and the one that is active
    Ptr<ToolContext> GetTools() const override;

    /// the undo and redo stacks of the active scene
    Ptr<EditCommandManager> GetCommandManager() const override;
    /// the editor's copy, paste and duplicate
    Ptr<Clipboard> GetClipboard() const override;
    /// delete authored entities through the command manager
    bool DeleteEntities(const Util::Array<Game::Entity>& entities) override;
    /// create an authored entity and select it, as one macro. Return Invalid when nothing was
    /// created
    Game::Entity CreateEntity(const Util::String& baseName, const Util::Guid& collection) override;
    /// the command registry
    EditorCommandSystem& GetCommands() override;
    /// the keybind registry
    EditorKeybinds& GetKeybinds() override;

    /// true while the viewport's look drag holds the camera. Keys pressed while it is held are
    /// camera input, not commands.
    bool IsCameraCapturing() const override;
    /// set by the viewport panel when a look drag starts and cleared when it ends
    void SetCameraCapture(bool on) override;

    /// the panel that currently has focus
    EditorPanel* GetFocusedPanel() const;
    /// focus a panel and tell the panels
    void SetFocusedPanel(EditorPanel* panel);

    /// Put a message on the status bar. An empty message clears it. Without a duration it stays
    /// until it is replaced.
    void SetStatus(const Util::String& msg, float duration = 0.0f) override;
    /// the status bar
    Ptr<StatusBar> GetStatusBar() const;

    /// the yes or no question. A panel that is about to throw work away asks it here, and the
    /// answer runs after the frame has drawn.
    Ptr<ConfirmModal> GetConfirmModal() const override;

    /// Queue an action to run after the frame has drawn.
    void QueueAction(Util::Delegate<void()> action) override;

    /// Run a command line.
    bool ExecuteCommand(const Util::String& command) override;

    /// Put a command into the console's command entry, for a menu item that wants a path typed
    /// rather than picked in the file dialog.
    void SeedConsoleCommand(const Util::String& command);

    /// One scene has written a prefab file. The other open scenes that instance it re-merge against
    /// what is on disk now.
    void PrefabFileWritten(const Util::String& path, Scene* from) override;

    /// Log a message to the log and to every panel.
    void Log(LogMessageType type, const Util::String& msg) override;

private:
    /// the dockspace, the scene tabs and the toolbar
    void DrawShell();
    /// build the shape the editor starts in when the user has no saved layout
    void BuildDefaultLayout();
    /// the row of scene tabs
    void DrawSceneTabs();
    /// the frame of reference the gizmo works in, as the tool row has it
    void DrawFrameDropdown();
    /// the grid button, which is the grid's visibility, and the popup of the settings it draws with
    void DrawGridButton();
    /// the snapping button, which is the snapping's on or off, and the popup of what it snaps to
    void DrawSnapButton();
    /// the panels button, which is the list of every panel the game registered and the dropdown that
    /// opens or closes each one
    void DrawPanelsButton(const char* glyph);
    /// run the actions the panels queued during the draw
    void FlushActions();

    /// Ask the file dialog where to write the active scene.
    void OpenSaveSceneDialog();
    /// The file dialog's Save callback. The callback takes a path and returns nothing, which is the
    /// shape the command side reads.
    void HandleSaveSceneFile(const Util::String& path);

    /// Open the file dialog for one scene. The callbacks are the caller's, because what a save
    /// means is what the caller knows.
    void OpenSaveSceneDialogFor(IndexT index, Util::Delegate<void(const Util::String&)> onAccepted,
        Util::Delegate<void()> onCanceled = Util::Delegate<void()>());

    // -- Closing a scene with unsaved work --

    /// Close the tab, after asking about unsaved work.
    void RequestCloseScene(IndexT index);
    /// 'Save' on the question: write the scene's file and close, or ask where to write when it has
    /// no file.
    void ConfirmCloseSave();
    /// 'Don't Save' on the question: close and let the work go.
    void ConfirmCloseSkip();
    /// The question was put away without an answer, so the tab stays and the question is over.
    void CloseDismissed();
    /// Save As for a scene being closed without a file, then close.
    void SaveAsThenClose(const Util::String& path);
    /// the tab the close question is about, InvalidIndex when no question is up
    IndexT pendingClose;

    // -- Exiting --

    /// 'Save' on the exit question: write every dirty scene, one at a time, and exit once none are
    /// left.
    void ConfirmQuitSave();
    /// 'Don't Save' on the exit question: exit and let the work go.
    void ConfirmQuitSkip();
    /// The exit question was put away without an answer, so the next request asks again. Without
    /// this the re-entry guard would swallow every later request.
    void QuitDismissed();
    /// Save As for a dirty scene without a file during the save-everything walk.
    void QuitSaveAsCurrent(const Util::String& path);
    /// A cancelled Save As during the walk takes the exit with it, because the answer was not yes.
    void QuitSaveAborted();
    /// End the app. This is the only path that lets the loop out.
    void QuitNow();
    /// true while the exit has been asked or is saving its way out
    bool pendingQuit;
    /// the scene the exit walk last handed a Save As dialog, InvalidIndex between dialogs
    IndexT quitSaveIndex;

    /// console commands
    void HandleScene(const Util::Array<Util::String>& args);
    void HandlePrefab(const Util::Array<Util::String>& args);
    void HandleSetComponent(const Util::Array<Util::String>& args);
    void HandleRemoveComponent(const Util::Array<Util::String>& args);
    void HandleGetComponent(const Util::Array<Util::String>& args);
    void HandleUndo(const Util::Array<Util::String>& args);
    void HandleRedo(const Util::Array<Util::String>& args);
    void HandleCopy(const Util::Array<Util::String>& args);
    void HandlePaste(const Util::Array<Util::String>& args);
    void HandleDuplicate(const Util::Array<Util::String>& args);
    void HandleDelete(const Util::Array<Util::String>& args);
    void HandleKeybind(const Util::Array<Util::String>& args);

    /// the description the command registry has for the first word of a command line
    Util::String CommandDescription(const Util::String& line);

    /// the tab a name refers to, or InvalidIndex when no open scene has that name
    IndexT FindScene(const Util::String& name) const;
    /// an authored entity of the active scene by its name, or Invalid when nothing has that name
    Game::Entity EntityNamed(const Util::String& name) const;
    /// the entity a prefab query names: the first word after the verb, or the selection when
    /// nothing was named
    Game::Entity EntityFromArgs(const Util::Array<Util::String>& args) const;

    Util::Array<Ptr<EditorPanel>> panels;
    Util::Array<Util::Delegate<void()>> actions;
    Ptr<EditorLog> log;
    Ptr<StatusBar> statusBar;
    Ptr<ToolContext> tools;
    /// the file dialog the File menu opens. It is part of the shell, not a window a user docks.
    Ptr<FileBrowserModal> fileBrowserModal;
    /// the yes or no question, for the closes that would throw work away
    Ptr<ConfirmModal> confirmModal;
    /// the command palette, which the Ctrl+Shift+P keybind opens
    Ptr<CommandPalette> commandPalette;
    /// the keybinds table, which the 'keybinds' command and the toolbar button open
    Ptr<KeybindsModal> keybindsModal;
    EditorPanel* focusedPanel;
    /// the open scenes, in tab order
    Util::Array<Ptr<Scene>> scenes;
    /// the tab that is showing
    IndexT activeScene;
    Ptr<EditorCommandSystem> commands;
    Ptr<EditorKeybinds> keybinds;
    /// the editor's copy, paste and duplicate
    Ptr<Clipboard> clipboard;
    /// true while a viewport look drag owns the mouse and keyboard
    bool cameraCapture;
    bool opened;
    /// the default shape is built once, on the first frame the dockspace is drawn
    bool layoutBuilt;

    /// the editor a feature reaches. It is null in a build with no editor, so a feature there does
    /// nothing.
    static EditorCore* instance;
};

} // namespace Editor
