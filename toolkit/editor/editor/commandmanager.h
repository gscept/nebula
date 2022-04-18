#pragma once
//------------------------------------------------------------------------------
/**
    CommandManager

    Implements a undo/redo stack manager.
  
    @todo   Implement execute for and entire CommandStack, so that we can construct
            macros and reuse them

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "core/ptr.h"
#include "command.h"

namespace Edit
{

struct CommandManager
{
    struct CommandStack
    {
        Util::String name;
        bool listAll = true;
        Util::ArrayStack<Command*, 1> commands;
    };
    //typedef Util::ArrayStack<Command*, 1> CommandStack;
    typedef Util::List<CommandStack> CommandList;

    /// Init singleton
    static void Create();
    /// Destroy singleton
    static void Discard();
    /// Returns true if the undo stack has entries
    static bool CanUndo();
    /// Returns true if the redo stack has entries
    static bool CanRedo();
    /// Returns the max amount undos the stack fits
    static SizeT GetUndoLevel();
    /// Set the max amount of undos the stack can fit
    static void SetUndoLevel(SizeT newValue);
    /// Checks if dirty
    static bool IsDirty();
    /// Peeks at the last undo command
    static CommandStack GetLastUndoCommand();
    /// Peeks at the last redo command
    static CommandStack GetLastRedoCommand();
    /// Get a read only copy of the undo list
    static CommandList const& GetUndoList();
    /// Get a read only copy of the redo list
    static CommandList const& GetRedoList();
    /// Execute command and add to stack. Hands over ownership of the memory to the command manager.
    static bool Execute(Command* command);
    /// Begin macro. Subsequent commands will be bundled as one undo list entry
    static void BeginMacro(const char* name = nullptr, bool fullHistory = true);
    /// End macro.
    static void EndMacro();
    /// Undo last command
    static void Undo();
    /// Redo last command
    static void Redo();
    /// Clear both redo and undo stacks
    static void Clear();
    /// Sets the command manager to a clean state
    static void SetClean();
    /// Clear the undo list
    static void ClearUndoList();
    /// Clear the redo list
    static void ClearRedoList();
private:
    /// Adds a command to the undo stack. Does not execute it.
    static void AddUndo(Command* command);
    /// Adds a command to the redo stack. Does not execute it.
    static void AddRedo(Command* command);
};

} // namespace Edit
