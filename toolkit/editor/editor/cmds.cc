//------------------------------------------------------------------------------
//  cmds.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "cmds.h"
#include "commandmanager.h"
#include "util/random.h"
#include "editor/tools/selectioncontext.h"
#include "graphicsfeature/managers/graphicsmanager.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/managers/hierarchymanager.h"
#include "editor/components/editorcomponents.h"

namespace Edit
{

//------------------------------------------------------------------------------
/**
*/
bool
InternalCreateEntity(Editor::Entity id, const Util::Guid& collection, uint collectionOrder)
{
    n_assert(Editor::state.editorWorld->IsValid(id));

    if (Editor::state.editorWorld->HasInstance(id))
    {
        n_warning("Entity already has an instance!\n");
        return false;
    }

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    Game::Entity const entity = gameWorld->CreateEntity(true);

    Editor::EditorEntity* editorEntityComponent = gameWorld->AddComponent<Editor::EditorEntity>(entity);
    editorEntityComponent->id = (uint64_t)id;

    if (Editor::state.editables.Size() >= id.index)
        Editor::state.editables.Append({});
    Editor::state.editorWorld->AllocateInstance(id);

    Editor::Editable& edit = Editor::state.editables[id.index];
    edit.gameEntity = entity;
    edit.name = Util::String("Entity ");
    edit.name.AppendInt(id.index);
    edit.collection = collection;
    edit.collectionOrder = collectionOrder;

    edit.version++;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
InternalCreateEntity(Editor::Entity editorEntity, MemDb::TableId editorTable, Util::Blob entityState)
{
    n_assert(Editor::state.editorWorld->IsValid(editorEntity));

    if (Editor::state.editorWorld->HasInstance(editorEntity))
    {
        n_warning("Entity already has an instance!\n");
        return false;
    }

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);

    MemDb::Table& editorTableInstance = Editor::state.editorWorld->GetDatabase()->GetTable(editorTable);
    MemDb::TableSignature const& signature = editorTableInstance.GetSignature();

    Game::Entity const entity = gameWorld->AllocateEntityId();

    if (Editor::state.editables.Size() <= editorEntity.index)
        Editor::state.editables.Resize(editorEntity.index + 1);

    Editor::Editable& edit = Editor::state.editables[editorEntity.index];
    edit.gameEntity = entity;

    MemDb::TableId gameTable = gameWorld->GetDatabase()->FindTable(signature);
    if (gameTable == MemDb::InvalidTableId)
    {
        Game::EntityTableCreateInfo info;
        info.components = editorTableInstance.GetAttributes();
        info.name = editorTableInstance.name.Value();
        gameTable = gameWorld->CreateEntityTable(info);
    }

    MemDb::RowId const gameInstance = gameWorld->AllocateInstance(entity, gameTable, &entityState);
    Editor::RemapInstanceToGame(gameWorld, entity);
    gameWorld->InitializeInstance(entity, gameTable, gameInstance);

    Editor::EditorEntity* editorEntityComponent = gameWorld->AddComponent<Editor::EditorEntity>(entity);
    editorEntityComponent->id = (uint64_t)editorEntity;

    MemDb::RowId editorInstance = Editor::state.editorWorld->AllocateInstance(editorEntity, editorTable);
    editorTableInstance.DeserializeInstance(entityState, editorInstance);
    Editor::state.editorWorld->SetComponent<Game::Entity>(editorEntity, editorEntity);

    edit.version++;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
InternalDestroyEntity(Editor::Entity editorEntity)
{
    n_assert(Editor::state.editorWorld->IsValid(editorEntity));
    n_assert(Editor::state.editorWorld->HasInstance(editorEntity));

    Game::EntityMapping const mapping = Editor::state.editorWorld->GetEntityMapping(editorEntity);
    Editor::Editable& edit = Editor::state.editables[editorEntity.index];

    if (Game::GetWorld(WORLD_DEFAULT)->IsValid(edit.gameEntity))
        Game::GetWorld(WORLD_DEFAULT)->DeleteEntity(edit.gameEntity);

    edit.gameEntity = Game::Entity::Invalid();

    Editor::state.editorWorld->DeallocateInstance(editorEntity);

    // Make sure the editor world is always defragged
    Editor::state.editorWorld->Defragment(mapping.table);

    edit.version++;
}

//------------------------------------------------------------------------------
/**
*/
bool
InternalSetProperty(Editor::Entity editorEntity, Game::ComponentId component, void* value, size_t size)
{
    n_assert(Editor::state.editorWorld->IsValid(editorEntity));
    n_assert(Editor::state.editorWorld->HasInstance(editorEntity));

    Game::EntityMapping mapping = Editor::state.editorWorld->GetEntityMapping(editorEntity);
    Editor::Editable& edit = Editor::state.editables[editorEntity.index];

    Editor::state.editorWorld->SetComponentValue(editorEntity, component, value, size);
    
    Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);
    if (defaultWorld->IsValid(edit.gameEntity))
    {
        Util::Blob gameValue(value, size);
        Editor::RemapComponentToGame(component, gameValue.GetPtr());
        defaultWorld->ReinitializeComponent(edit.gameEntity, component, gameValue.GetPtr(), size);
        defaultWorld->MarkAsModified(edit.gameEntity);
    }

    edit.version++;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
InternalAddProperty(Editor::Entity editorEntity, Game::ComponentId component, void* value)
{
    n_assert(Editor::state.editorWorld->IsValid(editorEntity));

    Editor::Editable& edit = Editor::state.editables[editorEntity.index];

    if (Editor::state.editorWorld->HasComponent(editorEntity, component))
        return false;

    SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
    void* editorValue = Editor::state.editorWorld->AddComponent(editorEntity, component, value);

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    Util::Blob gameData(editorValue, typeSize);
    if (typeSize > 0)
    {
        Editor::RemapComponentToGame(component, gameData.GetPtr());
    }
    gameWorld->AddComponent(edit.gameEntity, component, gameData.GetPtr());
    
    edit.version++;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
InternalRemoveProperty(Editor::Entity editorEntity, Game::ComponentId component)
{
    n_assert(Editor::state.editorWorld->IsValid(editorEntity));

    Editor::Editable& edit = Editor::state.editables[editorEntity.index];

    if (!Editor::state.editorWorld->HasComponent(editorEntity, component))
        return false;

    Editor::state.editorWorld->RemoveComponent(editorEntity, component);
    Game::GetWorld(WORLD_DEFAULT)->RemoveComponent(edit.gameEntity, component);
    
    edit.version++;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
struct CMDCreateEntity : public Edit::Command
{
    ~CMDCreateEntity()
    {
        if (!executed)
            Editor::state.editorWorld->DeallocateEntityId(this->id);
    };
    const char*
    Name() override
    {
        return "Create Entity";
    };
    bool
    Execute() override
    {
        if (this->id == Editor::Entity::Invalid())
            this->id = Editor::state.editorWorld->AllocateEntityId();
        while (Editor::state.editables.Size() <= this->id.index)
            Editor::state.editables.Append({});
        if (!initialized)
        {
            Editor::state.editables[this->id.index].guid.Generate();
            initialized = true;
        }

        return InternalCreateEntity(id, collection, collectionOrder);
    };
    bool
    Unexecute() override
    {
        InternalDestroyEntity(id);
        return true;
    };
    Editor::Entity id;
    Util::Guid collection;
    uint collectionOrder = 0;

private:
    bool initialized = false;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDDuplicateEntity : public Edit::Command
{
    ~CMDDuplicateEntity()
    {
        if (!executed)
            Editor::state.editorWorld->DeallocateEntityId(this->duplicated);
    };
    const char*
    Name() override
    {
        return "Duplicate Entity";
    };
    bool
    Execute() override
    {
        if (this->duplicated == Editor::Entity::Invalid())
            this->duplicated = Editor::state.editorWorld->AllocateEntityId();
        while (Editor::state.editables.Size() <= this->duplicated.index)
            Editor::state.editables.Append({});
        
        Game::EntityMapping mapping = Editor::state.editorWorld->GetEntityMapping(this->id);
        
        if (!this->initialized)
        {
            Editor::state.editables[this->duplicated.index].guid.Generate();
            this->entityState =
                Editor::state.editorWorld->GetDatabase()->GetTable(mapping.table).SerializeInstance(mapping.instance);
            this->initialized = true;
        }

        Editor::state.editables[this->duplicated.index].name = Editor::state.editables[this->id.index].name;

        return InternalCreateEntity(this->duplicated, mapping.table, this->entityState);
    };
    bool
    Unexecute() override
    {
        InternalDestroyEntity(this->duplicated);
        return true;
    };

    Editor::Entity id;
    Editor::Entity duplicated = Editor::Entity::Invalid();
    Util::Guid collection;
    uint collectionOrder = 0;
    
private:
    bool initialized = false;
    Util::Blob entityState;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDDeleteEntity : public Edit::Command
{
    ~CMDDeleteEntity()
    {
        if (executed)
            Editor::state.editorWorld->DeallocateEntityId(this->id);
    };
    const char*
    Name() override
    {
        return "Delete Entity";
    };
    bool
    Execute() override
    {
        if (this->id == Editor::Entity::Invalid())
            return false;
        if (!this->initialized)
        {
            Game::EntityMapping mapping = Editor::state.editorWorld->GetEntityMapping(this->id);
            this->entityState =
                Editor::state.editorWorld->GetDatabase()->GetTable(mapping.table).SerializeInstance(mapping.instance);
            this->tid = mapping.table;
            Game::Filter filter = Game::FilterBuilder().Including<Game::Entity, Game::HTransform>().Build();
            Game::Dataset data = Editor::state.editorWorld->Query(filter);
            for (int viewIndex = 0; viewIndex < data.numViews; viewIndex++)
            {
                Game::Dataset::View const& view = data.views[viewIndex];
                Editor::Entity const* const entities = (Editor::Entity*)view.buffers[0];
                Game::HTransform const* const transforms = (Game::HTransform*)view.buffers[1];
                for (IndexT i = 0; i < view.numInstances; i++)
                {
                    if (view.validInstances.IsSet(i) && transforms[i].parent == this->id)
                    {
                        this->children.Append(entities[i]);
                        this->childTransforms.Append(transforms[i]);
                    }
                }
            }
            Game::DestroyFilter(filter);
            this->initialized = true;
        }
        InternalDestroyEntity(this->id);
        return true;
    };
    bool
    Unexecute() override
    {
        InternalCreateEntity(this->id, this->tid, this->entityState);
        for (IndexT i = 0; i < this->children.Size(); i++)
        {
            if (!Editor::state.editorWorld->IsValid(this->children[i]) || !Editor::state.editorWorld->HasInstance(this->children[i]))
            {
                continue;
            }
            if (Editor::state.editorWorld->HasComponent<Game::HTransform>(this->children[i]))
            {
                InternalSetProperty(
                    this->children[i],
                    Game::GetComponentId<Game::HTransform>(),
                    &this->childTransforms[i],
                    sizeof(Game::HTransform)
                );
            }
            else
            {
                InternalAddProperty(this->children[i], Game::GetComponentId<Game::HTransform>(), &this->childTransforms[i]);
            }
        }
        return true;
    };
    Editor::Entity id;

private:
    bool initialized = false;
    Util::Blob entityState;
    MemDb::TableId tid;
    Util::Array<Editor::Entity> children;
    Util::Array<Game::HTransform> childTransforms;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDSetSelection : public Edit::Command
{
    const char*
    Name() override
    {
        return "Set selection";
    };
    bool
    Execute() override
    {
        if (!this->initialized)
        {
            this->oldSelection = Tools::SelectionContext::Selection();
            this->initialized = true;
        }
        Tools::SelectionContext::Instance()->selection = newSelection;
        Tools::SelectionContext::Instance()->selection.Sort();
        return true;
    };
    bool
    Unexecute() override
    {
        Tools::SelectionContext::Instance()->selection = oldSelection;
        Tools::SelectionContext::Instance()->selection.Sort();
        return true;
    };
    Util::Array<Editor::Entity> newSelection;

private:
    Util::Array<Editor::Entity> oldSelection;
    bool initialized = false;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDSetProperty : public Edit::Command
{
    ~CMDSetProperty() {};
    const char*
    Name() override
    {
        return "Set property";
    };
    bool
    Execute() override
    {
        if (!newValue.IsValid())
            return false;
        if (!oldValue.IsValid())
        {
            Game::EntityMapping const mapping = Editor::state.editorWorld->GetEntityMapping(id);
            MemDb::TableId const tid = mapping.table;
            Ptr<MemDb::Database> editorWorldDB = Editor::state.editorWorld->GetDatabase();
            void* oldValuePtr = editorWorldDB->GetTable(tid).GetValuePointer(
                editorWorldDB->GetTable(tid).GetAttributeIndex(component), mapping.instance
            );
            oldValue.Set(oldValuePtr, newValue.Size());
        }
        return InternalSetProperty(id, component, newValue.GetPtr(), newValue.Size());
    };
    bool
    Unexecute() override
    {
        return InternalSetProperty(id, component, oldValue.GetPtr(), oldValue.Size());
    };
    Editor::Entity id;
    Game::ComponentId component;
    Util::Blob newValue;

private:
    Util::Blob oldValue;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDSetParent : public Edit::Command
{
    const char*
    Name() override
    {
        return this->newHasTransform ? "Set parent" : "Clear parent";
    };
    bool
    Execute() override
    {
        bool result;
        if (this->newHasTransform)
        {
            if (Editor::state.editorWorld->HasComponent<Game::HTransform>(this->child))
            {
                return InternalSetProperty(this->child, Game::GetComponentId<Game::HTransform>(), &this->newTransform, sizeof(this->newTransform));
            }
            result = InternalAddProperty(this->child, Game::GetComponentId<Game::HTransform>(), &this->newTransform);
            Editor::state.editorWorld->ExecuteAddComponentCommands();
            Game::GetWorld(WORLD_DEFAULT)->ExecuteAddComponentCommands();
            return result;
        }
        result = InternalRemoveProperty(this->child, Game::GetComponentId<Game::HTransform>());
        Editor::state.editorWorld->ExecuteRemoveComponentCommands();
        Game::GetWorld(WORLD_DEFAULT)->ExecuteRemoveComponentCommands();
        return result;
    };
    bool
    Unexecute() override
    {
        if (this->oldHasTransform)
        {
            if (Editor::state.editorWorld->HasComponent<Game::HTransform>(this->child))
            {
                return InternalSetProperty(this->child, Game::GetComponentId<Game::HTransform>(), &this->oldTransform, sizeof(this->oldTransform));
            }
            bool const result = InternalAddProperty(this->child, Game::GetComponentId<Game::HTransform>(), &this->oldTransform);
            Editor::state.editorWorld->ExecuteAddComponentCommands();
            Game::GetWorld(WORLD_DEFAULT)->ExecuteAddComponentCommands();
            return result;
        }
        bool const result = InternalRemoveProperty(this->child, Game::GetComponentId<Game::HTransform>());
        Editor::state.editorWorld->ExecuteRemoveComponentCommands();
        Game::GetWorld(WORLD_DEFAULT)->ExecuteRemoveComponentCommands();
        return result;
    };
    Editor::Entity child;
    bool oldHasTransform = false;
    bool newHasTransform = false;
    Game::HTransform oldTransform;
    Game::HTransform newTransform;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDAddProperty : public Edit::Command
{
    ~CMDAddProperty() {};
    const char*
    Name() override
    {
        return "Add property";
    };
    bool
    Execute() override
    {
        return InternalAddProperty(id, component, nullptr);
    };
    bool
    Unexecute() override
    {
        return InternalRemoveProperty(id, component);
    };
    Editor::Entity id;
    Game::ComponentId component;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDRemoveProperty : public Edit::Command
{
    ~CMDRemoveProperty() {};
    const char*
    Name() override
    {
        return "Remove property";
    };
    bool
    Execute() override
    {
        if (!value.IsValid() && MemDb::AttributeRegistry::TypeSize(component) != 0)
        {
            Game::EntityMapping const mapping = Editor::state.editorWorld->GetEntityMapping(id);
            MemDb::TableId const tid = mapping.table;
            Ptr<MemDb::Database> editorWorldDB = Editor::state.editorWorld->GetDatabase();
            void* valuePtr = editorWorldDB->GetTable(tid).GetValuePointer(
                editorWorldDB->GetTable(tid).GetAttributeIndex(component), mapping.instance
            );
            value.Set(valuePtr, MemDb::AttributeRegistry::TypeSize(component));
        }
        return InternalRemoveProperty(id, component);
    };
    bool
    Unexecute() override
    {
        return InternalAddProperty(id, component, value.IsValid() ? value.GetPtr() : nullptr);
    };
    Editor::Entity id;
    Game::ComponentId component;

private:
    Util::Blob value;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDSetEntityName : public Edit::Command
{
    const char*
    Name() override
    {
        return "Set entity name";
    };
    bool
    Execute() override
    {
        if (oldName.IsEmpty())
            oldName = Editor::state.editables[id.index].name;

        Editor::state.editables[id.index].name = newName;
        return true;
    };
    bool
    Unexecute() override
    {
        Editor::state.editables[id.index].name = oldName;
        return true;
    };
    Editor::Entity id;
    Util::String newName;

private:
    Util::String oldName;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDCreateCollection : public Edit::Command
{
    const char*
    Name() override
    {
        return "Create collection";
    };
    bool
    Execute() override
    {
        if (Editor::FindCollection(this->collection.guid) != InvalidIndex)
        {
            return false;
        }
        Editor::state.collections.Append(this->collection);
        return true;
    };
    bool
    Unexecute() override
    {
        IndexT const index = Editor::FindCollection(this->collection.guid);
        if (index == InvalidIndex)
        {
            return false;
        }
        Editor::state.collections.EraseIndex(index);
        return true;
    };
    Editor::Collection collection;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDSetCollectionName : public Edit::Command
{
    const char*
    Name() override
    {
        return "Set collection name";
    };
    bool
    Execute() override
    {
        IndexT const index = Editor::FindCollection(this->collection);
        if (index == InvalidIndex)
        {
            return false;
        }
        if (!this->initialized)
        {
            this->oldName = Editor::state.collections[index].name;
            this->initialized = true;
        }
        Editor::state.collections[index].name = this->newName;
        return true;
    };
    bool
    Unexecute() override
    {
        IndexT const index = Editor::FindCollection(this->collection);
        if (index == InvalidIndex)
        {
            return false;
        }
        Editor::state.collections[index].name = this->oldName;
        return true;
    };
    Util::Guid collection;
    Util::String newName;

private:
    bool initialized = false;
    Util::String oldName;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDSetCollectionParent : public Edit::Command
{
    const char*
    Name() override
    {
        return "Set collection parent";
    };
    bool
    Execute() override
    {
        IndexT const index = Editor::FindCollection(this->collection);
        if (index == InvalidIndex)
        {
            return false;
        }
        if (!this->initialized)
        {
            this->oldParent = Editor::state.collections[index].parent;
            this->initialized = true;
        }
        Editor::state.collections[index].parent = this->newParent;
        return true;
    };
    bool
    Unexecute() override
    {
        IndexT const index = Editor::FindCollection(this->collection);
        if (index == InvalidIndex)
        {
            return false;
        }
        Editor::state.collections[index].parent = this->oldParent;
        return true;
    };
    Util::Guid collection;
    Util::Guid newParent;

private:
    bool initialized = false;
    Util::Guid oldParent;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDMoveEntitiesToCollection : public Edit::Command
{
    const char*
    Name() override
    {
        return "Move entities to collection";
    };
    bool
    Execute() override
    {
        if (!this->initialized)
        {
            for (Editor::Entity entity : this->entities)
            {
                Editor::Editable const& editable = Editor::state.editables[entity.index];
                this->oldCollections.Append(editable.collection);
                this->oldOrders.Append(editable.collectionOrder);
            }
            this->initialized = true;
        }

        for (IndexT i = 0; i < this->entities.Size(); i++)
        {
            Editor::Editable& editable = Editor::state.editables[this->entities[i].index];
            editable.collection = this->newCollection;
            editable.collectionOrder = this->firstOrder + i;
            editable.version++;
        }
        return true;
    };
    bool
    Unexecute() override
    {
        for (IndexT i = 0; i < this->entities.Size(); i++)
        {
            Editor::Editable& editable = Editor::state.editables[this->entities[i].index];
            editable.collection = this->oldCollections[i];
            editable.collectionOrder = this->oldOrders[i];
            editable.version++;
        }
        return true;
    };
    Util::Array<Editor::Entity> entities;
    Util::Guid newCollection;
    uint firstOrder = 0;

private:
    bool initialized = false;
    Util::Array<Util::Guid> oldCollections;
    Util::Array<uint> oldOrders;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDDeleteCollection : public Edit::Command
{
    const char*
    Name() override
    {
        return "Delete collection";
    };
    bool
    Execute() override
    {
        IndexT const index = Editor::FindCollection(this->collectionGuid);
        if (index == InvalidIndex)
        {
            return false;
        }
        if (!this->initialized)
        {
            this->collection = Editor::state.collections[index];
            this->collectionIndex = index;
            for (IndexT i = 0; i < Editor::state.collections.Size(); i++)
            {
                if (Editor::state.collections[i].parent == this->collectionGuid)
                {
                    this->childCollections.Append(Editor::state.collections[i].guid);
                }
            }
            for (IndexT i = 0; i < Editor::state.editables.Size(); i++)
            {
                if (Editor::state.editables[i].collection == this->collectionGuid)
                {
                    this->entities.Append(i);
                }
            }
            this->initialized = true;
        }

        for (Util::Guid const& child : this->childCollections)
        {
            IndexT const childIndex = Editor::FindCollection(child);
            if (childIndex != InvalidIndex)
            {
                Editor::state.collections[childIndex].parent = this->collection.parent;
            }
        }
        for (IndexT const entityIndex : this->entities)
        {
            Editor::state.editables[entityIndex].collection = this->collection.parent;
            Editor::state.editables[entityIndex].version++;
        }
        Editor::state.collections.EraseIndex(index);
        return true;
    };
    bool
    Unexecute() override
    {
        if (Editor::FindCollection(this->collectionGuid) != InvalidIndex)
        {
            return false;
        }
        Editor::state.collections.Insert(this->collectionIndex, this->collection);
        for (Util::Guid const& child : this->childCollections)
        {
            IndexT const childIndex = Editor::FindCollection(child);
            if (childIndex != InvalidIndex)
            {
                Editor::state.collections[childIndex].parent = this->collectionGuid;
            }
        }
        for (IndexT const entityIndex : this->entities)
        {
            Editor::state.editables[entityIndex].collection = this->collectionGuid;
            Editor::state.editables[entityIndex].version++;
        }
        return true;
    };
    Util::Guid collectionGuid;

private:
    bool initialized = false;
    IndexT collectionIndex = InvalidIndex;
    Editor::Collection collection;
    Util::Array<Util::Guid> childCollections;
    Util::Array<IndexT> entities;
};

//------------------------------------------------------------------------------
/**
*/
Editor::Entity
CreateEntity()
{
    CMDCreateEntity* cmd = new CMDCreateEntity;
    Editor::Entity const entity = Editor::state.editorWorld->AllocateEntityId();
    cmd->id = entity;
    cmd->collection = Editor::FindCollection(Editor::state.activeCollection) != InvalidIndex
                          ? Editor::state.activeCollection
                          : Util::Guid();
    for (Editor::Editable const& editable : Editor::state.editables)
    {
        if (editable.gameEntity != Game::Entity::Invalid() && editable.collection == cmd->collection)
        {
            cmd->collectionOrder = Math::max(cmd->collectionOrder, editable.collectionOrder + 1);
        }
    }
    CommandManager::Execute(cmd);
    return entity;
}

//------------------------------------------------------------------------------
/**
    @todo We should duplicate the collection as well?
*/
Editor::Entity
DuplicateEntity(Editor::Entity entity)
{
    CMDDuplicateEntity* cmd = new CMDDuplicateEntity;
    cmd->id = entity;
    CommandManager::Execute(cmd);
    return cmd->duplicated;
}

//------------------------------------------------------------------------------
/**
*/
void
DeleteEntity(Editor::Entity entity)
{
    CMDDeleteEntity* cmd = new CMDDeleteEntity;
    cmd->id = entity;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
SetSelection(Util::Array<Editor::Entity> const& entities)
{
    CMDSetSelection* cmd = new CMDSetSelection;
    cmd->newSelection = entities;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
SetComponent(Editor::Entity entity, Game::ComponentId component, void* value)
{
    CMDSetProperty* cmd = new CMDSetProperty;
    cmd->id = entity;
    cmd->component = component;
    cmd->newValue.Set(value, MemDb::AttributeRegistry::TypeSize(component));
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
AddComponent(Editor::Entity entity, Game::ComponentId component)
{
    CMDAddProperty* cmd = new CMDAddProperty;
    cmd->id = entity;
    cmd->component = component;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
RemoveComponent(Editor::Entity entity, Game::ComponentId component)
{
    CMDRemoveProperty* cmd = new CMDRemoveProperty;
    cmd->id = entity;
    cmd->component = component;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
SetEntityName(Editor::Entity entity, Util::String const& name)
{
    CMDSetEntityName* cmd = new CMDSetEntityName;
    cmd->id = entity;
    cmd->newName = name;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
Util::Guid
CreateCollection(const Util::String& name, const Util::Guid& parent)
{
    if (parent.IsValid() && Editor::FindCollection(parent) == InvalidIndex)
    {
        return Util::Guid();
    }

    CMDCreateCollection* cmd = new CMDCreateCollection;
    cmd->collection.guid.Generate();
    cmd->collection.name = name;
    cmd->collection.parent = parent;
    cmd->collection.order = 0;
    for (Editor::Collection const& collection : Editor::state.collections)
    {
        if (collection.parent == parent)
        {
            cmd->collection.order = Math::max(cmd->collection.order, collection.order + 1);
        }
    }
    Util::Guid const guid = cmd->collection.guid;
    if (!CommandManager::Execute(cmd))
    {
        return Util::Guid();
    }
    return guid;
}

//------------------------------------------------------------------------------
/**
*/
void
SetCollectionName(const Util::Guid& collection, const Util::String& name)
{
    CMDSetCollectionName* cmd = new CMDSetCollectionName;
    cmd->collection = collection;
    cmd->newName = name;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
bool
SetCollectionParent(const Util::Guid& collection, const Util::Guid& parent)
{
    if (Editor::FindCollection(collection) == InvalidIndex || collection == parent)
    {
        return false;
    }

    Util::Guid ancestor = parent;
    while (ancestor.IsValid())
    {
        if (ancestor == collection)
        {
            return false;
        }
        IndexT const index = Editor::FindCollection(ancestor);
        if (index == InvalidIndex)
        {
            return false;
        }
        ancestor = Editor::state.collections[index].parent;
    }

    CMDSetCollectionParent* cmd = new CMDSetCollectionParent;
    cmd->collection = collection;
    cmd->newParent = parent;
    return CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
MoveEntitiesToCollection(const Util::Array<Editor::Entity>& entities, const Util::Guid& collection)
{
    if (collection.IsValid() && Editor::FindCollection(collection) == InvalidIndex)
    {
        return;
    }

    uint firstOrder = 0;
    for (Editor::Editable const& editable : Editor::state.editables)
    {
        if (editable.gameEntity != Game::Entity::Invalid() && editable.collection == collection)
        {
            firstOrder = Math::max(firstOrder, editable.collectionOrder + 1);
        }
    }

    CMDMoveEntitiesToCollection* cmd = new CMDMoveEntitiesToCollection;
    for (Editor::Entity entity : entities)
    {
        if (Editor::state.editorWorld->IsValid(entity) && Editor::state.editorWorld->HasInstance(entity))
        {
            cmd->entities.Append(entity);
        }
    }
    if (cmd->entities.IsEmpty())
    {
        delete cmd;
        return;
    }
    cmd->newCollection = collection;
    cmd->firstOrder = firstOrder;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
bool
DeleteCollection(const Util::Guid& collection)
{
    if (Editor::FindCollection(collection) == InvalidIndex)
    {
        return false;
    }
    CMDDeleteCollection* cmd = new CMDDeleteCollection;
    cmd->collectionGuid = collection;
    return CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
bool
SetParent(Editor::Entity child, Editor::Entity parent)
{
    Game::World* editorWorld = Editor::state.editorWorld;
    if (child == parent || !editorWorld->IsValid(child) || !editorWorld->HasInstance(child) ||
        !editorWorld->IsValid(parent) || !editorWorld->HasInstance(parent))
    {
        return false;
    }

    Editor::Entity ancestor = parent;
    SizeT depth = 0;
    while (ancestor != Game::Entity::Invalid() && depth++ <= Editor::state.editables.Size())
    {
        if (ancestor == child)
        {
            return false;
        }
        if (!editorWorld->IsValid(ancestor) || !editorWorld->HasInstance(ancestor) ||
            !editorWorld->HasComponent<Game::HTransform>(ancestor))
        {
            break;
        }
        ancestor = editorWorld->GetComponent<Game::HTransform>(ancestor).parent;
    }
    if (depth > Editor::state.editables.Size())
    {
        return false;
    }

    Game::Scale const parentScale = editorWorld->GetComponent<Game::Scale>(parent);
    if (Math::abs(parentScale.x) < 0.000001f || Math::abs(parentScale.y) < 0.000001f || Math::abs(parentScale.z) < 0.000001f)
    {
        n_warning("Cannot parent an entity under a transform with zero scale\n");
        return false;
    }

    Game::Position const childPosition = editorWorld->GetComponent<Game::Position>(child);
    Game::Orientation const childOrientation = editorWorld->GetComponent<Game::Orientation>(child);
    Game::Scale const childScale = editorWorld->GetComponent<Game::Scale>(child);
    Game::Position const parentPosition = editorWorld->GetComponent<Game::Position>(parent);
    Game::Orientation const parentOrientation = editorWorld->GetComponent<Game::Orientation>(parent);
    Math::mat4 const parentTransform = Math::trs(parentPosition, parentOrientation, parentScale);

    CMDSetParent* cmd = new CMDSetParent;
    cmd->child = child;
    cmd->oldHasTransform = editorWorld->HasComponent<Game::HTransform>(child);
    if (cmd->oldHasTransform)
    {
        cmd->oldTransform = editorWorld->GetComponent<Game::HTransform>(child);
    }
    cmd->newHasTransform = true;
    cmd->newTransform.parent = parent;
    cmd->newTransform.localPosition = Math::vec3(Math::inverse(parentTransform) * Math::point(childPosition));
    cmd->newTransform.localOrientation = childOrientation * Math::inverse(parentOrientation);
    cmd->newTransform.localScale = Math::vec3(
        childScale.x / parentScale.x,
        childScale.y / parentScale.y,
        childScale.z / parentScale.z
    );
    return CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
bool
ClearParent(Editor::Entity child)
{
    Game::World* editorWorld = Editor::state.editorWorld;
    if (!editorWorld->IsValid(child) || !editorWorld->HasInstance(child) ||
        !editorWorld->HasComponent<Game::HTransform>(child))
    {
        return false;
    }

    CMDSetParent* cmd = new CMDSetParent;
    cmd->child = child;
    cmd->oldHasTransform = true;
    cmd->oldTransform = editorWorld->GetComponent<Game::HTransform>(child);
    cmd->newHasTransform = false;
    return CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
bool
SetWorldPosition(Editor::Entity entity, const Game::Position& position)
{
    Game::World* editorWorld = Editor::state.editorWorld;
    if (!editorWorld->HasComponent<Game::HTransform>(entity))
    {
        Game::Position value = position;
        SetComponent(entity, Game::GetComponentId<Game::Position>(), &value);
        return true;
    }

    Game::HTransform transform = editorWorld->GetComponent<Game::HTransform>(entity);
    if (transform.parent == Game::Entity::Invalid())
    {
        transform.localPosition = position;
        SetComponent(entity, Game::GetComponentId<Game::HTransform>(), &transform);
        return true;
    }
    Game::Scale const parentScale = editorWorld->GetComponent<Game::Scale>(transform.parent);
    if (Math::abs(parentScale.x) < 0.000001f || Math::abs(parentScale.y) < 0.000001f || Math::abs(parentScale.z) < 0.000001f)
    {
        return false;
    }
    Math::mat4 const parentTransform = Math::trs(
        editorWorld->GetComponent<Game::Position>(transform.parent),
        editorWorld->GetComponent<Game::Orientation>(transform.parent),
        parentScale
    );
    transform.localPosition = Math::vec3(Math::inverse(parentTransform) * Math::point(position));
    SetComponent(entity, Game::GetComponentId<Game::HTransform>(), &transform);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
SetWorldOrientation(Editor::Entity entity, const Game::Orientation& orientation)
{
    Game::World* editorWorld = Editor::state.editorWorld;
    if (!editorWorld->HasComponent<Game::HTransform>(entity))
    {
        Game::Orientation value = orientation;
        SetComponent(entity, Game::GetComponentId<Game::Orientation>(), &value);
        return true;
    }

    Game::HTransform transform = editorWorld->GetComponent<Game::HTransform>(entity);
    if (transform.parent == Game::Entity::Invalid())
    {
        transform.localOrientation = orientation;
        SetComponent(entity, Game::GetComponentId<Game::HTransform>(), &transform);
        return true;
    }
    Game::Orientation const parentOrientation = editorWorld->GetComponent<Game::Orientation>(transform.parent);
    transform.localOrientation = orientation * Math::inverse(parentOrientation);
    SetComponent(entity, Game::GetComponentId<Game::HTransform>(), &transform);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
SetWorldScale(Editor::Entity entity, const Game::Scale& scale)
{
    Game::World* editorWorld = Editor::state.editorWorld;
    if (!editorWorld->HasComponent<Game::HTransform>(entity))
    {
        Game::Scale value = scale;
        SetComponent(entity, Game::GetComponentId<Game::Scale>(), &value);
        return true;
    }

    Game::HTransform transform = editorWorld->GetComponent<Game::HTransform>(entity);
    if (transform.parent == Game::Entity::Invalid())
    {
        transform.localScale = scale;
        SetComponent(entity, Game::GetComponentId<Game::HTransform>(), &transform);
        return true;
    }
    Game::Scale const parentScale = editorWorld->GetComponent<Game::Scale>(transform.parent);
    if (Math::abs(parentScale.x) < 0.000001f || Math::abs(parentScale.y) < 0.000001f || Math::abs(parentScale.z) < 0.000001f)
    {
        return false;
    }
    transform.localScale = Math::vec3(scale.x / parentScale.x, scale.y / parentScale.y, scale.z / parentScale.z);
    SetComponent(entity, Game::GetComponentId<Game::HTransform>(), &transform);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
PreviewWorldTransform(Editor::Entity entity, const Game::Position& position, const Game::Orientation& orientation, const Game::Scale& scale)
{
    Game::World* editorWorld = Editor::state.editorWorld;
    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    Game::Entity const gameEntity = Editor::state.editables[entity.index].gameEntity;
    if (!editorWorld->HasComponent<Game::HTransform>(entity))
    {
        gameWorld->SetComponent<Game::Position>(gameEntity, position);
        gameWorld->SetComponent<Game::Orientation>(gameEntity, orientation);
        gameWorld->SetComponent<Game::Scale>(gameEntity, scale);
        gameWorld->MarkAsModified(gameEntity);
        return;
    }

    Game::HTransform transform = editorWorld->GetComponent<Game::HTransform>(entity);
    if (transform.parent == Game::Entity::Invalid())
    {
        transform.localPosition = position;
        transform.localOrientation = orientation;
        transform.localScale = scale;
        gameWorld->SetComponent<Game::HTransform>(gameEntity, transform);
        Game::HierarchyManager::Resolve(gameWorld);
        return;
    }
    Game::Scale const parentScale = editorWorld->GetComponent<Game::Scale>(transform.parent);
    if (Math::abs(parentScale.x) < 0.000001f || Math::abs(parentScale.y) < 0.000001f || Math::abs(parentScale.z) < 0.000001f)
    {
        return;
    }
    Game::Orientation const parentOrientation = editorWorld->GetComponent<Game::Orientation>(transform.parent);
    Math::mat4 const parentTransform = Math::trs(
        editorWorld->GetComponent<Game::Position>(transform.parent),
        parentOrientation,
        parentScale
    );
    transform.parent = Editor::ToGameEntity(transform.parent);
    transform.localPosition = Math::vec3(Math::inverse(parentTransform) * Math::point(position));
    transform.localOrientation = orientation * Math::inverse(parentOrientation);
    transform.localScale = Math::vec3(scale.x / parentScale.x, scale.y / parentScale.y, scale.z / parentScale.z);
    gameWorld->SetComponent<Game::HTransform>(gameEntity, transform);
    Game::HierarchyManager::Resolve(gameWorld);
}

} // namespace Edit
