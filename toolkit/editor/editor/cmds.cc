//------------------------------------------------------------------------------
//  cmds.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "cmds.h"
#include "commandmanager.h"
#include "util/random.h"
#include "editor/tools/selectiontool.h"
#include "graphicsfeature/managers/graphicsmanager.h"
#include "basegamefeature/components/basegamefeature.h"
#include "graphicsfeature/components/graphicsfeature.h"

namespace Edit
{

//------------------------------------------------------------------------------
/**
*/
bool
InternalCreateEntity(Editor::Entity id, Util::StringAtom templateName)
{
    n_assert(Editor::state.editorWorld->IsValid(id));

    Game::TemplateId const tid = Game::GetTemplateId(templateName);

    if (Editor::state.editorWorld->HasInstance(id))
    {
        n_warning("Entity already has an instance!\n");
        return false;
    }

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    Game::EntityCreateInfo createInfo;
    createInfo.immediate = true;
    createInfo.templateId = tid;
    Game::Entity const entity = gameWorld->CreateEntity(createInfo);

    if (Editor::state.editables.Size() >= id.index)
        Editor::state.editables.Append({});
    Editor::state.editorWorld->AllocateInstance(id, tid);

    Editor::Editable& edit = Editor::state.editables[id.index];
    edit.gameEntity = entity;
    edit.name = templateName.AsString().ExtractFileName();
    edit.templateId = tid;

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

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);

    MemDb::TableSignature const& signature =
        Editor::state.editorWorld->GetDatabase()->GetTable(editorTable).GetSignature();

    Game::Entity const entity = gameWorld->AllocateEntity();
    MemDb::TableId gameTable = gameWorld->GetDatabase()->FindTable(signature);
    n_assert(gameTable != MemDb::InvalidTableId);
    MemDb::RowId instance = gameWorld->AllocateInstance(entity, gameTable);
    gameWorld->GetDatabase()->GetTable(gameTable).DeserializeInstance(entityState, instance);
    gameWorld->SetComponent<Game::Entity>(entity, entity);

    if (Editor::state.editorWorld->HasInstance(editorEntity))
    {
        n_warning("Entity already has an instance!\n");
        return false;
    }

    if (Editor::state.editables.Size() >= editorEntity.index)
        Editor::state.editables.Append({});
    MemDb::RowId editorInstance = Editor::state.editorWorld->AllocateInstance(editorEntity, editorTable);
    Editor::state.editorWorld->GetDatabase()->GetTable(editorTable).DeserializeInstance(entityState, editorInstance);
    Editor::state.editorWorld->SetComponent<Game::Entity>(editorEntity, editorEntity);

    Editor::Editable& edit = Editor::state.editables[editorEntity.index];
    edit.gameEntity = entity;

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
        // Remove default world entity instance and create a new one from the data in editor world. Then 
        defaultWorld->DeallocateInstance(edit.gameEntity);

        MemDb::Table const& editorTable = Editor::state.editorWorld->GetDatabase()->GetTable(mapping.table);
        MemDb::TableSignature signature = editorTable.GetSignature();
        
        MemDb::TableId gameTableId = defaultWorld->GetDatabase()->FindTable(signature);
        if (gameTableId == MemDb::TableId::Invalid())
        {
            // TODO: Maybe generalize this into a function in database?
            auto const& attributes = editorTable.GetAttributes();
            MemDb::TableCreateInfo tableInfo;
            tableInfo.attributeIds = &attributes[0];
            tableInfo.numAttributes = attributes.Size();
            tableInfo.name = editorTable.name.Value();
            gameTableId = defaultWorld->GetDatabase()->CreateTable(tableInfo);
        }
        MemDb::Table& gameTable = defaultWorld->GetDatabase()->GetTable(gameTableId);

        Util::Blob const blob = editorTable.SerializeInstance(mapping.instance);

        MemDb::RowId gameRow = defaultWorld->AllocateInstance(edit.gameEntity, gameTableId, &blob);
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

    Editor::state.editorWorld->AddComponent(editorEntity, component);

    Game::GetWorld(WORLD_DEFAULT)->AddComponent(edit.gameEntity, component);
    
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
            Editor::state.editorWorld->DeallocateEntity(this->id);
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
            this->id = Editor::state.editorWorld->AllocateEntity();
        if (Editor::state.editables.Size() >= this->id.index)
            Editor::state.editables.Append({});
        if (!initialized)
        {
            Editor::state.editables[this->id.index].guid.Generate();
            initialized = true;
        }

        return InternalCreateEntity(id, templateName);
    };
    bool
    Unexecute() override
    {
        InternalDestroyEntity(id);
        return true;
    };
    Editor::Entity id;
    Util::StringAtom templateName;

private:
    bool initialized = false;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDDeleteEntity : public Edit::Command
{
    ~CMDDeleteEntity()
    {
        if (executed)
            Editor::state.editorWorld->DeallocateEntity(this->id);
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
            this->initialized = true;
        }
        InternalDestroyEntity(this->id);
        return true;
    };
    bool
    Unexecute() override
    {
        InternalCreateEntity(this->id, this->tid, this->entityState);
        return true;
    };
    Editor::Entity id;

private:
    bool initialized = false;
    Util::Blob entityState;
    MemDb::TableId tid;
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
            this->oldSelection = Tools::SelectionTool::Selection();
            this->initialized = true;
        }
        Tools::SelectionTool::selection = newSelection;
        Tools::SelectionTool::selection.Sort();
        return true;
    };
    bool
    Unexecute() override
    {
        Tools::SelectionTool::selection = oldSelection;
        Tools::SelectionTool::selection.Sort();
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
Editor::Entity
CreateEntity(Util::StringAtom templateName)
{
    CMDCreateEntity* cmd = new CMDCreateEntity;
    Editor::Entity const entity = Editor::state.editorWorld->AllocateEntity();
    cmd->id = entity;
    cmd->templateName = templateName;
    CommandManager::Execute(cmd);
    return entity;
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

} // namespace Edit
