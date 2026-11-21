#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditCommand

    An undoable editor action. This is not the console command registry, which is
    Editor::EditorCommandSystem.

    A command knows how to run itself and how to put the world back. The command manager owns the
    command once it has been executed, so a command is created through Create(), filled with its
    arguments, and then handed over.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"

namespace Editor
{

class EditCommand : public Core::RefCounted
{
    __DeclareAbstractClass(EditCommand)
public:
    virtual ~EditCommand();

    /// the name the undo stack shows for this command
    virtual const char* Name() const = 0;
    /// run the action
    virtual bool Execute() = 0;
    /// run it again, which is what a redo does
    virtual bool Unexecute() = 0;

protected:
    friend class EditCommandManager;
    /// true while the command is executed. A command that holds an entity id needs this to know
    /// whether the id
    /// is its own to release when the command is destroyed
    bool executed = false;
};

} // namespace Editor
