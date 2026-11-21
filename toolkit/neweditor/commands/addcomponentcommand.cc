//------------------------------------------------------------------------------
//  @file addcomponentcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/addcomponentcommand.h"
#include "game/world.h"

namespace Editor
{

__ImplementClass(Editor::AddComponentCommand, 'ADDC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
AddComponentCommand::Name() const
{
    return "Add component";
}

//------------------------------------------------------------------------------
/**
*/
bool
AddComponentCommand::Execute()
{
    if (this->scene->GetAuthoringWorld()->HasComponent(this->entity, this->component))
        return false;

    this->scene->AddComponent(this->entity, this->component, this->value.IsValid() ? this->value.GetPtr() : nullptr);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
AddComponentCommand::Unexecute()
{
    this->scene->RemoveComponent(this->entity, this->component);
    return true;
}

} // namespace Editor
