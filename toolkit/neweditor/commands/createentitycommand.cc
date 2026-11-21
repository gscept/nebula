//------------------------------------------------------------------------------
//  @file createentitycommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/createentitycommand.h"

namespace Editor
{

__ImplementClass(Editor::CreateEntityCommand, 'CREC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    The id is the command's while the entity has no instance, and the world's once it does.
*/
CreateEntityCommand::~CreateEntityCommand()
{
    if (!this->executed && this->record.entity != Game::Entity::Invalid())
        this->scene->DeallocateEntityId(this->record.entity);
}

//------------------------------------------------------------------------------
/**
*/
const char*
CreateEntityCommand::Name() const
{
    return "Create entity";
}

//------------------------------------------------------------------------------
/**
*/
bool
CreateEntityCommand::Execute()
{
    if (!this->record.guid.IsValid())
        this->record.guid.Generate();

    if (this->record.entity == Game::Entity::Invalid())
        this->record.entity = this->scene->AllocateEntityId();

    this->scene->AllocateInstance(this->record);

    // The entity joins the instance its collection belongs to, which is what makes a create inside
    // a
    // prefab instance the instance's own addition
    this->scene->ResolveInstanceMembership(this->record.entity);
    return true;
}

//------------------------------------------------------------------------------
/**
    The instance goes, the id stays, because the other commands still have this entity.
*/
bool
CreateEntityCommand::Unexecute()
{
    if (this->record.entity == Game::Entity::Invalid())
        return false;

    this->scene->DeallocateInstance(this->record.entity);
    return true;
}

} // namespace Editor
