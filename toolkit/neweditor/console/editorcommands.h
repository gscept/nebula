#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditorCommandSystem

    The editor command registry. Anything can register a command, and the console, the toolbar and
    the keybinds all run commands through this, so a feature that registers a command immediately
    becomes available everywhere.

    A command that is not registered is treated as a console variable, which keeps the console
    usable for reading and writing cvars without anything having to be registered.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/array.h"
#include "util/delegate.h"
#include "util/string.h"

namespace Core
{
struct CVar;
}

namespace Editor
{

class Shell;

//------------------------------------------------------------------------------
/**
*/
struct EditorCommand
{
    Util::String name;
    Util::String description;
    Util::Delegate<void(const Util::Array<Util::String>& args)> func;
};

class EditorCommandSystem : public Core::RefCounted
{
    __DeclareClass(EditorCommandSystem)
public:
    EditorCommandSystem();
    virtual ~EditorCommandSystem();

    /// the core that the command system reports to
    void Setup(Shell* core);
    /// clear the registry
    void Discard();

    /// register a command. Panels register their commands in OnSetup
    void Register(
        const Util::String& name,
        const Util::String& description,
        Util::Delegate<void(const Util::Array<Util::String>& args)> func);

    /// run a command line, where the first word is the command name
    bool Execute(const Util::String& command);

    /// all registered commands
    const Util::Array<EditorCommand>& GetCommands() const;

private:
    /// read or write a console variable
    bool EvaluateCVar(const Util::Array<Util::String>& splits);
    /// a cvar value as a string
    Util::String CVarValueAsString(Core::CVar* cVar) const;

    Shell* core;
    Util::Array<EditorCommand> commands;
};

} // namespace Editor
