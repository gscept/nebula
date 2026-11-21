//------------------------------------------------------------------------------
//  @file deleteentitycommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/deleteentitycommand.h"
#include "basegamefeature/components/basegamefeature.h"
#include "game/component.h"
#include "game/world.h"
#include "memdb/attributeregistry.h"

namespace Editor
{

__ImplementClass(Editor::DeleteEntityCommand, 'DEEC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    While the entity has no instance, the id is the command's. Releasing it here is what makes the
    entity actually go away, and it is the last thing that still holds it.
*/
DeleteEntityCommand::~DeleteEntityCommand()
{
    if (this->executed && this->entity != Game::Entity::Invalid())
        this->scene->DeallocateEntityId(this->entity);
}

//------------------------------------------------------------------------------
/**
*/
const char*
DeleteEntityCommand::Name() const
{
    return "Delete entity";
}

//------------------------------------------------------------------------------
/**
    Everything the entity is has to be read before the instance is gone, because the row the
    components live in is released with it.
*/
bool
DeleteEntityCommand::Execute()
{
    Game::World* world = this->scene->GetAuthoringWorld();
    if (this->entity == Game::Entity::Invalid() || !world->IsValid(this->entity) ||
        !this->scene->HasInstance(this->entity))
    {
        return false;
    }

    if (!this->initialized)
    {
        this->record = this->scene->GetEditable(this->entity);

        const Util::Array<Game::ComponentId>& entityComponents = this->scene->GetComponents(this->entity);
        for (IndexT i = 0; i < entityComponents.Size(); i++)
        {
            const Game::ComponentId component = entityComponents[i];
            if (component == Game::GetComponentId<Game::Entity>())
                continue;

            this->components.Append(component);

            const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
            if (typeSize > 0)
                this->values.Append(Util::Blob(this->scene->GetComponentData(this->entity, component), typeSize));
            else
                this->values.Append(Util::Blob());
        }

        const Game::ComponentId transform = Game::GetComponentId<Game::HTransform>();
        for (IndexT i = 0; i < this->scene->GetEditables().Size(); i++)
        {
            const Editable& editable = this->scene->GetEditables()[i];
            if (!editable.guid.IsValid() || editable.entity == this->entity ||
                !this->scene->HasInstance(editable.entity))
            {
                continue;
            }

            if (world->HasComponent(editable.entity, transform) &&
                world->GetComponent<Game::HTransform>(editable.entity).parent == this->entity)
            {
                this->children.Append(editable.entity);
                this->childTransforms.Append(world->GetComponent<Game::HTransform>(editable.entity));
            }
        }

        this->instanceIndex = this->record.prefabInstance;
        this->marker.record = this->record.prefabEntity;
        this->marker.name = this->record.name;
        this->marker.collection = this->record.collection;
        this->wroteMarker = this->record.prefabEntity.IsValid();

        this->initialized = true;
    }

    // An entity the file has a record for leaves a marker behind, because the instance's entity set
    // is
    // what its tree holds and the delete took an entity out of it. An entity the instance added has
    // no
    // record, so there is nothing for the delete to mark
    if (this->wroteMarker)
        this->scene->MarkRecordRemoved(this->instanceIndex, this->marker);

    this->scene->DeallocateInstance(this->entity);
    return true;
}

//------------------------------------------------------------------------------
/**
    The instance goes back at the same index, because the id was never released.
*/
bool
DeleteEntityCommand::Unexecute()
{
    if (this->entity == Game::Entity::Invalid() || !this->record.guid.IsValid())
        return false;

    if (this->wroteMarker)
        this->scene->ClearRecordRemoved(this->instanceIndex, this->marker.record);

    this->scene->AllocateInstance(this->record);

    for (IndexT i = 0; i < this->components.Size(); i++)
    {
        const Util::Blob& value = this->values[i];
        this->scene->AddComponent(this->entity, this->components[i], value.IsValid() ? value.GetPtr() : nullptr);
    }

    const Game::ComponentId transform = Game::GetComponentId<Game::HTransform>();
    for (IndexT i = 0; i < this->children.Size(); i++)
    {
        const Game::Entity child = this->children[i];
        if (!this->scene->HasInstance(child))
            continue;

        // The transform is put back as it was captured, with the values the child had while this
        // entity was
        // still its parent. Putting back the transform the child has by then would keep the parent
        // but change
        // where the child stands
        if (!this->scene->GetAuthoringWorld()->HasComponent(child, transform))
            this->scene->AddComponent(child, transform, &this->childTransforms[i]);
        else
            this->scene->SetComponent(child, transform, &this->childTransforms[i]);
    }

    return true;
}

} // namespace Editor
