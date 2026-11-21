//------------------------------------------------------------------------------
//  @file setentitynamecommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/setentitynamecommand.h"

namespace Editor
{

__ImplementClass(Editor::SetEntityNameCommand, 'SENC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
SetEntityNameCommand::Name() const
{
    return "Set entity name";
}

//------------------------------------------------------------------------------
/**
    An entity that is not authored has no name to change, so this is the check that decides whether
    the command ran.
*/
bool
SetEntityNameCommand::Execute()
{
    if (!this->initialized)
    {
        if (!this->scene->GetEditable(this->entity).guid.IsValid())
            return false;

        this->oldName = this->scene->GetEditable(this->entity).name;
        this->initialized = true;
    }

    return this->scene->SetEntityName(this->entity, this->newName);
}

//------------------------------------------------------------------------------
/**
*/
bool
SetEntityNameCommand::Unexecute()
{
    return this->scene->SetEntityName(this->entity, this->oldName);
}

} // namespace Editor
