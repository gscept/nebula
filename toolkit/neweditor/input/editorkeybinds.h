#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditorKeybinds

    This class is the editor's keybind registry. A keybind maps a named action to a key, its
    modifiers, and a command line. Anything can register a keybind, so a tool that registers a
    command can bind a key to it here.

    The key and the modifiers a registration passes are the default. The bindings read from the file
    go over those defaults. The action name is the identity, so an action can be rebound without
    anything knowing where it was registered. An action the file does not have keeps its default. An
    action the file has but nothing registers is stale, and the registry warns and skips it.

    The key code and the modifier bits are the engine's, in input/key.h and input/modifier.h. A key
    comes here only when a text field does not have it.

    The file is 'bin:keybinds.json' unless something sets another path.

    A bind matches when every modifier it names is held, so more than one bind can answer the same
    key. The bind with the most modifiers wins, so ctrl+shift+z runs instead of ctrl+z. A bind with
    no modifiers names nothing and answers for any modifier. The registry is kept in action order,
    not in that order of size, because a list sorted by it moves its rows on every rebinding. The
    order the list is drawn in does not decide anything.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "input/key.h"
#include "input/modifier.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class Shell;

//------------------------------------------------------------------------------
/**
    One action and what it is bound to.
*/
struct EditorKeyBind
{
    /// the stable name the file and the command use
    Util::String action;
    /// the key the action is bound to
    Input::Key::Code key;
    /// the modifiers the bind needs, as the bits in input/modifier.h
    Input::Modifier::Code mods;
    /// the key the registration passed
    Input::Key::Code defaultKey;
    /// the modifiers the registration passed
    Input::Modifier::Code defaultMods;
    /// the command line the bind runs
    Util::String command;
};

class EditorKeybinds : public Core::RefCounted
{
    __DeclareClass(EditorKeybinds)
public:
    EditorKeybinds();
    virtual ~EditorKeybinds();

    /// Set the core the keybinds run their commands against.
    void Setup(Shell* core);
    /// Clear the registry.
    void Discard();

    /// Register a keybind. The key and the modifiers are the default the file can change.
    void Register(const Util::String& action, Input::Key::Code key, Input::Modifier::Code mods, const Util::String& command);

    /// Set the file the bindings are read from and written to, as an assign URI. Use
    /// 'bin:keybinds.json'.
    void SetPath(const Util::String& path);
    const Util::String& GetPath() const;

    /// Read the file and put what it holds over the defaults. Return false when there was nothing
    /// to read.
    bool Load();
    /// Write every binding out as it is now.
    bool Save();

    /// Rebind an action. Return false when the action is not registered.
    bool Bind(const Util::String& action, Input::Key::Code key, Input::Modifier::Code mods);
    /// Rebind an action from a combo string like 'ctrl+shift+z'.
    bool BindCombo(const Util::String& action, const Util::String& combo);
    /// Clear a binding. The action then does nothing.
    bool Unbind(const Util::String& action);
    /// Put one action back to its default. An empty action means all actions.
    void Reset(const Util::String& action);

    /// Return a key and its modifiers as the combo string the file and the command use.
    static Util::String ComboString(Input::Key::Code key, Input::Modifier::Code mods);
    /// Read a combo string back into a key and its modifiers. Return false when it cannot be read.
    static bool ComboFromString(const Util::String& combo, Input::Key::Code& key, Input::Modifier::Code& mods);

    /// Return the binding an action name refers to, or nullptr when nothing has that action.
    const EditorKeyBind* Find(const Util::String& action) const;

    /// Set the action the next key press binds. The listening ends when the key comes or the modal
    /// closes.
    void SetCapture(const Util::String& action);
    /// Stop listening.
    void ClearCapture();
    /// Return true when the registry is listening for a key.
    bool IsCapturing() const;
    const Util::String& GetCapture() const;

    /// Return the modifiers held while listening. A modifier is not a bind on its own, so listening
    /// waits for the key.
    Input::Modifier::Code GetCaptureMods() const;

    /// Run a key event. Return true when a keybind ran, so the key is not given to anything else.
    bool Process(Input::Key::Code key, bool isPress, Input::Modifier::Code mods);

    /// All registered keybinds.
    const Util::Array<EditorKeyBind>& GetKeybinds() const;

private:
    /// Log that two actions have the same combo. The order in the registry decides which one a key
    /// press runs.
    void WarnConflict(IndexT index) const;

    /// Return the index of the binding an action name refers to, or InvalidIndex.
    IndexT FindIndex(const Util::String& action) const;

    /// Return true when lhs sorts before rhs by action name. The registry keeps that order, so the
    /// list does not move when something is rebound.
    static bool ActionOrder(const EditorKeyBind& lhs, const EditorKeyBind& rhs);
    /// Keep the registry in that order.
    void Sort();

    Shell* core;
    Util::String path;
    /// the action the next key press binds, empty while nothing is listening
    Util::String capturing;
    /// the modifiers held while listening
    Input::Modifier::Code captureMods;
    Util::Array<EditorKeyBind> keybinds;
};

} // namespace Editor
//------------------------------------------------------------------------------
