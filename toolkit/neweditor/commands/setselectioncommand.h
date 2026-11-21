#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SetSelectionCommand

    The command a selection change is. Both sides are stored, which is what makes it undoable
    without the selection needing a stack of its own.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/selectioncontext.h"
#include "game/entity.h"
#include "util/array.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
class SetSelectionCommand : public EditCommand
{
    __DeclareClass(SetSelectionCommand)
public:
    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<SelectionContext> context;
    Util::Array<Game::Entity> oldSelection;
    Util::Array<Game::Entity> newSelection;
};

} // namespace Editor
