//------------------------------------------------------------------------------
//  @file instantiateprefabcommand.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/commands/instantiateprefabcommand.h"
#include "game/component.h"
#include "game/world.h"

namespace Editor
{

__ImplementClass(Editor::InstantiatePrefabCommand, 'INPC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    While the entities have no instance, their ids are the command's, and releasing them here is
    what makes the instantiation actually gone.
*/
InstantiatePrefabCommand::~InstantiatePrefabCommand()
{
    if (!this->executed)
    {
        for (IndexT i = 0; i < this->snapshots.Size(); i++)
            this->scene->DeallocateEntityId(this->snapshots[i].record.entity);
    }
}

//------------------------------------------------------------------------------
/**
*/
const char*
InstantiatePrefabCommand::Name() const
{
    return "Instantiate prefab";
}

//------------------------------------------------------------------------------
/**
*/
bool
InstantiatePrefabCommand::Execute()
{
    if (this->snapshots.IsEmpty())
        return this->InstantiateFromDocument();

    this->InstantiateFromSnapshots();
    return true;
}

//------------------------------------------------------------------------------
/**
    The instances go and the ids stay, the way the create command puts an entity out. The scene
    keeps the prefab instance itself, with its entities, so the redo has somewhere to put them back
    into, and the instance is marked not valid while they are out, which is what keeps the scene
    from treating them as an instantiation.
*/
bool
InstantiatePrefabCommand::Unexecute()
{
    for (IndexT i = 0; i < this->snapshots.Size(); i++)
        this->scene->DeallocateInstance(this->snapshots[i].record.entity);

    // The collections go with the entities they were made for. Backwards, which takes a
    // nested one before the one it hangs under
    for (IndexT i = (IndexT)this->collections.Size() - 1; i >= 0; i--)
        this->scene->DeleteCollection(this->collections[i].guid);

    for (IndexT i = 0; i < this->snapshots.Size(); i++)
    {
        if (this->snapshots[i].record.prefabInstance != InvalidIndex)
        {
            this->scene->SetPrefabInstanceValid(this->snapshots[i].record.prefabInstance, false);
            break;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
InstantiatePrefabCommand::InstantiateFromDocument()
{
    Util::Array<Game::Entity>& created = this->created;
    created.Clear();
    if (!this->scene->InstantiatePrefab(this->path, this->collection, created))
        return false;

    // the collections the file brought become part of the instantiation's record, so a
    // redo puts back the same ones the entity snapshots name
    if (!created.IsEmpty())
        this->CaptureCollections(this->scene->GetPrefabInstance(created[0]));

    // The instantiation's roots are the entities nothing else in it is the parent of. They
    // take the name the command was given, which is what lets 'Enemy' from the file be 'Boss'
    // in the level without renaming the file
    if (!this->rootName.IsEmpty())
    {
        for (IndexT i = 0; i < created.Size(); i++)
        {
            const Game::Entity parent = this->scene->GetParent(created[i]);
            bool parentIsPartOfInstance = false;
            for (IndexT c = 0; c < created.Size(); c++)
            {
                if (created[c] == parent)
                {
                    parentIsPartOfInstance = true;
                    break;
                }
            }
            if (!parentIsPartOfInstance)
                this->scene->SetEntityName(created[i], this->scene->UniqueEntityName(this->rootName));
        }
    }

    for (IndexT i = 0; i < created.Size(); i++)
    {
        EntitySnapshot snapshot;
        snapshot.record = this->scene->GetEditable(created[i]);

        const Util::Array<Game::ComponentId>& components = this->scene->GetComponents(created[i]);
        for (IndexT c = 0; c < components.Size(); c++)
        {
            const Game::ComponentId component = components[c];
            if (component == Game::GetComponentId<Game::Entity>())
                continue;

            snapshot.components.Append(component);

            const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
            if (typeSize > 0)
                snapshot.values.Append(Util::Blob(this->scene->GetComponentData(created[i], component), typeSize));
            else
                snapshot.values.Append(Util::Blob());
        }

        this->snapshots.Append(snapshot);
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
InstantiatePrefabCommand::InstantiateFromSnapshots()
{
    // The collections come back with the same guids, which is what makes the collection
    // fields of the entity records name something again
    for (IndexT i = 0; i < this->collections.Size(); i++)
        this->scene->CreateCollection(this->collections[i], InvalidIndex);

    for (IndexT i = 0; i < this->snapshots.Size(); i++)
    {
        const EntitySnapshot& snapshot = this->snapshots[i];
        this->scene->AllocateInstance(snapshot.record);

        // The undo took the instance out of the scene. Its entities are back, so it is one again
        if (snapshot.record.prefabInstance != InvalidIndex)
            this->scene->SetPrefabInstanceValid(snapshot.record.prefabInstance, true);

        for (IndexT c = 0; c < snapshot.components.Size(); c++)
        {
            const Util::Blob& value = snapshot.values[c];
            this->scene->AddComponent(snapshot.record.entity, snapshot.components[c],
                value.IsValid() ? value.GetPtr() : nullptr);
        }
    }
}

//------------------------------------------------------------------------------
/**
    The records as the scene holds them, which is what a redo creates again.
*/
void
InstantiatePrefabCommand::CaptureCollections(const PrefabInstance& instance)
{
    for (IndexT i = 0; i < instance.collections.Size(); i++)
    {
        const IndexT index = this->scene->FindCollection(instance.collections[i]);
        if (index == InvalidIndex)
            continue;

        this->collections.Append(this->scene->GetCollections()[index]);
    }
}

} // namespace Editor
