//------------------------------------------------------------------------------
//  @file setcomponentcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/setcomponentcommand.h"

namespace Editor
{

__ImplementClass(Editor::SetComponentCommand, 'SECC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
SetComponentCommand::Name() const
{
    return "Set component";
}

//------------------------------------------------------------------------------
/**
*/
bool
SetComponentCommand::Execute()
{
    if (!this->newValue.IsValid())
        return false;

    if (!this->initialized)
    {
        this->oldValue.Set(this->scene->GetComponentData(this->entity, this->component), this->newValue.Size());
        this->initialized = true;
    }

    this->scene->SetComponent(this->entity, this->component, this->newValue.GetPtr());
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
SetComponentCommand::Unexecute()
{
    if (!this->oldValue.IsValid())
        return false;

    this->scene->SetComponent(this->entity, this->component, this->oldValue.GetPtr());
    return true;
}

} // namespace Editor
