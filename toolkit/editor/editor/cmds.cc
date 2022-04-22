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

namespace Edit
{

//------------------------------------------------------------------------------
/**
*/
bool
InternalCreateEntity(Editor::Entity id, Util::StringAtom templateName)
{
    n_assert(Game::IsValid(Editor::state.editorWorld, id));

    Game::TemplateId const tid = Game::GetTemplateId(templateName);

    if (Game::IsActive(Editor::state.editorWorld, id))
    {
        n_warning("Entity already has an instance!\n");
        return false;
    }

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    Game::EntityCreateInfo createInfo;
    createInfo.immediate = true;
    createInfo.templateId = tid;
    Game::Entity const entity = Game::CreateEntity(gameWorld, createInfo);
    
    if (Editor::state.editables.Size() >= id.index)
        Editor::state.editables.Append({});
    Game::AllocateInstance(Editor::state.editorWorld, id, tid);

    Editor::Editable& edit = Editor::state.editables[id.index];
    edit.gameEntity = entity;
    edit.name = templateName.AsString().ExtractFileName();
    edit.templateId = tid;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
InternalCreateEntity(Editor::Entity editorEntity, MemDb::TableId editorTable, Util::Blob entityState)
{
    n_assert(Game::IsValid(Editor::state.editorWorld, editorEntity));

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);

    MemDb::TableSignature const& signature = Game::GetWorldDatabase(Editor::state.editorWorld)->GetTableSignature(editorTable);

    Game::Entity const entity = Game::AllocateEntity(gameWorld);
    MemDb::TableId gameTable = Game::GetWorldDatabase(gameWorld)->FindTable(signature);
    n_assert(gameTable != MemDb::InvalidTableId);
    MemDb::Row instance = Game::AllocateInstance(gameWorld, entity, gameTable);
    Game::GetWorldDatabase(gameWorld)->DeserializeInstance(entityState, gameTable, instance);
    Game::SetComponent<Game::Entity>(gameWorld, entity, Game::GetComponentId("Owner"_atm), entity);

    if (Game::IsActive(Editor::state.editorWorld, editorEntity))
    {
        n_warning("Entity already has an instance!\n");
        return false;
    }
    
    if (Editor::state.editables.Size() >= editorEntity.index)
        Editor::state.editables.Append({});
    MemDb::Row editorInstance = Game::AllocateInstance(Editor::state.editorWorld, editorEntity, editorTable);
    Game::GetWorldDatabase(Editor::state.editorWorld)->DeserializeInstance(entityState, editorTable, editorInstance);
    Game::SetComponent<Editor::Entity>(Editor::state.editorWorld, editorEntity, Game::GetComponentId("Owner"_atm), editorEntity);

    Editor::Editable& edit = Editor::state.editables[editorEntity.index];
    edit.gameEntity = entity;
    

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
InternalDestroyEntity(Editor::Entity editorEntity)
{
    n_assert(Game::IsValid(Editor::state.editorWorld, editorEntity));
    n_assert(Game::IsActive(Editor::state.editorWorld, editorEntity));

    Game::EntityMapping const mapping = Game::GetEntityMapping(Editor::state.editorWorld, editorEntity);
    Editor::Editable& edit = Editor::state.editables[editorEntity.index];

    if (Game::IsValid(Game::GetWorld(WORLD_DEFAULT), edit.gameEntity))
        Game::DeleteEntity(Game::GetWorld(WORLD_DEFAULT), edit.gameEntity);

    edit.gameEntity = Game::Entity::Invalid();

    Game::DeallocateInstance(Editor::state.editorWorld, editorEntity);
    
    // Make sure the editor world is always defragged
    Game::Defragment(Editor::state.editorWorld, mapping.table);
}

//------------------------------------------------------------------------------
/**
*/
bool
InternalSetProperty(Editor::Entity editorEntity, Game::ComponentId component, void* value, size_t size)
{
    n_assert(Game::IsValid(Editor::state.editorWorld, editorEntity));
    n_assert(Game::IsActive(Editor::state.editorWorld, editorEntity));

    Game::EntityMapping mapping = Game::GetEntityMapping(Editor::state.editorWorld, editorEntity);
    Editor::Editable& edit = Editor::state.editables[editorEntity.index];

    Game::SetComponent(Editor::state.editorWorld, editorEntity, component, value, size);
	Game::World* defaultWorld = Game::GetWorld(WORLD_DEFAULT);
	if (Game::IsValid(defaultWorld, edit.gameEntity))
	{
        Game::SetComponent(defaultWorld, edit.gameEntity, component, value, size);
		if (component == Game::GetComponentId("ModelResource"))
		{
			auto modelEntityDataPid = Game::GetComponentId("ModelEntityData");
			if (Game::HasComponent(defaultWorld, edit.gameEntity, modelEntityDataPid))
			{
				Game::Op::DeregisterComponent deregOp;
				deregOp.entity = edit.gameEntity;
				deregOp.component = modelEntityDataPid;
				Game::Execute(defaultWorld, deregOp);
			}
		}
	}

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
InternalAddProperty(Editor::Entity editorEntity, Game::ComponentId component, void* value)
{
    n_assert(Game::IsValid(Editor::state.editorWorld, editorEntity));
    
    Editor::Editable& edit = Editor::state.editables[editorEntity.index];

    if (Game::HasComponent(Editor::state.editorWorld, editorEntity, component))
        return false;

    Game::Op::RegisterComponent regOp;
    regOp.component = component;
    regOp.value = value;
    
    regOp.entity = editorEntity;
    Game::Execute(Editor::state.editorWorld, regOp);
    
    regOp.entity = edit.gameEntity;
    Game::Execute(Game::GetWorld(WORLD_DEFAULT), regOp);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
InternalRemoveProperty(Editor::Entity editorEntity, Game::ComponentId component)
{
    n_assert(Game::IsValid(Editor::state.editorWorld, editorEntity));
    
    Editor::Editable& edit = Editor::state.editables[editorEntity.index];

    if (!Game::HasComponent(Editor::state.editorWorld, editorEntity, component))
        return false;

    Game::Op::DeregisterComponent deregOp;
    deregOp.component = component;
    
    deregOp.entity = editorEntity;
    Game::Execute(Editor::state.editorWorld, deregOp);
    
    deregOp.entity = edit.gameEntity;
    Game::Execute(Game::GetWorld(WORLD_DEFAULT), deregOp);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
struct CMDCreateEntity : public Edit::Command
{
    ~CMDCreateEntity() { if (!executed) Game::DeallocateEntity(Editor::state.editorWorld, this->id); };
    const char* Name() override { return "Create Entity"; };
    bool Execute() override
    {
        if (this->id == Editor::Entity::Invalid())
            this->id = Game::AllocateEntity(Editor::state.editorWorld);
        if (Editor::state.editables.Size() >= this->id.index)
            Editor::state.editables.Append({});
        if (!initialized)
        {
            Editor::state.editables[this->id.index].guid.Generate();
            initialized = true;
        }

        return InternalCreateEntity(id, templateName);
    };
    bool Unexecute() override
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
    ~CMDDeleteEntity() { if (executed) Game::DeallocateEntity(Editor::state.editorWorld, this->id); };
    const char* Name() override { return "Delete Entity"; };
    bool Execute() override
    {
        if (this->id == Editor::Entity::Invalid())
            return false;
        if (!this->initialized)
        {
            Game::EntityMapping mapping = Game::GetEntityMapping(Editor::state.editorWorld, this->id);
            this->entityState = Game::GetWorldDatabase(Editor::state.editorWorld)->SerializeInstance(mapping.table, mapping.instance);
            this->tid = mapping.table;
            this->initialized = true;
        }
        InternalDestroyEntity(this->id);
        return true;
    };
    bool Unexecute() override
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
    const char* Name() override { return "Set selection"; };
    bool Execute() override
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
    bool Unexecute() override
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
    const char* Name() override { return "Set property"; };
    bool Execute() override
    {
        if (!newValue.IsValid()) return false;
        if (!oldValue.IsValid())
        {
            Game::EntityMapping const mapping = Game::GetEntityMapping(Editor::state.editorWorld, id);
            MemDb::TableId const tid = mapping.table;
            Ptr<MemDb::Database> editorWorldDB = Game::GetWorldDatabase(Editor::state.editorWorld);
            void* oldValuePtr = editorWorldDB->GetValuePointer(tid, editorWorldDB->GetColumnId(tid, component), mapping.instance);
            oldValue.Set(oldValuePtr, newValue.Size());
        }
        return InternalSetProperty(id, component, newValue.GetPtr(), newValue.Size());
    };
    bool Unexecute() override
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
    const char* Name() override { return "Add property"; };
    bool Execute() override
    {
        return InternalAddProperty(id, component, nullptr);
    };
    bool Unexecute() override
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
    const char* Name() override { return "Remove property"; };
    bool Execute() override
    {
        if (!value.IsValid() && MemDb::TypeRegistry::TypeSize(component) != 0)
        {
            Game::EntityMapping const mapping = Game::GetEntityMapping(Editor::state.editorWorld, id);
            MemDb::TableId const tid = mapping.table;
            Ptr<MemDb::Database> editorWorldDB = Game::GetWorldDatabase(Editor::state.editorWorld);
            void* valuePtr = editorWorldDB->GetValuePointer(tid, editorWorldDB->GetColumnId(tid, component), mapping.instance);
            value.Set(valuePtr, MemDb::TypeRegistry::TypeSize(component));
        }
        return InternalRemoveProperty(id, component);
    };
    bool Unexecute() override
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
    const char* Name() override { return "Set entity name"; };
    bool Execute() override
    {
        if (oldName.IsEmpty())
            oldName = Editor::state.editables[id.index].name;

        Editor::state.editables[id.index].name = newName;
        return true;
    };
    bool Unexecute() override
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
    CMDCreateEntity* cmd = n_new(CMDCreateEntity);
    Editor::Entity const entity = Game::AllocateEntity(Editor::state.editorWorld);
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
    CMDDeleteEntity* cmd = n_new(CMDDeleteEntity);
    cmd->id = entity;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
SetSelection(Util::Array<Editor::Entity> const& entities)
{
	CMDSetSelection* cmd = n_new(CMDSetSelection);
    cmd->newSelection = entities;
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
SetComponent(Editor::Entity entity, Game::ComponentId component, void* value)
{
    CMDSetProperty* cmd = n_new(CMDSetProperty);
    cmd->id = entity;
    cmd->component = component;
    cmd->newValue.Set(value, MemDb::TypeRegistry::TypeSize(component));
    CommandManager::Execute(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
AddComponent(Editor::Entity entity, Game::ComponentId component)
{
    CMDAddProperty* cmd = n_new(CMDAddProperty);
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
    CMDRemoveProperty* cmd = n_new(CMDRemoveProperty);
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
    CMDSetEntityName* cmd = n_new(CMDSetEntityName);
    cmd->id = entity;
    cmd->newName = name;
    CommandManager::Execute(cmd);
}

} // namespace Edit
