//------------------------------------------------------------------------------
//  @file revertrecordcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/revertrecordcommand.h"

namespace Editor
{

__ImplementClass(Editor::RevertRecordCommand, 'RVRC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    A revert that never ran allocated nothing, so there is no id to give back.
*/
RevertRecordCommand::~RevertRecordCommand()
{
    if (!this->executed && this->entity != Game::Entity::Invalid())
        this->scene->DeallocateEntityId(this->entity);
}

//------------------------------------------------------------------------------
/**
*/
const char*
RevertRecordCommand::Name() const
{
    return "Revert to prefab";
}

//------------------------------------------------------------------------------
/**
    The entity is made on the first run and the id is kept, so a redo puts the same entity back with
    the same identity.
*/
bool
RevertRecordCommand::Execute()
{
    if (!this->initialized)
    {
        this->entityGuid.Generate();
        this->entity = this->scene->AllocateEntityId();
        if (!this->scene->RevertRecord(this->instanceIndex, this->record, this->entityGuid, this->entity))
            return false;

        this->marker.record = this->record;
        this->marker.name = this->scene->GetEditable(this->entity).name;
        this->marker.collection = this->scene->GetEditable(this->entity).collection;

        this->initialized = true;
        return true;
    }

    return this->scene->RevertRecord(this->instanceIndex, this->record, this->entityGuid, this->entity);
}

//------------------------------------------------------------------------------
/**
    The marker goes back first, because the entity is what the record names, and only the instance
    in the world goes: the id stays with the command, and the instance keeps its entities.
*/
bool
RevertRecordCommand::Unexecute()
{
    if (this->entity == Game::Entity::Invalid())
        return false;

    this->scene->MarkRecordRemoved(this->instanceIndex, this->marker);
    this->scene->DeallocateInstance(this->entity);
    return true;
}

} // namespace Editor
