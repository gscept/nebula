//------------------------------------------------------------------------------
//  @file unpackprefabcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/unpackprefabcommand.h"

namespace Editor
{

__ImplementClass(Editor::UnpackPrefabCommand, 'UPPC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
const char*
UnpackPrefabCommand::Name() const
{
    return "Unpack prefab";
}

//------------------------------------------------------------------------------
/**
*/
bool
UnpackPrefabCommand::Execute()
{
    if (!this->initialized)
    {
        this->instanceIndex = this->scene->GetPrefabInstanceIndex(this->entity);
        if (this->instanceIndex == InvalidIndex)
            return false;

        this->instanceRecord = this->scene->GetPrefabInstance(this->entity);
        if (!this->instanceRecord.valid)
            return false;

        for (IndexT i = 0; i < this->instanceRecord.entities.Size(); i++)
        {
            const Game::Entity instanceEntity = this->instanceRecord.entities[i];
            if (instanceEntity.index >= this->scene->GetEditables().Size())
                continue;

            const Editable& editable = this->scene->GetEditable(instanceEntity);
            if (!editable.guid.IsValid())
                continue;

            EntityLink link;
            link.entity = instanceEntity;
            link.prefabEntity = editable.prefabEntity;
            link.overrides = editable.prefabOverrides;
            this->links.Append(link);
        }

        this->initialized = true;
    }

    this->scene->UnpackPrefab(this->entity);
    return true;
}

//------------------------------------------------------------------------------
/**
    The instance goes back into the slot it left, and every entity back onto it. No value was
    touched by the unpack, so there is nothing else to put back.
*/
bool
UnpackPrefabCommand::Unexecute()
{
    if (this->instanceIndex == InvalidIndex)
        return false;

    this->scene->RestorePrefabInstance(this->instanceIndex, this->instanceRecord);

    // The index is part of the link, not just the guid: an entity is an instance entity
    // by which instance slot it names, and the unpack took that out of it too
    for (IndexT i = 0; i < this->links.Size(); i++)
    {
        this->scene->SetEntityPrefabLink(this->links[i].entity, this->instanceIndex,
            this->links[i].prefabEntity, this->links[i].overrides);
    }

    return true;
}

} // namespace Editor
