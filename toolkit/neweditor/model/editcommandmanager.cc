//------------------------------------------------------------------------------
//  @file editcommandmanager.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/api/shell.h"

namespace Editor
{

__ImplementClass(Editor::EditCommandManager, 'EDMG', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
//------------------------------------------------------------------------------
/**
    The manager does not register anything. The core registers the commands and the keybinds once
    and runs them against the active scene's manager.
*/
void
EditCommandManager::Setup(Shell* core)
{
    this->core = core;
    this->undoLevel = 1024;
}

//------------------------------------------------------------------------------
/**
*/
void
EditCommandManager::Discard()
{
    this->Clear();
    this->macroMode = false;
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditCommandManager::CanUndo() const
{
    return !this->undoList.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
bool
EditCommandManager::CanRedo() const
{
    return !this->redoList.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
EditCommandManager::GetUndoLevel() const
{
    return this->undoLevel;
}

//------------------------------------------------------------------------------
/**
*/
void
EditCommandManager::SetUndoLevel(SizeT newValue)
{
    this->undoLevel = newValue;
}

//------------------------------------------------------------------------------
/**
*/
EditCommandManager::Entry
EditCommandManager::GetLastUndoCommand() const
{
    return this->undoList.Back();
}

//------------------------------------------------------------------------------
/**
*/
EditCommandManager::Entry
EditCommandManager::GetLastRedoCommand() const
{
    return this->redoList.Back();
}

//------------------------------------------------------------------------------
/**
*/
EditCommandManager::CommandList const&
EditCommandManager::GetUndoList() const
{
    return this->undoList;
}

//------------------------------------------------------------------------------
/**
*/
EditCommandManager::CommandList const&
EditCommandManager::GetRedoList() const
{
    return this->redoList;
}

//------------------------------------------------------------------------------
/**
    The command runs before it is remembered, because a command that fails should not end up on the
    undo stack.
*/
void
EditCommandManager::Execute(Ptr<EditCommand> command)
{
    n_assert(command.isvalid());

    if (!command->Execute())
        return;

    command->executed = true;

    if (this->macroMode)
    {
        this->macro.Append(command);
        return;
    }

    Entry entry;
    entry.name = Util::String(command->Name());
    entry.commands.Append(command);

    this->undoList.Append(entry);
    this->redoList.Clear();

    if ((SizeT)this->undoList.Size() > this->undoLevel)
        this->undoList.EraseIndex(0);
}

//------------------------------------------------------------------------------
/**
*/
void
EditCommandManager::BeginMacro(const char* name)
{
    this->macroMode = true;
    this->macro.Clear();
    this->macroName = name != nullptr ? Util::String(name) : Util::String("Macro");
}

//------------------------------------------------------------------------------
/**
    A macro that ran no command is not an action, so it does not become an entry.
*/
void
EditCommandManager::EndMacro()
{
    if (!this->macroMode)
        return;

    this->macroMode = false;

    if (this->macro.IsEmpty())
        return;

    Entry entry;
    entry.name = this->macroName;
    entry.commands = this->macro;
    this->macro.Clear();

    this->undoList.Append(entry);
    this->redoList.Clear();

    if ((SizeT)this->undoList.Size() > this->undoLevel)
        this->undoList.EraseIndex(0);
}

//------------------------------------------------------------------------------
/**
    The commands of an entry are undone in the order they were not done in, which is backwards.
*/
bool
EditCommandManager::Undo()
{
    if (this->undoList.IsEmpty())
        return false;

    const Entry entry = this->undoList[this->undoList.Size() - 1];
    this->undoList.EraseIndex(this->undoList.Size() - 1);

    for (IndexT i = (IndexT)entry.commands.Size() - 1; i >= 0; i--)
    {
        entry.commands[i]->Unexecute();
        entry.commands[i]->executed = false;
    }

    this->redoList.Append(entry);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditCommandManager::Redo()
{
    if (this->redoList.IsEmpty())
        return false;

    const Entry entry = this->redoList[this->redoList.Size() - 1];
    this->redoList.EraseIndex(this->redoList.Size() - 1);

    for (IndexT i = 0; i < (IndexT)entry.commands.Size(); i++)
    {
        entry.commands[i]->Execute();
        entry.commands[i]->executed = true;
    }

    this->undoList.Append(entry);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
EditCommandManager::Clear()
{
    this->undoList.Clear();
    this->redoList.Clear();
    this->macro.Clear();
    this->macroName.Clear();
}

} // namespace Editor
