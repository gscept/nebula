#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditCommandManager

    Implements a undo/redo stack manager. One manager per scene, which is what makes the history
    belong to the tab it was made in.

    The console commands and the keybinds are registered once by the editor core and run against
    whichever scene is active, because a delegate cannot point at a manager that comes and goes.

    @todo
    Implement execute for and entire CommandStack, so that we can construct macros and reuse
    them

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class Shell;

class EditCommandManager : public Core::RefCounted
{
    __DeclareClass(EditCommandManager)
public:
    struct Entry
    {
        Util::String name;
        Util::Array<Ptr<EditCommand>> commands;
    };
    typedef Util::Array<Entry> CommandList;

    /// Init
    void Setup(Shell* core);
    /// Destroy
    void Discard();
    /// Returns true if the undo stack has entries
    bool CanUndo() const;
    /// Returns true if the redo stack has entries
    bool CanRedo() const;
    /// Returns the max amount undos the stack fits
    SizeT GetUndoLevel() const;
    /// Set the max amount of undos the stack can fit
    void SetUndoLevel(SizeT newValue);
    /// Peeks at the last undo command
    Entry GetLastUndoCommand() const;
    /// Peeks at the last redo command
    Entry GetLastRedoCommand() const;
    /// Get a read only copy of the undo list
    CommandList const& GetUndoList() const;
    /// Get a read only copy of the redo list
    CommandList const& GetRedoList() const;
    /// Execute command and add to stack. Hands over ownership of the memory to the command manager.
    void Execute(Ptr<EditCommand> command);
    /// Begin macro. Subsequent commands will be bundled as one undo list entry
    void BeginMacro(const char* name);
    /// End macro.
    void EndMacro();
    /// Undo last command
    bool Undo();
    /// Redo last command
    bool Redo();
    /// Clear both redo and undo stacks
    void Clear();

private:
    Shell* core;
    CommandList undoList;
    CommandList redoList;
    Util::Array<Ptr<EditCommand>> macro;
    Util::String macroName;
    SizeT undoLevel;
    bool macroMode;
};

} // namespace Editor
