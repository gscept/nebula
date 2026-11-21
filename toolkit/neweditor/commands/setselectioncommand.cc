//------------------------------------------------------------------------------
//  @file setselectioncommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/setselectioncommand.h"
#include "neweditor/model/selectioncontext.h"

namespace Editor
{

__ImplementClass(Editor::SetSelectionCommand, 'SSEC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
SetSelectionCommand::Name() const
{
    return "Set selection";
}

//------------------------------------------------------------------------------
/**
    The selection is applied through the context rather than set on it, so the panels hear about the
    change the same way they do when a click changes it.
*/
bool
SetSelectionCommand::Execute()
{
    if (this->context == nullptr)
        return false;

    this->context->Apply(this->newSelection);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
SetSelectionCommand::Unexecute()
{
    if (this->context == nullptr)
        return false;

    this->context->Apply(this->oldSelection);
    return true;
}

} // namespace Editor
