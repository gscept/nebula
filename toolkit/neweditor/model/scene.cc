//------------------------------------------------------------------------------
//  @file scene.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/model/scene.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/scale.h"
#include "basegamefeature/managers/transformhierarchy.h"
#include "neweditor/api/shell.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/base/editorlog.h"
#include "game/component.h"
#include "game/dataset.h"
#include "game/filter.h"
#include "game/gameserver.h"
#include "game/levelextension.h"
#include "memdb/attributeregistry.h"
#include "memdb/database.h"
#include "memdb/table.h"
#include "memdb/tableid.h"

namespace Editor
{

__ImplementClass(Editor::Scene, 'SCNE', Core::RefCounted);

static const Math::vec3 CameraStart(5, 5, 5);

// The angles that make a camera at CameraStart look at the origin.
static const float CameraStartPitch = Math::deg2rad(-35.264f);
static const float CameraStartYaw = Math::deg2rad(45.0f);

// The most chain files a prefab entity can walk through. The walk stops at a file it has already
// read, so this is the backstop for a hand-written file that keeps naming new ones.
static const IndexT MaxPrefabChainDepth = 32;

//------------------------------------------------------------------------------
/**
    Whether two spans of component bytes hold different values.
*/
static bool
BytesDiffer(const byte* a, const byte* b, SizeT size)
{
    for (SizeT i = 0; i < size; i++)
    {
        if (a[i] != b[i])
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
Scene::Scene() :
    core(nullptr),
    authoringHash(Game::WorldHash::Invalid()),
    authoringWorld(nullptr),
    simWorld(nullptr),
    savedVersion(0),
    version(0),
    playing(false),
    inSimWorld(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Scene::~Scene()
{
    this->Discard();
}

//------------------------------------------------------------------------------
/**
    The hash is the guid, so a scene keeps the same hash wherever it is in the scene list.
*/
void
Scene::Setup(Shell* core, const Util::String& sceneName)
{
    this->core = core;
    this->name = sceneName;
    this->guid.Generate();

    this->authoringHash = Game::WorldHash{this->guid.HashCode()};

    this->authoringWorld = Game::GameServer::Instance()->CreateWorld(this->authoringHash);
    this->authoringWorld->componentInitializationEnabled = false;
    this->authoringWorld->mode = Game::EditorMode;
    Game::GameServer::Instance()->SetupEmptyWorld(this->authoringWorld);

    // The sim world is shared by every open scene, so only the active scene is in it.
    this->simWorld = Game::GetWorld(WORLD_DEFAULT);
    n_assert(this->simWorld != nullptr);
    this->simWorld->mode = Game::EditorMode;

    // The history and the selection are the scene's, so a tab that is not showing keeps them
    this->commandManager = EditCommandManager::Create();
    this->commandManager->Setup(core);

    this->selection = SelectionContext::Create();
    this->selection->Setup(core, this);

    // The view is the scene's own, so a tab you come back to shows the view you left it with. The
    // camera object the renderer draws with is shared by every scene.
    this->ResetCamera();
}

//------------------------------------------------------------------------------
/**
    The undo stacks go before the world, so a command that still holds an id can release it against
    a world that is still there.
*/
void
Scene::Discard()
{
    if (this->authoringWorld == nullptr)
        return;

    // The sim world belongs to the scene that is showing, so only the flag changes here.
    this->playing = false;

    if (this->commandManager != nullptr)
        this->commandManager->Discard();
    if (this->selection != nullptr)
        this->selection->Discard();

    if (Game::GameServer::HasInstance())
        Game::GameServer::Instance()->DestroyWorld(this->authoringHash);

    this->authoringWorld = nullptr;
    this->editables.Clear();
    this->collections.Clear();
    this->activeCollection = Util::Guid();
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
    The sim world is shared, so showing a scene means copying its data over the scene that was
    showing. Sim state does not survive a tab switch, the authored data does. The camera is shared
    the same way, so the view is written from this scene's camera.
*/
void
Scene::Activate()
{
    // The mode is set before the copy, because a property is activated when its world starts
    // simulating.
    this->simWorld->mode = this->playing ? Game::PlayMode : Game::EditorMode;

    this->RebuildSimWorld();
    this->inSimWorld = true;

    EditorCameraInput input;
    this->camera.Update(input, 0.0f);
}

//------------------------------------------------------------------------------
/**
    The sim world now holds the scene that is showing, so there is nothing to copy back here. Only
    the flag changes.
*/
void
Scene::Deactivate()
{
    this->playing = false;
    this->inSimWorld = false;

    // The next scene is copied over the sim world, so these ids belong to another tab now. Nothing
    // may
    // still point at them.
    for (IndexT i = 0; i < this->editables.Size(); i++)
        this->editables[i].gameEntity = Game::Entity::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
EditorCamera&
Scene::GetCamera()
{
    return this->camera;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::ResetCamera()
{
    this->camera.position = CameraStart;

    this->camera.rotx = CameraStartPitch;
    this->camera.roty = CameraStartYaw;
    this->camera.forward = Math::normalize(CameraStart);

    // The pivot is the point the camera looks at, so an orbit the length of CameraStart puts it on
    // the
    // origin.
    this->camera.SetPivot(Math::vec3(0, 0, 0), Math::length(CameraStart));
}

//------------------------------------------------------------------------------
/**
*/
const Util::Guid&
Scene::GetGuid() const
{
    return this->guid;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SelectionContext>
Scene::GetSelection() const
{
    return this->selection;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<EditCommandManager>
Scene::GetCommandManager() const
{
    return this->commandManager;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
Scene::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
Game::World*
Scene::GetAuthoringWorld() const
{
    return this->authoringWorld;
}

//------------------------------------------------------------------------------
/**
*/
Game::World*
Scene::GetSimWorld() const
{
    return this->simWorld;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
Scene::CreateEntity(const Util::String& entityName)
{
    Editable record;
    record.guid.Generate();
    record.name = entityName;
    record.collection = this->activeCollection;
    record.entity = this->AllocateEntityId();
    this->AllocateInstance(record);
    this->ResolveInstanceMembership(record.entity);
    return record.entity;
}

//------------------------------------------------------------------------------
/**
    The full delete. A command that holds an entity id only takes the instance out, so the id stays
    where the other commands have it.
*/
void
Scene::DeleteEntity(Game::Entity entity)
{
    this->DeallocateInstance(entity);

    if (entity.index < this->editables.Size())
    {
        Editable& editable = this->editables[entity.index];
        editable.guid = Util::Guid();
        editable.name.Clear();
        editable.entity = Game::Entity::Invalid();
        editable.gameEntity = Game::Entity::Invalid();
        editable.collection = Util::Guid();
        editable.prefabInstance = InvalidIndex;
        editable.prefabEntity = Util::Guid();
        editable.prefabOverrides.Clear();
    }

    this->DeallocateEntityId(entity);
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
Scene::AllocateEntityId()
{
    Game::Entity const entity = this->authoringWorld->AllocateEntityId();

    if (entity.index >= this->editables.Size())
        this->editables.Resize(entity.index + 1);

    return entity;
}

//------------------------------------------------------------------------------
/**
    Releasing an id clears the editable record. The command that held it has its own copy of the
    identity.
*/
void
Scene::DeallocateEntityId(Game::Entity entity)
{
    if (entity == Game::Entity::Invalid() || this->authoringWorld == nullptr)
        return;

    this->authoringWorld->DeallocateEntityId(entity);

    if (entity.index < this->editables.Size())
    {
        Editable& editable = this->editables[entity.index];
        editable.guid = Util::Guid();
        editable.name.Clear();
        editable.entity = Game::Entity::Invalid();
        editable.gameEntity = Game::Entity::Invalid();
        editable.collection = Util::Guid();
        editable.prefabInstance = InvalidIndex;
        editable.prefabEntity = Util::Guid();
        editable.prefabOverrides.Clear();
    }

    this->version++;
}

//------------------------------------------------------------------------------
/**
    The instance goes back at the index the record says, which is the index the entity had before.
    The components are not part of the record, so the command adds them after this.
*/
void
Scene::AllocateInstance(const Editable& record)
{
    n_assert(record.entity != Game::Entity::Invalid());

    if (record.entity.index >= this->editables.Size())
        this->editables.Resize(record.entity.index + 1);

    Editable& editable = this->editables[record.entity.index];
    editable.guid = record.guid;
    editable.name = record.name;
    editable.entity = record.entity;
    editable.gameEntity = Game::Entity::Invalid();
    editable.collection = record.collection;
    editable.prefabInstance = record.prefabInstance;
    editable.prefabEntity = record.prefabEntity;
    editable.prefabOverrides = record.prefabOverrides;

    // An addition is in the instance's set, so a redo puts it back. An entity with a record was put
    // in
    // by the path that read the file.
    if (!record.prefabEntity.IsValid() && record.prefabInstance != InvalidIndex &&
        record.prefabInstance < (IndexT)this->prefabInstances.Size() &&
        this->prefabInstances[record.prefabInstance].valid)
        this->AppendInstanceEntity(record.prefabInstance, record.entity);

    this->authoringWorld->AllocateInstance(record.entity);

    this->version++;
    this->MirrorEntity(record.entity);
}

//------------------------------------------------------------------------------
/**
    Only the instance goes. The id and the record stay for the command that took it out.
*/
void
Scene::DeallocateInstance(Game::Entity entity)
{
    if (!this->authoringWorld->HasInstance(entity))
        return;

    Game::Entity const gameEntity = this->ToGameEntity(entity);
    if (gameEntity != Game::Entity::Invalid() && this->simWorld->IsValid(gameEntity))
    {
        // Deleted through the world so that the managers still get the decay buffer for the
        // components
        this->simWorld->DeleteEntity(gameEntity);
    }

    this->authoringWorld->DeallocateInstance(entity);

    if (entity.index < this->editables.Size())
    {
        // An addition is in the instance's set, so taking the entity out takes it out of the
        // instance.
        // An entity with a record stays, because the undo puts the entity back.
        const Editable& released = this->editables[entity.index];
        if (!released.prefabEntity.IsValid() && released.prefabInstance != InvalidIndex &&
            released.prefabInstance < (IndexT)this->prefabInstances.Size())
        {
            Util::Array<Game::Entity>& entities = this->prefabInstances[released.prefabInstance].entities;
            const IndexT i = entities.FindIndex(entity);
            if (i != InvalidIndex)
                entities.EraseIndex(i);
        }

        this->editables[entity.index].gameEntity = Game::Entity::Invalid();
    }

    this->version++;
}

//------------------------------------------------------------------------------
/**
    A null value is a plain add. A value is what an undo of a removal puts back. The version is
    bumped here too, because which components an entity has is part of what a panel reads.
*/
void*
Scene::AddComponent(Game::Entity entity, Game::ComponentId component, const void* value)
{
    this->authoringWorld->AddComponent(entity, component, value);

    // The world dispatches components at the frame boundary, but authoring has to be visible right
    // away, so the queue runs here.
    this->authoringWorld->ExecuteAddComponentCommands();

    this->MirrorEntity(entity);
    this->version++;

    // A component the chain does not have is one the instance added.
    if (this->IsPrefabInstanceEntity(entity))
    {
        this->MarkPrefabOverrides(entity, component,
            (const byte*)this->GetComponentData(entity, component));
    }

    return this->GetComponentData(entity, component);
}

//------------------------------------------------------------------------------
/**
    The version is bumped for the same reason as the add.
*/
void
Scene::RemoveComponent(Game::Entity entity, Game::ComponentId component)
{
    this->authoringWorld->RemoveComponent(entity, component);
    this->authoringWorld->ExecuteRemoveComponentCommands();

    this->MirrorEntity(entity);
    this->version++;
}

//------------------------------------------------------------------------------
/**
    The three components the transform is made of. A write to one of them goes through the hierarchy
    for an entity with a parent, so SetComponent and PreviewComponent have to know it.
*/
bool
Scene::IsTransformComponent(Game::ComponentId component)
{
    return component == Game::GetComponentId<Game::Position>() ||
        component == Game::GetComponentId<Game::Orientation>() ||
        component == Game::GetComponentId<Game::Scale>();
}

//------------------------------------------------------------------------------
/**
    A component value is only applied by a feature when someone tells it, and the editor is the one
    who made the change. The scene does not know which features need it, so it tells the ones that
    registered. A feature that reads a transform chain has to see the subtree as well, and that walk
    is the feature's own.
*/
void
Scene::NotifyComponentWritten(Game::Entity gameEntity, Game::ComponentId component)
{
    const Util::Array<EditorOnComponentChangeListener>& listeners = EditorFeatureRegistry::GetListeners();
    for (IndexT i = 0; i < listeners.Size(); i++)
    {
        if (listeners[i].onChange.IsValid())
            listeners[i].onChange(this, gameEntity, component);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::SetComponent(Game::Entity entity, Game::ComponentId component, const void* value)
{
    // The three transform components are the world values, what the panels show and the renderer
    // draws
    // with. For an entity with a parent they are composed from its local transform, so what is
    // authored is the value read through the parent. Writing the local one recomposes the world
    // values
    // for everything below it.
    const Game::ComponentId transform = Game::GetComponentId<Game::HTransform>();
    if (IsTransformComponent(component) && this->authoringWorld->HasComponent(entity, transform))
    {
        Game::Position position = this->authoringWorld->GetComponent<Game::Position>(entity);
        Game::Orientation orientation = this->authoringWorld->GetComponent<Game::Orientation>(entity);
        Game::Scale scale = this->authoringWorld->GetComponent<Game::Scale>(entity);

        if (component == Game::GetComponentId<Game::Position>())
            position = *(const Game::Position*)value;
        else if (component == Game::GetComponentId<Game::Orientation>())
            orientation = *(const Game::Orientation*)value;
        else
            scale = *(const Game::Scale*)value;

        Game::HTransform local = this->LocalTransform(this->authoringWorld, entity, this->GetParent(entity), position, orientation, scale);
        this->authoringWorld->SetComponentValue(entity, transform, &local, MemDb::AttributeRegistry::TypeSize(transform));
        this->MirrorComponent(entity, transform);
    }

    this->authoringWorld->SetComponentValue(entity, component, (void*)value, MemDb::AttributeRegistry::TypeSize(component));
    this->MarkPrefabOverrides(entity, component, (const byte*)value);
    this->MirrorComponent(entity, component);
    this->version++;
}

//------------------------------------------------------------------------------
/**
    An authored write is what makes a field overridden, so the mark is recomputed by comparing the
    written value with what the chain gives. Comparing instead of just marking is what lets an undo
    drop the mark again.
*/
void
Scene::MarkPrefabOverrides(Game::Entity entity, Game::ComponentId component, const byte* value)
{
    if (entity.index >= this->editables.Size())
        return;

    Editable& editable = this->editables[entity.index];
    if (editable.prefabInstance == InvalidIndex || !editable.guid.IsValid())
        return;

    if (!this->authoringWorld->HasComponent(entity, component))
        return;

    Game::ComponentInterface* interface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    const IndexT numFields = (IndexT)interface->GetNumFields();

    // A component the chain does not have is one the instance added, and every field of it is the
    // instance's own.
    Util::Blob base;
    const bool inChain = this->GetPrefabBaseValue(entity, component, base);

    PrefabOverride* state = nullptr;
    for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
    {
        if (editable.prefabOverrides[i].component == component)
        {
            state = &editable.prefabOverrides[i];
            break;
        }
    }

    if (!inChain)
    {
        if (nullptr == state)
        {
            PrefabOverride added;
            added.component = component;
            added.componentAdded = true;
            editable.prefabOverrides.Append(added);
            state = &editable.prefabOverrides.Back();
        }
        state->componentRemoved = false;
        for (IndexT field = 0; field < numFields; field++)
            state->overriddenFields.SetBit((uint64_t)field);
        return;
    }

    const byte* baseValue = (const byte*)base.GetPtr();
    const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);

    if (nullptr == state)
    {
        PrefabOverride override;
        override.component = component;
        editable.prefabOverrides.Append(override);
        state = &editable.prefabOverrides.Back();
    }
    state->componentAdded = false;
    state->componentRemoved = false;
    state->overriddenFields = Util::BitField<MAX_COMPONENT_FIELDS>();

    for (IndexT field = 0; field < numFields; field++)
    {
        const SizeT offset = (SizeT)interface->GetFieldByteOffsets()[field];
        const SizeT size = (field + 1 < numFields)
            ? (SizeT)interface->GetFieldByteOffsets()[field + 1] - offset
            : typeSize - offset;

        if (BytesDiffer(baseValue + offset, value + offset, size))
            state->overriddenFields.SetBit((uint64_t)field);
    }
}

//------------------------------------------------------------------------------
/**
    The value goes into the sim world only and the authoring world is left alone, so a preview is
    view state and cannot be undone. No references are remapped, because a drag writes plain values.
    It writes through World::SetComponentValue rather than the row pointer, because that path also
    reaches the entities below the one written, and the features that registered for a write are
    told.
*/
void
Scene::PreviewComponent(Game::Entity entity, Game::ComponentId component, const void* value)
{
    Game::Entity const gameEntity = this->ToGameEntity(entity);
    if (!this->inSimWorld || gameEntity == Game::Entity::Invalid() || !this->simWorld->IsValid(gameEntity) ||
        !this->simWorld->HasInstance(gameEntity))
        return;

    SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
    if (typeSize == 0)
        return;

    Game::EntityMapping const dstMapping = this->simWorld->GetEntityMapping(gameEntity);
    MemDb::Table& dstTable = this->simWorld->GetDatabase()->GetTable(dstMapping.table);
    if (!dstTable.HasAttribute(component))
        return;

    // The transform is previewed through the parent as well, so the world components come back as
    // the
    // value the drag is at. The authored transform is not touched, so an aborted drag puts the view
    // back.
    const Game::ComponentId transform = Game::GetComponentId<Game::HTransform>();
    if (IsTransformComponent(component) && dstTable.HasAttribute(transform))
    {
        Game::HTransform const current = this->simWorld->GetComponent<Game::HTransform>(gameEntity);

        Game::Position position = this->simWorld->GetComponent<Game::Position>(gameEntity);
        Game::Orientation orientation = this->simWorld->GetComponent<Game::Orientation>(gameEntity);
        Game::Scale scale = this->simWorld->GetComponent<Game::Scale>(gameEntity);

        if (component == Game::GetComponentId<Game::Position>())
            position = *(const Game::Position*)value;
        else if (component == Game::GetComponentId<Game::Orientation>())
            orientation = *(const Game::Orientation*)value;
        else
            scale = *(const Game::Scale*)value;

        Game::HTransform local = this->LocalTransform(this->simWorld, gameEntity, current.parent, position, orientation, scale);
        this->simWorld->SetComponentValue(gameEntity, transform, &local, MemDb::AttributeRegistry::TypeSize(transform));
    }

    this->simWorld->SetComponentValue(gameEntity, component, (void*)value, typeSize);
    this->NotifyComponentWritten(gameEntity, component);

    this->simWorld->MarkAsModified(gameEntity);
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::RestoreComponent(Game::Entity entity, Game::ComponentId component)
{
    this->MirrorComponent(entity, component);
}

//------------------------------------------------------------------------------
/**
    Put one entity's extension data back from the text an extension wrote for it. The scene does not
    read the text. An empty text means an entity with nothing on it, which is what makes the same
    call serve both sides of an add. It writes the authoring mirror, and the sim mirror is put back
    by the mirror that follows.
*/
bool
Scene::ApplyExtension(Game::Entity entity, const Game::LevelExtension* extension, const Util::String& text)
{
    if (!this->HasInstance(entity) || extension == nullptr || extension->apply == nullptr)
        return false;

    // An extension's text names entities by the scene's guids, so the map has to be the scene's own
    // here.
    Util::HashTable<Util::Guid, Game::Entity> map;
    this->BuildSceneMap(map);
    this->SetRefContext(map);

    extension->apply(entity, text.AsCharPtr());

    this->version++;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::SetEntityName(Game::Entity entity, const Util::String& name)
{
    if (entity.index >= this->editables.Size() || !this->editables[entity.index].guid.IsValid())
        return false;

    this->editables[entity.index].name = name;
    this->version++;
    return true;
}

//------------------------------------------------------------------------------
/**
    A name no other entity has. The base is numbered until it does not collide, because a name is
    how an entity is looked up.
*/
Util::String
Scene::UniqueEntityName(const Util::String& base) const
{
    Util::String name = base;
    int number = 1;

    bool collides = true;
    while (collides)
    {
        collides = false;
        for (IndexT i = 0; i < this->editables.Size(); i++)
        {
            if (this->editables[i].guid.IsValid() && this->editables[i].name == name)
            {
                collides = true;
                break;
            }
        }

        if (collides)
        {
            name.Format("%s %d", base.AsCharPtr(), number);
            number++;
        }
    }

    return name;
}

bool
Scene::HasInstance(Game::Entity entity) const
{
    return this->authoringWorld != nullptr && this->authoringWorld->HasInstance(entity);
}

//------------------------------------------------------------------------------
/**
    The components an entity has are the attributes of the table its instance is in.
*/
const Util::Array<Game::ComponentId>&
Scene::GetComponents(Game::Entity entity)
{
    Game::EntityMapping const mapping = this->authoringWorld->GetEntityMapping(entity);
    return this->authoringWorld->GetDatabase()->GetTable(mapping.table).GetAttributes();
}

//------------------------------------------------------------------------------
/**
    The data of a component on an entity in the authoring world.
*/
void*
Scene::GetComponentData(Game::Entity entity, Game::ComponentId component)
{
    Game::EntityMapping const mapping = this->authoringWorld->GetEntityMapping(entity);
    MemDb::Table& table = this->authoringWorld->GetDatabase()->GetTable(mapping.table);
    return table.GetValuePointer(table.GetAttributeIndex(component), mapping.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Collection>&
Scene::GetCollections() const
{
    return this->collections;
}

//------------------------------------------------------------------------------
/**
    A guid is only a collection if it is one, so this tells a collection guid and an entity guid
    apart.
*/
IndexT
Scene::FindCollection(const Util::Guid& guid) const
{
    if (!guid.IsValid())
        return InvalidIndex;

    for (IndexT i = 0; i < this->collections.Size(); i++)
    {
        if (this->collections[i].guid == guid)
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    The order is the position among the siblings, in creation order.
*/
Util::Guid
Scene::CreateCollection(const Util::String& name, const Util::Guid& parent)
{
    Collection record;
    record.guid.Generate();
    record.name = name;
    record.parent = parent;
    record.order = (IndexT)this->collections.Size();
    return this->CreateCollection(record, InvalidIndex);
}

//------------------------------------------------------------------------------
/**
    A redo puts the collection back at the index it was taken out of, because the outline draws
    collections in array order.
*/
Util::Guid
Scene::CreateCollection(const Collection& record, IndexT index)
{
    if (this->FindCollection(record.guid) != InvalidIndex)
        return Util::Guid();

    if (index == InvalidIndex || index > (IndexT)this->collections.Size())
        this->collections.Append(record);
    else
        this->collections.Insert(index, record);

    this->version++;
    return record.guid;
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::SetCollectionName(const Util::Guid& guid, const Util::String& name)
{
    const IndexT index = this->FindCollection(guid);
    if (index == InvalidIndex)
        return false;

    this->collections[index].name = name;
    this->version++;
    return true;
}

//------------------------------------------------------------------------------
/**
    A collection cannot be inside itself, so the parent chain is walked first.
*/
bool
Scene::SetCollectionParent(const Util::Guid& guid, const Util::Guid& parent)
{
    const IndexT index = this->FindCollection(guid);
    if (index == InvalidIndex)
        return false;

    IndexT ancestor = this->FindCollection(parent);
    SizeT depth = 0;
    while (ancestor != InvalidIndex && depth++ <= this->collections.Size())
    {
        if (ancestor == index)
            return false;
        ancestor = this->FindCollection(this->collections[ancestor].parent);
    }

    this->collections[index].parent = parent;
    this->version++;
    return true;
}

//------------------------------------------------------------------------------
/**
    Deleting a collection does not delete what is in it, it moves the children and the entities to
    wherever the collection was.
*/
bool
Scene::DeleteCollection(const Util::Guid& guid)
{
    const IndexT index = this->FindCollection(guid);
    if (index == InvalidIndex)
        return false;

    const Util::Guid parent = this->collections[index].parent;

    for (IndexT i = 0; i < this->collections.Size(); i++)
    {
        if (this->collections[i].parent == guid)
            this->collections[i].parent = parent;
    }

    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        if (this->editables[i].guid.IsValid() && this->editables[i].collection == guid)
            this->editables[i].collection = parent;
    }

    if (this->activeCollection == guid)
        this->activeCollection = parent;

    this->collections.EraseIndex(index);
    this->version++;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::MoveEntityToCollection(Game::Entity entity, const Util::Guid& collection)
{
    if (entity.index >= this->editables.Size() || !this->editables[entity.index].guid.IsValid())
        return;

    this->editables[entity.index].collection = collection;
    this->version++;

    // An entity with no parent is placed by its collection, so moving it into an instance's
    // collection
    // puts it in the instance.
    this->ResolveInstanceMembership(entity);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Guid&
Scene::GetActiveCollection() const
{
    return this->activeCollection;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::SetActiveCollection(const Util::Guid& guid)
{
    this->activeCollection = guid;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
Scene::GetParent(Game::Entity entity) const
{
    if (!this->authoringWorld->HasComponent<Game::HTransform>(entity))
        return Game::Entity::Invalid();

    return this->authoringWorld->GetComponent<Game::HTransform>(entity).parent;
}

//------------------------------------------------------------------------------
/**
    An entity cannot be its own ancestor, so the parent chain is walked first. The component is
    added when the entity does not have it.

    The entity keeps its place in the world, which is what makes a drop in the outline a reparent
    instead of a jump. The local values are written as the world values read through the new parent,
    the way the hierarchy resolves them back. A parent with a zero scale has no inverse, so it
    cannot be a parent.
*/
bool
Scene::SetParent(Game::Entity entity, Game::Entity parent)
{
    if (!this->authoringWorld->IsValid(entity) || !this->authoringWorld->IsValid(parent))
        return false;

    if (entity == parent || this->IsDescendantOf(entity, parent))
        return false;

    const Editable& editable = this->GetEditable(entity);

    // Membership follows the parent, and with no parent the collection decides it. An entity with a
    // record stands where its file puts it, so only the root of an instantiation can be placed by
    // the
    // level.
    if (this->IsPrefabInstanceEntity(entity) && editable.prefabEntity.IsValid() && !this->IsPrefabInstanceRoot(entity))
    {
        const IndexT instanceIndex = editable.prefabInstance;
        if (parent.index >= this->editables.Size() || this->editables[parent.index].prefabInstance != instanceIndex)
        {
            n_warning("Cannot move '%s' out of its prefab instance. Unpack the instance to move it\n",
                editable.name.AsCharPtr());
            return false;
        }
    }

    if (!this->authoringWorld->HasComponent<Game::HTransform>(entity))
        this->AddComponent(entity, Game::GetComponentId<Game::HTransform>(), nullptr);

    Game::HTransform transform = this->authoringWorld->GetComponent<Game::HTransform>(entity);
    if (transform.parent == parent)
        return false;

    Game::Scale const parentScale = this->authoringWorld->GetComponent<Game::Scale>(parent);
    if (Math::abs(parentScale.x) < 0.000001f || Math::abs(parentScale.y) < 0.000001f || Math::abs(parentScale.z) < 0.000001f)
    {
        n_warning("Cannot parent an entity under a transform with zero scale\n");
        return false;
    }

    Game::Position const position = this->authoringWorld->GetComponent<Game::Position>(entity);
    Game::Orientation const orientation = this->authoringWorld->GetComponent<Game::Orientation>(entity);
    Game::Scale const scale = this->authoringWorld->GetComponent<Game::Scale>(entity);

    Game::HTransform local = this->LocalTransform(this->authoringWorld, entity, parent, position, orientation, scale);

    this->SetComponent(entity, Game::GetComponentId<Game::HTransform>(), &local);

    // Resolved after the assignment, because it is the parent the entity now has that decides it
    this->ResolveInstanceMembership(entity);
    return true;
}

//------------------------------------------------------------------------------
/**
    Clearing the parent keeps the entity where it is, by writing the world values into the local
    ones. The values it had were read through the parent it is losing, and on their own they would
    put it back at the origin.
*/
bool
Scene::ClearParent(Game::Entity entity)
{
    if (!this->authoringWorld->HasComponent<Game::HTransform>(entity))
        return false;

    // The same rule as setting a parent: an entity inside an instantiation has the parent its file
    // gives it. An addition has no record, so the level places it, and clearing its parent puts it
    // back in its collection.
    const Editable& cleared = this->GetEditable(entity);
    if (this->IsPrefabInstanceEntity(entity) && cleared.prefabEntity.IsValid() && !this->IsPrefabInstanceRoot(entity))
    {
        n_warning("Cannot clear the parent of '%s', it is inside a prefab instance\n", cleared.name.AsCharPtr());
        return false;
    }

    Game::HTransform transform = this->authoringWorld->GetComponent<Game::HTransform>(entity);
    if (transform.parent == Game::Entity::Invalid())
        return false;

    Game::Position const position = this->authoringWorld->GetComponent<Game::Position>(entity);
    Game::Orientation const orientation = this->authoringWorld->GetComponent<Game::Orientation>(entity);
    Game::Scale const scale = this->authoringWorld->GetComponent<Game::Scale>(entity);

    Game::HTransform local = this->LocalTransform(this->authoringWorld, entity, Game::Entity::Invalid(), position, orientation, scale);

    this->SetComponent(entity, Game::GetComponentId<Game::HTransform>(), &local);

    this->ResolveInstanceMembership(entity);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::IsDescendantOf(Game::Entity entity, Game::Entity ancestor) const
{
    SizeT depth = 0;
    while (this->authoringWorld->HasComponent<Game::HTransform>(ancestor) && depth++ <= this->editables.Size())
    {
        Game::Entity const parent = this->authoringWorld->GetComponent<Game::HTransform>(ancestor).parent;
        if (parent == Game::Entity::Invalid() || !this->authoringWorld->IsValid(parent))
            return false;
        if (parent == entity)
            return true;
        ancestor = parent;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    The local transform an entity needs to stand at one of its world values. Which of the three the
    value is decides which field it becomes. For an entity with a parent the world components are
    the result of the hierarchy, so the field is the value read through the parent. Without a
    parent, or with a parent that has no inverse, the answer is the value as given.
*/
Game::HTransform
Scene::LocalTransform(Game::World* world, Game::Entity entity, Game::Entity parent, const Math::vec3& position,
    const Math::quat& orientation, const Math::vec3& scale) const
{
    const Game::ComponentId positionId = Game::GetComponentId<Game::Position>();
    const Game::ComponentId orientationId = Game::GetComponentId<Game::Orientation>();
    const Game::ComponentId scaleId = Game::GetComponentId<Game::Scale>();

    Game::Position parentPosition(Math::vec3(0));
    Game::Orientation parentOrientation;
    Game::Scale parentScale;

    if (parent != Game::Entity::Invalid() && world->IsValid(parent) && world->HasInstance(parent) &&
        world->HasComponent(parent, positionId) && world->HasComponent(parent, orientationId) &&
        world->HasComponent(parent, scaleId))
    {
        Game::Position const readPosition = world->GetComponent<Game::Position>(parent);
        Game::Orientation const readOrientation = world->GetComponent<Game::Orientation>(parent);
        Game::Scale const readScale = world->GetComponent<Game::Scale>(parent);

        // A parent with a zero scale has no inverse, so there is nothing to read through it
        if (Math::abs(readScale.x) >= 0.000001f && Math::abs(readScale.y) >= 0.000001f &&
            Math::abs(readScale.z) >= 0.000001f)
        {
            parentPosition = readPosition;
            parentOrientation = readOrientation;
            parentScale = readScale;
        }
    }

    Math::mat4 const parentTransform = Math::trs(parentPosition, parentOrientation, parentScale);

    Game::HTransform local = world->GetComponent<Game::HTransform>(entity);
    local.parent = parent;
    local.localPosition = Math::vec3(Math::inverse(parentTransform) * Math::point(position));
    local.localOrientation = Math::inverse(parentOrientation) * orientation;
    local.localScale = Math::vec3(scale.x / parentScale.x, scale.y / parentScale.y, scale.z / parentScale.z);

    return local;
}

//------------------------------------------------------------------------------
/**
    A value goes into the authoring row and the sim world. The merge and the materialize write
    through here rather than SetComponent, which would mark what the chain gave as the instance's
    own.
*/
void
Scene::WriteComponentValue(Game::Entity entity, Game::ComponentId component, const void* value)
{
    this->authoringWorld->SetComponentValue(entity, component, (void*)value, MemDb::AttributeRegistry::TypeSize(component));

    Editable& editable = this->editables[entity.index];
    for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
    {
        PrefabOverride& override = editable.prefabOverrides[i];
        if (override.component != component)
            continue;

        // The chain just answered with this component, so it is neither added nor removed here
        override.componentAdded = false;
        override.componentRemoved = false;
        break;
    }

    this->MirrorComponent(entity, component);
    this->version++;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
Scene::FindFieldIndex(Game::ComponentInterface* interface, const Util::String& field)
{
    for (IndexT i = 0; i < (IndexT)interface->GetNumFields(); i++)
    {
        if (field == interface->GetFieldNames()[i])
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    The bytes a field spans, up to the next field or the end of the component.
*/
SizeT
Scene::FieldSpan(Game::ComponentInterface* interface, Game::ComponentId component, IndexT field)
{
    const SizeT offset = (SizeT)interface->GetFieldByteOffsets()[field];
    if (field + 1 < (IndexT)interface->GetNumFields())
        return (SizeT)interface->GetFieldByteOffsets()[field + 1] - offset;
    return MemDb::AttributeRegistry::TypeSize(component) - offset;
}

//------------------------------------------------------------------------------
/**
    What one record's component says about a value: the component leaves the chain when the record
    removed it, joins it when the record added it, and its fields overwrite the bytes. Its mark says
    whether the component is in the chain at this point.
*/
void
Scene::MergeRecordComponent(const DocumentComponent& component, bool& present, Util::Blob& value)
{
    const Game::ComponentId componentId = component.component;
    const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(componentId);

    if (component.removedFromPrefab)
    {
        present = false;
        return;
    }

    if (!present)
    {
        // The component joins the chain at this record. The file carries it whole, and the value
        // starts at the default.
        if (typeSize == 0)
        {
            present = true;
            return;
        }
        value = Util::Blob(MemDb::AttributeRegistry::DefaultValue(componentId), typeSize);
        present = true;
    }

    if (typeSize == 0 || !component.value.IsValid())
        return;

    Game::ComponentInterface* interface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(componentId));
    byte* dst = (byte*)value.GetPtr();
    const byte* src = (const byte*)component.value.GetPtr();

    for (IndexT field = 0; field < (IndexT)interface->GetNumFields(); field++)
    {
        if (!component.presentFields.IsSet((uint64_t)field))
            continue;
        Memory::Copy(src + interface->GetFieldByteOffsets()[field], dst + interface->GetFieldByteOffsets()[field],
            FieldSpan(interface, componentId, field));
    }
}

//------------------------------------------------------------------------------
/**
    A prefab file is read once and kept, because every entity of an instance walks the same chain.
    The copy goes when the file is written, so the next read sees the new contents.
*/
Ptr<SceneDocument>
Scene::LoadDocumentCached(const Util::String& path) const
{
    IndexT index;
    if (this->documentCache.Contains(path, index))
        return this->documentCache.ValueAtIndex(index);

    Ptr<SceneDocument> doc = SceneDocument::FromFile(path);
    if (doc.isvalid())
        this->documentCache.Add(path, doc);
    return doc;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::InvalidateDocument(const Util::String& path)
{
    if (this->documentCache.Contains(path))
        this->documentCache.Erase(path);
}

//------------------------------------------------------------------------------
/**
    The chain of records that describes an entity, from the record named in the prefab file down to
    the plain record it ends at. Each hop carries the entities its file was created as, which turns
    the guids inside the values back into entities. The walk stops at the first file it has already
    read, so a prefab that instances itself comes back unresolved instead of never coming back.
*/
bool
Scene::WalkChain(const Util::String& source, const Util::Guid& entityGuid, Util::Array<ChainHop>& hops,
    IndexT instanceIndex) const
{
    if (entityGuid == Util::Guid())
        return false;

    Util::String path = source;
    Util::Guid guid = entityGuid;

    for (IndexT guard = 0; guard < MaxPrefabChainDepth; guard++)
    {
        Ptr<SceneDocument> doc = this->LoadDocumentCached(path);
        if (!doc.isvalid())
            return false;

        const IndexT record = doc->FindEntity(guid);
        if (record == InvalidIndex)
        {
            n_warning("Prefab file '%s' has no entity with guid '%s'\n", path.AsCharPtr(), guid.AsString().AsCharPtr());
            return false;
        }

        ChainHop hop;
        hop.doc = doc;
        hop.docEntity = record;

        // A file's values point at its own entities, so a hop that answers with values carries the
        // entities this scene created that file's records as.
        if (instanceIndex != InvalidIndex)
            this->BuildFileMap(instanceIndex, doc->source, hop.fileMap);

        hops.Append(hop);

        const DocumentEntity& entity = doc->entities[record];

        // A record that names itself is the top of the chain, which is what a file's own record is
        if (entity.prefabEntity == entity.guid)
            return true;

        if (entity.prefabSource.IsEmpty())
            return true;

        // A file that names itself, or a chain that comes back to one it has read
        for (IndexT h = 0; h < hops.Size(); h++)
        {
            if (hops[h].doc->source == entity.prefabSource)
            {
                n_warning("Prefab chain from '%s' comes back to '%s', which makes it a cycle\n",
                    source.AsCharPtr(), entity.prefabSource.AsCharPtr());
                return false;
            }
        }

        path = entity.prefabSource;
        guid = entity.prefabEntity;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
    A file whose chains do not resolve is broken: an entity with an unreadable chain has nothing to
    merge and nothing to revert to. Checking before anything is taken in lets such a file be
    refused, and the log says what is missing. A node that names a record the file does not have is
    not a broken link; the entity is simply the instance's own addition.
*/
bool
Scene::ValidatePrefabLinks(const Ptr<SceneDocument>& doc) const
{
    Util::Array<Util::Guid> checkedInstances;
    Util::Array<Util::String> checkedSources;
    Util::Dictionary<Util::String, Ptr<SceneDocument>> readDocs;

    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        DocumentEntity& record = doc->entities[i];
        if (record.prefabSource.IsEmpty())
            continue;

        // An entity the instance added has no record, which is not a broken link. Its chain is the
        // instance's file.
        if (record.addedToPrefab)
            continue;

        // One read per file, so a file that cannot be read answers once instead of once per entity.
        // The scene's cache only keeps files that came in, so the failures are remembered here.
        IndexT readIndex = readDocs.FindIndex(record.prefabSource);
        if (readIndex == InvalidIndex)
            readIndex = readDocs.Add(record.prefabSource, this->LoadDocumentCached(record.prefabSource));

        const Ptr<SceneDocument> source = readDocs.ValueAtIndex(readIndex);
        if (source.isvalid())
        {
            // A node that names a record the file does not have is the same case: the file can be
            // read, there is just nothing under the node, so the entity is the instance's own
            // addition.
            if (source->FindEntity(record.prefabEntity) == InvalidIndex)
            {
                const Util::String missing = record.prefabEntity.AsString();

                record.addedToPrefab = true;
                record.prefabEntity = Util::Guid();

                const Util::String msg = Util::String("Prefab file '") + record.prefabSource +
                    "' has no entity with guid '" + missing.AsCharPtr() +
                    "', so '" + record.name + "' is taken as the instance's own addition";
                if (this->core != nullptr)
                    this->core->Log(LogMessageType::N_WARNING, msg);
                else
                    n_warning("%s\n", msg.AsCharPtr());
                continue;
            }
        }

        bool alreadyChecked = false;
        for (IndexT c = 0; c < checkedInstances.Size(); c++)
        {
            if (checkedInstances[c] == record.instance && checkedSources[c] == record.prefabSource)
            {
                alreadyChecked = true;
                break;
            }
        }
        if (alreadyChecked)
            continue;

        checkedInstances.Append(record.instance);
        checkedSources.Append(record.prefabSource);

        bool resolves = source.isvalid();
        if (resolves)
        {
            Util::Array<ChainHop> hops;
            resolves = this->WalkChain(record.prefabSource, record.prefabEntity, hops);
        }
        if (resolves)
            continue;

        const Util::String msg = Util::String("File '") + doc->source + "' is broken: entity '" +
            record.name + "' instances the prefab file '" + record.prefabSource + "', and the " +
            "prefab chain from there does not resolve. The warning above names the file that failed";
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_ERROR, msg);
        else
            n_warning("%s\n", msg.AsCharPtr());
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    One guid per record, before anything is made from a file. A file describes one entity per guid,
    so a second record with the same guid is something the file cannot point at. The first record
    keeps the guid the file gave and the rest take a new one. The log names the file, because these
    guids are not the ones the file had and a save writes them back.
*/
void
Scene::NormalizeDocumentGuids(const Ptr<SceneDocument>& doc) const
{
    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        if (doc->FindEntity(doc->entities[i].guid) == i)
            continue;

        const Util::String msg = Util::String("Scene file '") + doc->source + "' has entity with guid '" +
            doc->entities[i].guid.AsString() + "' already! Generating new guid for entity";
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, msg);
        else
            n_warning("%s\n", msg.AsCharPtr());

        doc->entities[i].guid.Generate();
    }

    for (IndexT i = 0; i < doc->collections.Size(); i++)
    {
        if (doc->FindCollection(doc->collections[i].guid) == i)
            continue;

        const Util::String msg = Util::String("Scene file '") + doc->source + "' has collection with guid '" +
            doc->collections[i].guid.AsString() + "' already! Generating new guid for collection";
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, msg);
        else
            n_warning("%s\n", msg.AsCharPtr());

        doc->collections[i].guid.Generate();
    }
}

//------------------------------------------------------------------------------
/**
    The components the chain gives an entity: the plain record at the bottom seeds them, the records
    above merge their overrides, and this scene's own record merges last. The present flags say
    which components survive the adds and removals.
*/
void
Scene::MergeChainComponents(const DocumentEntity& record, const Util::Array<ChainHop>& hops,
    Util::Array<Game::ComponentId>& chainCids, Util::Array<Util::Blob>& values, Util::Array<bool>& present) const
{
    auto findOrAdd = [&](Game::ComponentId component) -> IndexT
    {
        for (IndexT i = 0; i < chainCids.Size(); i++)
        {
            if (chainCids[i] == component)
                return i;
        }
        chainCids.Append(component);
        values.Append(Util::Blob());
        present.Append(false);
        return chainCids.Size() - 1;
    };

    auto applyRecord = [&](const DocumentEntity& rec, const ChainHop* hop)
    {
        for (IndexT c = 0; c < rec.components.Size(); c++)
        {
            const DocumentComponent& component = rec.components[c];
            if (component.component == Game::ComponentId::Invalid())
                continue;
            const IndexT i = findOrAdd(component.component);
            MergeRecordComponent(component, present[i], values[i]);
        }

        // A record's references name entities of its own file, so they are resolved here, where the
        // file is known, rather than after the merge has composed a value out of several files.
        if (hop != nullptr && !hop->fileMap.IsEmpty())
            this->PatchRecordValueRefs(*hop, rec, chainCids, values);
    };

    if (hops.IsEmpty())
    {
        applyRecord(record, nullptr);
    }
    else
    {
        for (IndexT h = hops.Size() - 1; h >= 0; h--)
            applyRecord(hops[h].doc->entities[hops[h].docEntity], &hops[h]);
        applyRecord(record, nullptr);
    }
}

//------------------------------------------------------------------------------
/**
    The extension texts the chain gives an entity, merged bottom up. The scene composes them through
    the extensions that wrote them, because it cannot read them. An extension this build does not
    have contributes nothing.

    A text names the entities it names by the guids of the file it was read from, and a merged text can
    only name one file's records, so each text is put against targetMap before it joins. Without that,
    a text composed from a chain names guids the file it is written into has no record for.
*/
void
Scene::MergeChainExtensions(const DocumentEntity& record, const Util::Array<ChainHop>& hops,
    Util::Dictionary<Util::StringAtom, Util::String>& outTexts,
    Util::HashTable<Util::Guid, Game::Entity>& targetMap) const
{
    // The texts are read through the map they came from and written against one map, so the way back from
    // an entity to a guid has to be built once for the whole merge. HashTable has no const iterator, which
    // is why the map is not const here
    Util::Dictionary<Game::Entity, Util::Guid> toGuids;
    for (auto it = targetMap.Begin(); it != targetMap.End(); it++)
        toGuids.Add(*it.val, *it.key);

    auto applyRecord = [&](const DocumentEntity& rec, const Util::HashTable<Util::Guid, Game::Entity>& fromMap)
    {
        for (IndexT e = 0; e < rec.extensions.Size(); e++)
        {
            const DocumentExtension& entry = rec.extensions[e];
            const Game::LevelExtension* extension = Game::LevelExtensionRegistry::GetExtension(entry.name);
            if (extension == nullptr || extension->merge == nullptr || entry.json.IsEmpty())
                continue;

            Util::String json = Scene::RemapTextRefs(extension, entry.json, fromMap, toGuids);

            const IndexT existing = outTexts.FindIndex(entry.name);
            const char* base = existing == InvalidIndex ? nullptr : outTexts.ValueAtIndex(existing).AsCharPtr();

            Util::String merged;
            extension->merge(base, json.AsCharPtr(), merged);
            outTexts.Emplace(entry.name) = merged;
        }
    };

    if (hops.IsEmpty())
    {
        applyRecord(record, targetMap);
    }
    else
    {
        for (IndexT h = hops.Size() - 1; h >= 0; h--)
            applyRecord(hops[h].doc->entities[hops[h].docEntity], hops[h].fileMap);
        applyRecord(record, targetMap);
    }
}

//------------------------------------------------------------------------------
/**
    The text with its references put as the guids targetMap names the targets by. The scene cannot read a
    text, so this is the only way it can say anything about the entities one names: it asks the extension
    that wrote it for the references, resolves each through the map the text came from, and hands the guids
    back. A reference whose target has no entity in the map is given with no guid, which is what the
    extension writes as nothing.
*/
Util::String
Scene::RemapTextRefs(const Game::LevelExtension* extension, const Util::String& text,
    const Util::HashTable<Util::Guid, Game::Entity>& fromMap,
    const Util::Dictionary<Game::Entity, Util::Guid>& toGuids)
{
    if (extension == nullptr || extension->refs == nullptr || extension->remap == nullptr || text.IsEmpty())
        return text;

    Util::Array<Game::TextRef> named;
    extension->refs(text.AsCharPtr(), named);
    if (named.IsEmpty())
        return text;

    Util::String pairs;
    for (IndexT i = 0; i < named.Size(); i++)
    {
        Game::Entity target = Game::Entity::Invalid();
        const IndexT mapIndex = fromMap.FindIndex(named[i].guid);
        if (mapIndex != InvalidIndex)
            target = fromMap.ValueAtIndex(named[i].guid, mapIndex);

        if (!pairs.IsEmpty())
            pairs += "\n";
        pairs += named[i].path;
        pairs += "\t";

        const IndexT guidIndex = toGuids.FindIndex(target);
        if (guidIndex != InvalidIndex)
            pairs += toGuids.ValueAtIndex(guidIndex).AsString();
    }

    Util::String out;
    extension->remap(text.AsCharPtr(), pairs.AsCharPtr(), out);
    return out;
}

//------------------------------------------------------------------------------
/**
    The guid to entity map for one instance's file. A save or a merge asks for it again per file it
    touches.
*/
void
Scene::BuildFileMap(IndexT instanceIndex, const Util::String& docSource,
    Util::HashTable<Util::Guid, Game::Entity>& outMap) const
{
    if (instanceIndex >= this->prefabInstances.Size())
        return;

    const PrefabInstance& instance = this->prefabInstances[instanceIndex];
    for (IndexT e = 0; e < instance.entities.Size(); e++)
    {
        const Game::Entity entity = instance.entities[e];
        if (entity.index >= this->editables.Size() || !this->editables[entity.index].guid.IsValid())
            continue;

        const Editable& editable = this->editables[entity.index];
        Util::Array<ChainHop> hops;
        if (!this->WalkChain(instance.source, editable.prefabEntity, hops))
            continue;

        for (IndexT h = 0; h < hops.Size(); h++)
        {
            if (hops[h].doc->source != docSource)
                continue;

            // One entity per guid. Two records can name the same record of a file further up, which
            // is
            // what a prefab that instances a prefab twice looks like, and a map keyed by guid holds
            // one entity for it.
            const Util::Guid& recordGuid = hops[h].doc->entities[hops[h].docEntity].guid;
            if (outMap.Contains(recordGuid))
                continue;

            outMap.Add(recordGuid, entity);
        }
    }
}

//------------------------------------------------------------------------------
/**
    A record's guids resolved to the entities its file was created as, written into the entity's
    row. A guid the file does not have answers as no entity, which is what a dangling reference
    looks like.
*/
void
Scene::PatchRecordEntityRefs(IndexT docEntity, Game::Entity entity, const Ptr<SceneDocument>& doc,
    const Util::HashTable<Util::Guid, Game::Entity>& fileMap)
{
    for (IndexT i = 0; i < doc->entityRefs.Size(); i++)
    {
        const DocumentEntityRef& ref = doc->entityRefs[i];
        if (ref.entity != docEntity)
            continue;

        // A reference inside extension text is patched by the extension that wrote the text, against the
        // same file map, because the scene cannot write into a text it cannot read. Only the warning is
        // the scene's, and it is the same one a component reference gets
        if (ref.extension.IsValid())
        {
            if (ref.target.IsValid() && !fileMap.Contains(ref.target))
                n_warning("Entity reference in '%s' points at a guid that is not in that file\n",
                    doc->source.AsCharPtr());
            continue;
        }

        if (!this->authoringWorld->HasComponent(entity, ref.component))
            continue;

        Game::ComponentInterface* interface =
            static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(ref.component));
        const IndexT field = FindFieldIndex(interface, ref.field);
        if (field == InvalidIndex)
        {
            n_warning("Entity reference on '%s' names field '%s' which the component does not have\n",
                doc->source.AsCharPtr(), ref.field.AsCharPtr());
            continue;
        }

        Game::Entity target = Game::Entity::Invalid();
        const IndexT mapIndex = fileMap.FindIndex(ref.target);
        if (mapIndex != InvalidIndex)
            target = fileMap.ValueAtIndex(ref.target, mapIndex);
        else if (ref.target.IsValid())
        {
            n_warning("Entity reference in '%s' points at a guid that is not in that file\n",
                doc->source.AsCharPtr());
        }

        byte* data = (byte*)this->GetComponentData(entity, ref.component);
        Memory::Copy(&target, data + interface->GetFieldByteOffsets()[field], sizeof(Game::Entity));
    }
}

//------------------------------------------------------------------------------
/**
    The same resolution, but into the value the chain is building instead of a world row. Only a
    field the record carries is written, so a file that named no reference does not overwrite one a
    lower file named.
*/
void
Scene::PatchRecordValueRefs(const ChainHop& hop, const DocumentEntity& record,
    Util::Array<Game::ComponentId>& chainCids, Util::Array<Util::Blob>& values) const
{
    for (IndexT i = 0; i < hop.doc->entityRefs.Size(); i++)
    {
        const DocumentEntityRef& ref = hop.doc->entityRefs[i];
        if (ref.entity != hop.docEntity)
            continue;

        // A reference inside extension text is put into the value by the extension that wrote the text,
        // when the scene composes the text through the chain
        if (ref.extension.IsValid())
            continue;

        IndexT value = InvalidIndex;
        for (IndexT c = 0; c < chainCids.Size(); c++)
        {
            if (chainCids[c] == ref.component)
            {
                value = c;
                break;
            }
        }
        if (value == InvalidIndex || !values[value].IsValid())
            continue;

        const IndexT componentIndex = hop.doc->FindComponent(hop.docEntity, ref.component);
        if (componentIndex == InvalidIndex)
            continue;

        Game::ComponentInterface* interface =
            static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(ref.component));
        if (interface == nullptr)
            continue;

        const IndexT field = FindFieldIndex(interface, ref.field);
        if (field == InvalidIndex || !record.components[componentIndex].presentFields.IsSet((uint64_t)field))
            continue;

        Game::Entity target = Game::Entity::Invalid();
        const IndexT mapIndex = hop.fileMap.FindIndex(ref.target);
        if (mapIndex != InvalidIndex)
            target = hop.fileMap.ValueAtIndex(ref.target, mapIndex);

        byte* dst = (byte*)values[value].GetPtr() + interface->GetFieldByteOffsets()[field];
        Memory::Copy(&target, dst, sizeof(Game::Entity));
    }
}

//------------------------------------------------------------------------------
/**
    A value the document holds no longer answers for its own entity once it is rewritten, so the
    references read out of it go with it.
*/
void
Scene::DropDocumentRefs(const Ptr<SceneDocument>& doc, IndexT docEntity, Game::ComponentId component) const
{
    for (IndexT i = (IndexT)doc->entityRefs.Size() - 1; i >= 0; i--)
    {
        if (doc->entityRefs[i].entity == docEntity && doc->entityRefs[i].component == component)
            doc->entityRefs.EraseIndex(i);
    }
}

//------------------------------------------------------------------------------
/**
    The references one extension's text names for a document entity. The ones read from the text the new
    text replaces go first, because a rewritten text is not the text they were read from, and a stale
    reference would be written into the file as a guid the text no longer names.
*/
void
Scene::RecordDocumentRefs(const Ptr<SceneDocument>& doc, IndexT docEntity, Util::StringAtom extension,
    const Util::String& text)
{
    for (IndexT i = (IndexT)doc->entityRefs.Size() - 1; i >= 0; i--)
    {
        if (doc->entityRefs[i].entity == docEntity && doc->entityRefs[i].extension == extension)
            doc->entityRefs.EraseIndex(i);
    }

    const Game::LevelExtension* extensionHook = Game::LevelExtensionRegistry::GetExtension(extension);
    if (extensionHook == nullptr || extensionHook->refs == nullptr || text.IsEmpty())
        return;

    Util::Array<Game::TextRef> named;
    extensionHook->refs(text.AsCharPtr(), named);
    for (IndexT i = 0; i < named.Size(); i++)
    {
        DocumentEntityRef ref;
        ref.entity = docEntity;
        ref.extension = extension;
        ref.field = named[i].path;
        ref.target = named[i].guid;
        doc->entityRefs.Append(ref);
    }
}

//------------------------------------------------------------------------------
/**
    Put a document's extension entries back onto an entity. An entry the registry has no extension
    for is kept in the document but not applied, because the scene cannot read it.
*/
void
Scene::ApplyExtensions(const Util::String& entityName, const Util::Array<DocumentExtension>& extensions, Game::Entity entity,
    Util::HashTable<Util::Guid, Game::Entity>& map)
{
    if (!map.IsEmpty())
        this->SetRefContext(map);

    for (IndexT i = 0; i < extensions.Size(); i++)
    {
        const DocumentExtension& entry = extensions[i];

        const Game::LevelExtension* extension = Game::LevelExtensionRegistry::GetExtension(entry.name);
        if (extension == nullptr)
        {
            n_warning("Scene file entity '%s' has extension data for '%s', which no feature registered\n",
                entityName.AsCharPtr(), entry.name.Value());
            continue;
        }

        if (extension->apply == nullptr || entry.json.IsEmpty())
            continue;

        extension->apply(entity, entry.json.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    The scene's guid map, which is what a text this scene wrote names its entities by. A paste or an
    undo has no file to read a map from, so it uses this.
*/
void
Scene::BuildSceneMap(Util::HashTable<Util::Guid, Game::Entity>& outMap) const
{
    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        if (!this->editables[i].guid.IsValid())
            continue;

        outMap.Add(this->editables[i].guid, this->editables[i].entity);
    }
}

//------------------------------------------------------------------------------
/**
    Give the extensions the guid map they can read. The scene cannot read an extension's text, so
    the map has to reach the side that can. Every extension gets the same pairs.
*/
void
Scene::SetRefContext(Util::HashTable<Util::Guid, Game::Entity>& map)
{
    const Util::Array<Game::LevelExtension>& extensions = Game::LevelExtensionRegistry::GetAllExtensions();

    bool any = false;
    for (IndexT i = 0; i < extensions.Size(); i++)
    {
        if (extensions[i].refContext != nullptr)
        {
            any = true;
            break;
        }
    }
    if (!any)
        return;

    Util::String pairs;
    for (auto it = map.Begin(); it != map.End(); it++)
    {
        if (!pairs.IsEmpty())
            pairs += ";";

        Util::String id;
        id.Format("%llu", (unsigned long long)uint64_t(*it.val));

        pairs += it.key->AsString();
        pairs += ":";
        pairs += id;
    }

    for (IndexT i = 0; i < extensions.Size(); i++)
    {
        if (extensions[i].refContext != nullptr)
            extensions[i].refContext(pairs.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    The text and the keys one extension's chain gives an entity. The scene composes the text through
    the extension because it cannot read it. The keys come from what the records say they carry,
    which is the only way the scene can know them, and they tell a property the instance wrote from
    one the chain gives. This scene's own record is left out, and the chain's texts are put against
    this scene's guids, because the text composed from them is applied against this scene's map and a
    text can only name one file's records.
*/
void
Scene::GetChainExtension(Game::Entity entity, Util::StringAtom extension, Util::String& outText,
    Util::Array<Util::String>& outKeys)
{
    const Editable& editable = this->GetEditable(entity);
    if (editable.prefabInstance == InvalidIndex || editable.prefabInstance >= this->prefabInstances.Size() ||
        !this->prefabInstances[editable.prefabInstance].valid)
    {
        return;
    }

    Util::Array<ChainHop> hops;
    if (!this->WalkChain(this->prefabInstances[editable.prefabInstance].source, editable.prefabEntity, hops,
        editable.prefabInstance))
    {
        return;
    }

    Util::HashTable<Util::Guid, Game::Entity> map;
    this->BuildSceneMap(map);
    this->SetRefContext(map);

    const DocumentEntity noSceneRecord;
    Util::Dictionary<Util::StringAtom, Util::String> texts;
    this->MergeChainExtensions(noSceneRecord, hops, texts, map);

    const IndexT i = texts.FindIndex(extension);
    if (i != InvalidIndex)
        outText = texts.ValueAtIndex(i);

    for (IndexT h = 0; h < hops.Size(); h++)
    {
        const DocumentEntity& record = hops[h].doc->entities[hops[h].docEntity];
        const IndexT node = hops[h].doc->FindExtension(hops[h].docEntity, extension);
        if (node == InvalidIndex)
            continue;

        const DocumentExtension& entry = record.extensions[node];
        for (IndexT f = 0; f < entry.fields.Size(); f++)
        {
            bool known = false;
            for (IndexT k = 0; k < outKeys.Size(); k++)
            {
                if (outKeys[k] == entry.fields[f])
                {
                    known = true;
                    break;
                }
            }
            if (!known)
                outKeys.Append(entry.fields[f]);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Whether the scene's mark says an entity wrote one key. The mark is the scene's and an extension
    cannot read it, so this is how a drawn row knows it is the instance's own. With no path it asks
    whether the extension wrote anything. Only a linked entity has a mark.
*/
bool
Scene::IsKeyAuthored(Game::Entity entity, Util::StringAtom extension, const Util::String& path) const
{
    const PrefabOverride& override = this->FindExtensionOverride(entity, extension);
    if (override.extension != extension)
        return false;

    const IndexT dot = path.FindCharIndex('.');
    if (dot == InvalidIndex)
    {
        const Util::String prefix = path + ".";
        for (IndexT i = 0; i < override.fields.Size(); i++)
        {
            if (override.fields[i].BeginsWithString(prefix))
                return true;
        }
        return false;
    }

    for (IndexT i = 0; i < override.fields.Size(); i++)
    {
        if (override.fields[i] == path)
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
    The text each registered extension writes for an entity. The scene does not know what that text
    is, which is what lets an extension it knows nothing about go through the same document as the
    components.
*/
void
Scene::CaptureExtensions(Game::Entity entity, Util::Array<DocumentExtension>& out,
    Util::HashTable<Util::Guid, Game::Entity>& map)
{
    this->SetRefContext(map);

    const Util::Array<Game::LevelExtension>& extensions = Game::LevelExtensionRegistry::GetAllExtensions();
    for (IndexT i = 0; i < extensions.Size(); i++)
    {
        const Game::LevelExtension& extension = extensions[i];
        if (extension.capture == nullptr)
            continue;

        Util::String json;
        extension.capture(entity, json);
        if (json.IsEmpty())
            continue;

        DocumentExtension entry;
        entry.name = extension.name;
        entry.json = json;
        if (extension.keys != nullptr)
            extension.keys(json.AsCharPtr(), entry.fields);
        out.Append(entry);
    }
}

//------------------------------------------------------------------------------
/**
    The text for only the given keys, which is what a linked entity writes. Writing the whole value
    would make it stop following the prefab.
*/
void
Scene::CaptureExtensionKeys(Game::Entity entity, Util::StringAtom extensionName, const Util::Array<Util::String>& keys,
    Util::String& out)
{
    const Game::LevelExtension* extension = Game::LevelExtensionRegistry::GetExtension(extensionName);
    if (extension == nullptr || extension->captureKeys == nullptr || keys.IsEmpty())
        return;

    extension->captureKeys(entity, keys, out);
}

//------------------------------------------------------------------------------
/**
    A document that was read has its references nulled in the values and kept as guids, because at
    the read there were no entities to point at. Writing it again puts the guids back as record
    indexes, and answers with the document's guids by index.
*/
void
Scene::PrepareDocumentForWrite(const Ptr<SceneDocument>& doc, Util::Array<Util::Guid>& guidByEntityIndex) const
{
    guidByEntityIndex.Resize(doc->entities.Size());
    for (IndexT i = 0; i < doc->entities.Size(); i++)
        guidByEntityIndex[i] = doc->entities[i].guid;

    for (IndexT i = 0; i < doc->entityRefs.Size(); i++)
    {
        const DocumentEntityRef& ref = doc->entityRefs[i];
        if (ref.entity == InvalidIndex || ref.entity >= doc->entities.Size())
            continue;

        // A reference inside extension text is put back by the extension when the document is written,
        // because the writer cannot read the text it sits in
        if (ref.extension.IsValid())
            continue;

        const IndexT componentIndex = doc->FindComponent(ref.entity, ref.component);
        if (componentIndex == InvalidIndex)
            continue;

        DocumentComponent& component = doc->entities[ref.entity].components[componentIndex];
        if (!component.value.IsValid())
            continue;

        Game::ComponentInterface* interface =
            static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(ref.component));
        if (interface == nullptr)
            continue;

        const IndexT field = FindFieldIndex(interface, ref.field);
        if (field == InvalidIndex || !component.presentFields.IsSet((uint64_t)field))
            continue;

        Game::Entity placeholder = Game::Entity::Invalid();
        const IndexT targetIndex = doc->FindEntity(ref.target);
        if (targetIndex != InvalidIndex)
            placeholder = Game::Entity(this->authoringWorld->GetWorldId(), 0, (uint64_t)targetIndex);

        byte* dst = (byte*)component.value.GetPtr() + interface->GetFieldByteOffsets()[field];
        Memory::Copy(&placeholder, dst, sizeof(Game::Entity));
    }
}

//------------------------------------------------------------------------------
/**
    The record the file has for an entity takes the fields the entity wrote. Plain fields go in as
    bytes, a reference goes in as the record the same file has for the target, and a reference to an
    entity the file has no record for stays out and remains the level's.
*/
bool
Scene::ApplyRecordToDocument(const Editable& editable, IndexT instanceIndex, const Ptr<SceneDocument>& doc)
{
    const IndexT docEntity = doc->FindEntity(editable.prefabEntity);
    if (docEntity == InvalidIndex)
    {
        n_warning("Prefab file '%s' has no entity with guid '%s'\n", doc->source.AsCharPtr(),
            editable.prefabEntity.AsString().AsCharPtr());
        return false;
    }

    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();
    DocumentEntity& record = doc->entities[docEntity];
    bool touched = false;

    const Util::Array<Game::ComponentId>& components = this->GetComponents(editable.entity);
    for (IndexT c = 0; c < components.Size(); c++)
    {
        const Game::ComponentId component = components[c];
        if (component == ownerId)
            continue;

        const PrefabOverride& state = this->FindPrefabOverride(editable.entity, component);

        // Nothing written against the chain is nothing to write: the entity already holds what the
        // chain gives.
        if (state.component != component ||
            (state.overriddenFields.IsNull() && !state.componentAdded))
        {
            continue;
        }

        Game::ComponentInterface* interface =
            static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
        if (interface == nullptr)
            continue;

        const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
        const byte* current = typeSize > 0
            ? (const byte*)this->GetComponentData(editable.entity, component) : nullptr;

        // the value starts at what the chain gives and the entity's own fields go over it
        Util::Blob value;
        if (typeSize > 0)
        {
            Util::Blob base;
            if (this->GetPrefabBaseValue(editable.entity, component, base) && base.IsValid())
                value = base;
            else
                value = Util::Blob(MemDb::AttributeRegistry::DefaultValue(component), typeSize);
        }

        Util::BitField<MAX_COMPONENT_FIELDS> present;
        const IndexT numFields = (IndexT)interface->GetNumFields();

        for (IndexT field = 0; field < numFields; field++)
        {
            if (!state.componentAdded && !state.overriddenFields.IsSet((uint64_t)field))
                continue;

            const SizeT offset = (SizeT)interface->GetFieldByteOffsets()[field];

            if (Util::StringAtom(interface->GetFieldTypenames()[field]) == "Game::Entity"_atm)
            {
                Game::Entity target = Game::Entity::Invalid();
                if (current != nullptr)
                    target = *(const Game::Entity*)(current + offset);

                Game::Entity placeholder = Game::Entity::Invalid();
                if (target != Game::Entity::Invalid())
                {
                    // the file can only point at its own records, and the entity of the instance
                    // has
                    // one.
                    if (target.index >= this->editables.Size() ||
                        this->editables[target.index].prefabInstance != instanceIndex)
                    {
                        continue;
                    }

                    const IndexT targetRecord =
                        doc->FindEntity(this->editables[target.index].prefabEntity);
                    if (targetRecord == InvalidIndex)
                        continue;

                    placeholder = Game::Entity(this->authoringWorld->GetWorldId(), 0, (uint64_t)targetRecord);
                }

                if (current != nullptr)
                    Memory::Copy(&placeholder, (byte*)value.GetPtr() + offset, sizeof(Game::Entity));
            }
            else if (current != nullptr)
            {
                Memory::Copy(current + offset, (byte*)value.GetPtr() + offset,
                    this->FieldSpan(interface, component, field));
            }

            present.SetBit((uint64_t)field);
        }

        this->DropDocumentRefs(doc, docEntity, component);

        const IndexT node = doc->FindComponent(docEntity, component);
        if (node != InvalidIndex)
        {
            record.components[node].value = value;
            record.components[node].presentFields = present;
            record.components[node].removedFromPrefab = false;
            record.components[node].addedToPrefab = state.componentAdded;
        }
        else
        {
            DocumentComponent recordComponent;
            recordComponent.component = component;
            recordComponent.value = value;
            recordComponent.presentFields = present;
            recordComponent.addedToPrefab = state.componentAdded;
            record.components.Append(recordComponent);
        }

        touched = true;
    }

    // a component the instance removed against its chain: the record stops giving it. A linked
    // record
    // says so with a mark, a plain one simply stops carrying it.
    for (IndexT o = 0; o < editable.prefabOverrides.Size(); o++)
    {
        const PrefabOverride& state = editable.prefabOverrides[o];
        if (!state.componentRemoved)
            continue;

        const IndexT node = doc->FindComponent(docEntity, state.component);
        if (node != InvalidIndex && record.components[node].removedFromPrefab)
            continue;

        this->DropDocumentRefs(doc, docEntity, state.component);
        if (node != InvalidIndex)
            record.components.EraseIndex(node);

        if (!record.prefabSource.IsEmpty())
        {
            DocumentComponent removed;
            removed.component = state.component;
            removed.removedFromPrefab = true;
            record.components.Append(removed);
        }

        touched = true;
    }

    // What the instance wrote against an extension's text goes back the same way a component's
    // fields
    // do: the record takes the keys the entity wrote. The scene cannot read the text, so the
    // extension
    // writes them, and the references go in as the guids this file has for their targets.
    Util::HashTable<Util::Guid, Game::Entity> map;
    this->BuildFileMap(instanceIndex, doc->source, map);
    this->SetRefContext(map);

    for (IndexT o = 0; o < editable.prefabOverrides.Size(); o++)
    {
        const PrefabOverride& state = editable.prefabOverrides[o];
        if (state.extension == Util::StringAtom())
            continue;

        const Game::LevelExtension* extension = Game::LevelExtensionRegistry::GetExtension(state.extension);
        if (extension == nullptr || extension->captureKeys == nullptr)
            continue;

        if (state.fields.IsEmpty() && !state.componentAdded)
            continue;

        Util::String json;
        this->CaptureExtensionKeys(editable.entity, state.extension, state.fields, json);
        if (json.IsEmpty())
            continue;

        const IndexT node = doc->FindExtension(docEntity, state.extension);
        if (node != InvalidIndex)
        {
            record.extensions[node].json = json;
            record.extensions[node].fields = state.fields;
        }
        else
        {
            DocumentExtension entry;
            entry.name = state.extension;
            entry.json = json;
            entry.fields = state.fields;
            record.extensions.Append(entry);
        }

        touched = true;
    }

    return touched;
}

//------------------------------------------------------------------------------
/**
    What the instance wrote goes back into its file, the way Unity's Apply works: each record takes
    the fields its entity changed, and the override state is then recomputed against the chain,
    which drops what the file took and keeps what it could not. The values do not move, so the level
    reads as the same level with a fuller prefab.
*/
bool
Scene::ApplyInstanceToPrefab(Game::Entity entity)
{
    const IndexT instanceIndex = this->GetPrefabInstanceIndex(entity);
    if (instanceIndex == InvalidIndex || instanceIndex >= this->prefabInstances.Size())
        return false;

    const PrefabInstance& instance = this->prefabInstances[instanceIndex];
    if (!instance.valid)
        return false;

    Ptr<SceneDocument> doc = this->LoadDocumentCached(instance.source);
    if (!doc.isvalid())
        return false;

    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();

    // what an entity holds, compared against what its chain gives, entity by entity. A transform
    // write
    // on a child rewrites the parent's HTransform through the hierarchy without a mark, so the
    // state
    // is recomputed here.
    auto markInstanceFields = [&]()
    {
        for (IndexT i = 0; i < instance.entities.Size(); i++)
        {
            const Game::Entity instanceEntity = instance.entities[i];
            if (instanceEntity.index >= this->editables.Size() || !this->HasInstance(instanceEntity))
                continue;

            const Util::Array<Game::ComponentId>& components = this->GetComponents(instanceEntity);
            for (IndexT c = 0; c < components.Size(); c++)
            {
                if (components[c] == ownerId)
                    continue;

                this->MarkPrefabOverrides(instanceEntity, components[c],
                    (const byte*)this->GetComponentData(instanceEntity, components[c]));
            }
        }
    };

    // a component the instance removed, and that the chain now removes too, is nothing the instance
    // says anymore.
    auto pruneInstanceRemovals = [&]()
    {
        for (IndexT i = 0; i < instance.entities.Size(); i++)
        {
            const Game::Entity instanceEntity = instance.entities[i];
            if (instanceEntity.index >= this->editables.Size() || !this->HasInstance(instanceEntity))
                continue;

            Editable& editable = this->editables[instanceEntity.index];
            for (IndexT o = (IndexT)editable.prefabOverrides.Size() - 1; o >= 0; o--)
            {
                if (!editable.prefabOverrides[o].componentRemoved)
                    continue;

                Util::Blob base;
                if (!this->GetPrefabBaseValue(instanceEntity, editable.prefabOverrides[o].component, base))
                    editable.prefabOverrides.EraseIndex(o);
            }
        }
    };

    // the marks are what the rewrite reads, so both passes run against the chain they stand in.
    markInstanceFields();
    pruneInstanceRemovals();

    bool touched = false;

    // An entity the instance added has no record, and this write gives it one. They are written
    // before
    // the overrides, because a reference from one of them names another record of the same file.
    for (IndexT i = 0; i < instance.entities.Size(); i++)
    {
        const Game::Entity instanceEntity = instance.entities[i];
        if (instanceEntity.index >= this->editables.Size() || !this->HasInstance(instanceEntity))
            continue;

        Editable& editable = this->editables[instanceEntity.index];
        if (!editable.guid.IsValid() || editable.prefabEntity.IsValid())
            continue;

        if (this->AddRecordToDocument(editable, instanceIndex, doc))
            touched = true;
    }

    // A record the instance marked removed should not be in the file. The references that pointed
    // at
    // it go first, because they name positions rather than guids. The mark goes with the record.
    for (IndexT m = (IndexT)instance.removedRecords.Size() - 1; m >= 0; m--)
    {
        const IndexT docEntity = doc->FindEntity(instance.removedRecords[m].record);
        if (docEntity == InvalidIndex)
            continue;

        for (IndexT r = (IndexT)doc->entityRefs.Size() - 1; r >= 0; r--)
        {
            if (doc->entityRefs[r].entity == docEntity)
                doc->entityRefs.EraseIndex(r);
            else if (doc->entityRefs[r].entity > docEntity)
                doc->entityRefs[r].entity--;
        }

        doc->entities.EraseIndex(docEntity);
        this->ClearRecordRemoved(instanceIndex, instance.removedRecords[m].record);
        touched = true;
    }

    for (IndexT i = 0; i < instance.entities.Size(); i++)
    {
        const Game::Entity instanceEntity = instance.entities[i];
        if (instanceEntity.index >= this->editables.Size() || !this->HasInstance(instanceEntity))
            continue;

        const Editable& editable = this->editables[instanceEntity.index];
        if (!editable.guid.IsValid())
            continue;

        if (this->ApplyRecordToDocument(editable, instanceIndex, doc))
            touched = true;
    }

    if (touched)
    {
        Util::Array<Util::Guid> guidByEntityIndex;
        this->PrepareDocumentForWrite(doc, guidByEntityIndex);
        if (!doc->ToFile(instance.source, guidByEntityIndex))
            return false;
    }

    // The file changed, so the scene reads it again instead of trusting what it just wrote, which
    // keeps the recomputed state matching what is on disk.
    this->InvalidateDocument(instance.source);

    // An entity whose record this write took out of the file is not a copy of anything, so it is
    // the
    // instance's own addition. Every instance of this file is read against the same records.
    Ptr<SceneDocument> written = this->LoadDocumentCached(instance.source);
    if (written.isvalid())
    {
        for (IndexT i = 0; i < (IndexT)this->prefabInstances.Size(); i++)
        {
            if (!this->prefabInstances[i].valid || this->prefabInstances[i].source != instance.source)
                continue;

            for (IndexT e = 0; e < this->prefabInstances[i].entities.Size(); e++)
            {
                const Game::Entity instanceEntity = this->prefabInstances[i].entities[e];
                if (instanceEntity.index >= this->editables.Size())
                    continue;

                Editable& editable = this->editables[instanceEntity.index];
                if (!editable.prefabEntity.IsValid() || written->FindEntity(editable.prefabEntity) != InvalidIndex)
                    continue;

                editable.prefabEntity = Util::Guid();
                this->version++;
            }
        }
    }

    markInstanceFields();

    // The file cannot hold more precision than it printed, so a plain field can read back a hair
    // off
    // what the entity holds. The file answers for those fields now. A reference to an entity
    // outside
    // the instance is not the file's to answer, and stays the level's.
    for (IndexT i = 0; i < instance.entities.Size(); i++)
    {
        const Game::Entity instanceEntity = instance.entities[i];
        if (instanceEntity.index >= this->editables.Size() || !this->HasInstance(instanceEntity))
            continue;

        const Util::Array<Game::ComponentId>& components = this->GetComponents(instanceEntity);
        for (IndexT c = 0; c < components.Size(); c++)
        {
            const Game::ComponentId component = components[c];
            if (component == ownerId)
                continue;

            const PrefabOverride& state = this->FindPrefabOverride(instanceEntity, component);
            if (state.component != component || state.overriddenFields.IsNull())
                continue;

            const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
            Util::Blob base;
            if (typeSize == 0 || !this->GetPrefabBaseValue(instanceEntity, component, base) || !base.IsValid())
                continue;

            Game::ComponentInterface* interface =
                static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
            if (interface == nullptr)
                continue;

            const byte* current = (const byte*)this->GetComponentData(instanceEntity, component);
            Util::Blob merged(current, typeSize);
            byte* dst = (byte*)merged.GetPtr();

            bool changed = false;
            for (IndexT field = 0; field < (IndexT)interface->GetNumFields(); field++)
            {
                if (!state.overriddenFields.IsSet((uint64_t)field))
                    continue;
                if (Util::StringAtom(interface->GetFieldTypenames()[field]) == "Game::Entity"_atm)
                    continue;

                const SizeT offset = (SizeT)interface->GetFieldByteOffsets()[field];
                const SizeT size = this->FieldSpan(interface, component, field);
                if (BytesDiffer(current + offset, (const byte*)base.GetPtr() + offset, size))
                {
                    Memory::Copy((const byte*)base.GetPtr() + offset, dst + offset, size);
                    changed = true;
                }
            }

            if (changed)
                this->WriteComponentValue(instanceEntity, component, dst);
        }
    }

    markInstanceFields();
    pruneInstanceRemovals();

    this->version++;
    return true;
}

//------------------------------------------------------------------------------
/**
    Fill an entity from a document record. The components are added first, so the entity is in the
    right table, and the references are patched after, because every entity of the file has to exist
    before a value can point at one.
*/
void
Scene::MaterializeRecord(const DocumentEntity& record, IndexT recordIndex, const Ptr<SceneDocument>& recordDoc,
    Game::Entity entity, const Util::Array<ChainHop>& hops, Util::HashTable<Util::Guid, Game::Entity>& fileMap)
{
    Util::Array<Game::ComponentId> chainCids;
    Util::Array<Util::Blob> values;
    Util::Array<bool> present;
    this->MergeChainComponents(record, hops, chainCids, values, present);

    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();
    for (IndexT i = 0; i < chainCids.Size(); i++)
    {
        const Game::ComponentId component = chainCids[i];
        if (component == ownerId || !present[i])
            continue;

        const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
        const void* value = (typeSize > 0 && values[i].IsValid()) ? values[i].GetPtr() : nullptr;

        // The authoring world gives each entity its own set of components, so a component the
        // entity
        // already has takes the chain's value rather than going on a second time, which would put
        // the
        // same column on a table twice.
        if (this->authoringWorld->HasComponent(entity, component))
        {
            if (value != nullptr)
                this->authoringWorld->SetComponentValue(entity, component, (void*)value, typeSize);
        }
        else
            this->authoringWorld->AddComponent(entity, component, value);
    }
    this->authoringWorld->ExecuteAddComponentCommands();

    // The chain files' references point at records of their own files, resolved through the
    // entities
    // those records became. The patches run in merge order, so the last one is the record the
    // entity
    // stands in.
    const IndexT ownInstance = this->editables[entity.index].prefabInstance;
    if (ownInstance != InvalidIndex)
    {
        for (IndexT h = hops.Size() - 1; h >= 0; h--)
        {
            Util::HashTable<Util::Guid, Game::Entity> hopMap;
            this->BuildFileMap(ownInstance, hops[h].doc->source, hopMap);
            this->PatchRecordEntityRefs(hops[h].docEntity, entity, hops[h].doc, hopMap);
        }
    }

    // A record's overrides' references point at records of its own file. Instantiation has no
    // record
    // in the scene's file yet, so it passes a null document.
    if (recordDoc.isvalid())
        this->PatchRecordEntityRefs(recordIndex, entity, recordDoc, fileMap);

    this->MirrorEntity(entity);

    // A record's references name records of its own file, so the text resolves through the entities
    // those records became.
    this->ApplyExtensions(record.name, record.extensions, entity, fileMap);
    this->version++;
}

//------------------------------------------------------------------------------
/**
    Give the file a record for an entity the instance added. The whole value goes in, because there
    is no chain under it, and the references go in as the file's own records. After the write the
    entity has a record, so it is no longer an addition.
*/
bool
Scene::AddRecordToDocument(Editable& editable, IndexT instanceIndex, const Ptr<SceneDocument>& doc)
{
    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();

    DocumentEntity record;
    record.guid = editable.guid;
    record.name = editable.name;
    record.collection = editable.collection;
    record.instance = this->prefabInstances[instanceIndex].guid;

    // The record is the top of the chain, so it does not name itself. The entity takes it as its
    // record, and what the entity holds is what the record carries, which is why its fields are
    // written as values rather than as overrides.
    const IndexT docIndex = doc->entities.Size();
    doc->entities.Append(record);

    const Util::Array<Game::ComponentId>& components = this->GetComponents(editable.entity);
    for (IndexT c = 0; c < components.Size(); c++)
    {
        const Game::ComponentId component = components[c];
        if (component == ownerId)
            continue;

        Game::ComponentInterface* interface =
            static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
        if (interface == nullptr)
            continue;

        DocumentComponent recordComponent;
        recordComponent.component = component;

        const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
        if (typeSize > 0)
        {
            recordComponent.value = Util::Blob(this->GetComponentData(editable.entity, component), typeSize);

            const byte* current = (const byte*)recordComponent.value.GetPtr();
            const IndexT numFields = (IndexT)interface->GetNumFields();
            for (IndexT field = 0; field < numFields; field++)
            {
                recordComponent.presentFields.SetBit((uint64_t)field);

                if (Util::StringAtom(interface->GetFieldTypenames()[field]) != "Game::Entity"_atm)
                    continue;

                const Game::Entity target = *(const Game::Entity*)(current + interface->GetFieldByteOffsets()[field]);
                if (target == Game::Entity::Invalid())
                    continue;

                // The file can only name its own records, so a reference to an entity outside the
                // instance stays out and remains the level's. An entity with a record names that
                // record, and an addition names the record this write gives it.
                if (target.index >= this->editables.Size())
                    continue;

                const Editable& targetEditable = this->editables[target.index];
                if (targetEditable.prefabInstance != instanceIndex)
                    continue;

                const Util::Guid targetRecord = targetEditable.prefabEntity.IsValid()
                    ? targetEditable.prefabEntity : targetEditable.guid;
                if (!targetRecord.IsValid())
                    continue;

                DocumentEntityRef ref;
                ref.entity = docIndex;
                ref.component = component;
                ref.field = interface->GetFieldNames()[field];
                ref.target = targetRecord;
                doc->entityRefs.Append(ref);
            }
        }

        doc->entities[docIndex].components.Append(recordComponent);
    }

    Util::HashTable<Util::Guid, Game::Entity> map;
    this->BuildFileMap(instanceIndex, doc->source, map);
    this->SetRefContext(map);
    this->CaptureExtensions(editable.entity, doc->entities[docIndex].extensions, map);

    // The text is the one this file writes, so the references it names have to be recorded against this
    // file's records rather than the ones the text the document held was read from
    for (IndexT e = 0; e < doc->entities[docIndex].extensions.Size(); e++)
    {
        Scene::RecordDocumentRefs(doc, docIndex, doc->entities[docIndex].extensions[e].name,
            doc->entities[docIndex].extensions[e].json);
    }

    // The entity is the record now, so there is nothing left for it to be an override against. The
    // extensions are captured before this, because they read the marks the entity had.
    Util::Array<PrefabOverride> none;
    this->SetEntityPrefabLink(editable.entity, instanceIndex, record.guid, none);

    return true;
}

//------------------------------------------------------------------------------
/**
    Create the entity for a record the instance gained. The record's own overrides are its mark, and
    the chain under it is walked with the instance, so a reference in a chain file answers with an
    entity of this instance.
*/
void
Scene::CreateInstanceEntity(IndexT instanceIndex, const DocumentEntity& record, const Ptr<SceneDocument>& doc,
    IndexT recordIndex, const Util::Guid& guid, Game::Entity entity)
{
    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();
    const PrefabInstance& instance = this->prefabInstances[instanceIndex];

    Editable editable;
    editable.guid = guid;
    editable.name = record.name;

    // The instance's collections are the file's collection nodes in file order, so the record's
    // collection is the one at the same position.
    IndexT collectionIndex = InvalidIndex;
    for (IndexT c = 0; c < doc->collections.Size(); c++)
    {
        if (doc->collections[c].guid == record.collection)
        {
            if (c < instance.collections.Size())
                collectionIndex = c;
            break;
        }
    }
    editable.collection = collectionIndex != InvalidIndex ? instance.collections[collectionIndex] :
        this->activeCollection;

    editable.entity = entity;
    editable.prefabInstance = instanceIndex;
    editable.prefabEntity = record.guid;

    for (IndexT c = 0; c < record.components.Size(); c++)
    {
        const DocumentComponent& recordComponent = record.components[c];
        if (recordComponent.component == ownerId || recordComponent.component == Game::ComponentId::Invalid())
            continue;

        PrefabOverride override;
        override.component = recordComponent.component;
        override.overriddenFields = recordComponent.presentFields;
        override.componentAdded = recordComponent.addedToPrefab;
        override.componentRemoved = recordComponent.removedFromPrefab;
        editable.prefabOverrides.Append(override);
    }

    for (IndexT e = 0; e < record.extensions.Size(); e++)
    {
        const DocumentExtension& entry = record.extensions[e];
        if (entry.fields.IsEmpty())
            continue;

        PrefabOverride override;
        override.extension = entry.name;
        override.fields = entry.fields;
        editable.prefabOverrides.Append(override);
    }

    this->AllocateInstance(editable);

    // The entity may already be in the instance's set, which is what an undo of a revert leaves:
    // only
    // the instance in the world went.
    this->AppendInstanceEntity(instanceIndex, editable.entity);

    Util::HashTable<Util::Guid, Game::Entity> fileMap;
    this->BuildFileMap(instanceIndex, doc->source, fileMap);

    Util::Array<ChainHop> hops;
    this->WalkChain(instance.source, record.guid, hops, instanceIndex);

    this->MaterializeRecord(record, recordIndex, doc, editable.entity, hops, fileMap);
}

//------------------------------------------------------------------------------
/**
    What the chain gives a component, which is the value under whatever this scene wrote over it.
    This is what a write is marked against, what a revert puts back, and what the merge follows.
*/
bool
Scene::GetPrefabBaseValue(Game::Entity entity, Game::ComponentId component, Util::Blob& outValue) const
{
    if (entity.index >= this->editables.Size())
        return false;

    const Editable& editable = this->editables[entity.index];
    if (editable.prefabInstance == InvalidIndex || editable.prefabInstance >= this->prefabInstances.Size() ||
        !this->prefabInstances[editable.prefabInstance].valid)
    {
        return false;
    }

    Util::Array<ChainHop> hops;
    if (!this->WalkChain(this->prefabInstances[editable.prefabInstance].source, editable.prefabEntity, hops,
        editable.prefabInstance))
        return false;

    Util::Array<Game::ComponentId> chainCids;
    Util::Array<Util::Blob> values;
    Util::Array<bool> present;
    // An empty record on top: the base is what the chain's files say, before anything this scene
    // wrote
    // over it.
    const DocumentEntity noSceneRecord;
    this->MergeChainComponents(noSceneRecord, hops, chainCids, values, present);

    for (IndexT i = 0; i < chainCids.Size(); i++)
    {
        if (chainCids[i] == component)
        {
            if (!present[i])
                return false;
            outValue = values[i];
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
const PrefabOverride&
Scene::FindPrefabOverride(Game::Entity entity, Game::ComponentId component) const
{
    static const PrefabOverride none;

    if (entity.index >= this->editables.Size())
        return none;

    const Editable& editable = this->editables[entity.index];
    for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
    {
        if (editable.prefabOverrides[i].component == component)
            return editable.prefabOverrides[i];
    }
    return none;
}

//------------------------------------------------------------------------------
/**
    The entry for one extension's data. An entry with an invalid component is one of these, which is
    what lets one array cover both a component and an extension.
*/
const PrefabOverride&
Scene::FindExtensionOverride(Game::Entity entity, Util::StringAtom extension) const
{
    static const PrefabOverride none;

    if (entity.index >= this->editables.Size())
        return none;

    const Editable& editable = this->editables[entity.index];
    for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
    {
        if (editable.prefabOverrides[i].extension == extension)
            return editable.prefabOverrides[i];
    }
    return none;
}

//------------------------------------------------------------------------------
/**
    Set the keys an extension wrote on an entity. The mark is what the next save writes, so an undo
    that does not put it back leaves the scene and the file disagreeing.
*/
void
Scene::SetExtensionOverride(Game::Entity entity, Util::StringAtom extension, const Util::Array<Util::String>& fields)
{
    if (entity.index >= this->editables.Size())
        return;

    Editable& editable = this->editables[entity.index];

    for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
    {
        PrefabOverride& override = editable.prefabOverrides[i];
        if (override.extension != extension)
            continue;

        override.fields = fields;
        this->version++;
        return;
    }

    if (fields.IsEmpty())
        return;

    PrefabOverride override;
    override.extension = extension;
    override.fields = fields;
    editable.prefabOverrides.Append(override);
    this->version++;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<PrefabInstance>&
Scene::GetPrefabInstances() const
{
    return this->prefabInstances;
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::IsPrefabInstanceEntity(Game::Entity entity) const
{
    return this->GetPrefabInstanceIndex(entity) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    The root is the entity the level placed. It is the only entity of an instantiation whose parent
    the level decides; the others are where the file puts them.
*/
bool
Scene::IsPrefabInstanceRoot(Game::Entity entity) const
{
    const IndexT instanceIndex = this->GetPrefabInstanceIndex(entity);
    if (instanceIndex == InvalidIndex)
        return false;

    const Game::Entity parent = this->GetParent(entity);
    if (parent == Game::Entity::Invalid())
        return true;

    return parent.index >= this->editables.Size() || this->editables[parent.index].prefabInstance != instanceIndex;
}

//------------------------------------------------------------------------------
/**
    The instance, through the guard that reads the link, so a released instance answers with the
    empty entry.
*/
const PrefabInstance&
Scene::GetPrefabInstance(Game::Entity entity) const
{
    const IndexT instanceIndex = this->GetPrefabInstanceIndex(entity);
    if (instanceIndex != InvalidIndex)
        return this->prefabInstances[instanceIndex];

    static const PrefabInstance none;
    return none;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::GetInstanceEntities(Game::Entity entity, Util::Array<Game::Entity>& out) const
{
    out.Clear();
    const PrefabInstance& instance = this->GetPrefabInstance(entity);
    if (!instance.valid)
        return;

    for (IndexT i = 0; i < instance.entities.Size(); i++)
    {
        const Game::Entity instanceEntity = instance.entities[i];
        if (instanceEntity.index < this->editables.Size() && this->HasInstance(instanceEntity))
            out.Append(instanceEntity);
    }
}

//------------------------------------------------------------------------------
/**
    One entity per record of the file, in file order. The records of a prefab the prefab itself
    instances are in the file too, which is what lets one instantiation describe a whole subtree.
    Every entity is created before any is filled, because a value may point at an entity that comes
    later in the file.
*/
bool
Scene::InstantiatePrefab(const Util::String& path, const Util::Guid& collection, Util::Array<Game::Entity>& created)
{
    if (this->authoringWorld == nullptr)
        return false;

    if (!this->filePath.IsEmpty() && this->filePath == path)
    {
        n_warning("Cannot instantiate '%s' into itself\n", path.AsCharPtr());
        return false;
    }

    // Read through the copies the scene keeps, because the chain walks read this file from the
    // cache
    // later, and the guids the records get have to be the ones those walks find.
    Ptr<SceneDocument> doc = this->LoadDocumentCached(path);
    if (!doc.isvalid())
        return false;

    // the same rule as on a load: a file whose chains do not resolve is broken, and instantiating
    // it
    // would put half entities in the scene.
    if (!this->ValidatePrefabLinks(doc))
        return false;

    // One guid per record before anything is made. Normalizing after the entities were created
    // would
    // leave a duplicate's entity pointing at the record the first entity has.
    this->NormalizeDocumentGuids(doc);

    PrefabInstance instance;
    instance.valid = true;
    instance.guid.Generate();
    instance.source = path;
    this->prefabInstances.Append(instance);
    const IndexT instanceIndex = this->prefabInstances.Size() - 1;

    const Util::Guid targetCollection = collection.IsValid() ? collection : this->activeCollection;

    // The file's collection nodes become collections of this scene with new guids. The file's root
    // collections hang under the collection the instantiation goes into, and the file order puts a
    // parent before its children.
    Util::HashTable<Util::Guid, Util::Guid> collectionMap;
    for (IndexT i = 0; i < doc->collections.Size(); i++)
    {
        const DocumentCollection& source = doc->collections[i];

        Collection record;
        record.guid.Generate();
        record.name = source.name;
        record.parent = targetCollection;
        const IndexT parentIndex = collectionMap.FindIndex(source.parent);
        if (parentIndex != InvalidIndex)
            record.parent = collectionMap.ValueAtIndex(source.parent, parentIndex);
        record.order = (IndexT)this->collections.Size();

        this->CreateCollection(record, InvalidIndex);
        collectionMap.Add(source.guid, record.guid);
        this->prefabInstances[instanceIndex].collections.Append(record.guid);
    }

    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        Editable record;
        record.guid.Generate();
        record.name = this->UniqueEntityName(doc->entities[i].name);
        record.collection = targetCollection;
        const IndexT collectionIndex = collectionMap.FindIndex(doc->entities[i].collection);
        if (collectionIndex != InvalidIndex)
            record.collection = collectionMap.ValueAtIndex(doc->entities[i].collection, collectionIndex);
        record.entity = this->AllocateEntityId();
        record.prefabInstance = instanceIndex;
        record.prefabEntity = doc->entities[i].guid;
        this->AllocateInstance(record);
        this->AppendInstanceEntity(instanceIndex, record.entity);
    }

    Util::HashTable<Util::Guid, Game::Entity> fileMap;
    for (IndexT i = 0; i < doc->entities.Size(); i++)
        fileMap.Add(doc->entities[i].guid, this->prefabInstances[instanceIndex].entities[i]);

    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        const DocumentEntity& record = doc->entities[i];
        Game::Entity entity = this->prefabInstances[instanceIndex].entities[i];

        Util::Array<ChainHop> hops;
        // An added record is the instance's own, so there is no chain under it to resolve
        if (!record.addedToPrefab && !record.prefabSource.IsEmpty() &&
            !this->WalkChain(record.prefabSource, record.prefabEntity, hops))
        {
            n_warning("The prefab chain of '%s' from '%s' does not resolve, the entity keeps what the file has\n",
                record.name.AsCharPtr(), path.AsCharPtr());
        }

        this->MaterializeRecord(record, i, doc, entity, hops, fileMap);
        created.Append(entity);
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::PasteDocument(const Ptr<SceneDocument>& doc, const Util::Guid& collection, Game::Entity parent,
    Util::Array<Game::Entity>& created)
{
    if (this->authoringWorld == nullptr || !doc.isvalid() || doc->entities.IsEmpty())
        return false;

    // A paste is asked for one collection and not for a tree, so the collections the document names
    // are not made.
    const Util::Guid targetCollection = collection.IsValid() ? collection : this->activeCollection;

    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();
    const Game::ComponentId transform = Game::GetComponentId<Game::HTransform>();

    // The link. A copy that keeps it pastes entities that still follow their prefab. One new
    // instance
    // per instance the copy came from, so one group shares it. A group whose file cannot be read,
    // or
    // whose record the file no longer has, falls back to a plain copy, which is what a paste into a
    // scene without the file has to work from.
    Util::HashTable<Util::Guid, IndexT> instanceByGuid;
    Util::Array<IndexT> instanceByDocEntity;
    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        const DocumentEntity& record = doc->entities[i];

        IndexT instanceIndex = InvalidIndex;
        if (!record.prefabSource.IsEmpty() && record.instance.IsValid())
        {
            const IndexT known = instanceByGuid.FindIndex(record.instance);
            if (known != InvalidIndex)
            {
                instanceIndex = instanceByGuid.ValueAtIndex(record.instance, known);
            }
            else
            {
                // An entity the instance added has no record, so for it the only check is that the
                // file can be read. For the others the chain has to resolve.
                bool resolves = record.addedToPrefab && this->LoadDocumentCached(record.prefabSource).isvalid();
                if (!resolves)
                {
                    Util::Array<ChainHop> hops;
                    resolves = this->WalkChain(record.prefabSource, record.prefabEntity, hops);
                }

                if (resolves)
                {
                    PrefabInstance instance;
                    instance.valid = true;
                    instance.guid.Generate();
                    instance.source = record.prefabSource;
                    this->prefabInstances.Append(instance);
                    instanceIndex = this->prefabInstances.Size() - 1;
                    instanceByGuid.Add(record.instance, instanceIndex);
                }
            }
        }

        instanceByDocEntity.Append(instanceIndex);
    }

    Util::HashTable<Util::Guid, Game::Entity> entityMap;
    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        const DocumentEntity& record = doc->entities[i];
        const IndexT instanceIndex = instanceByDocEntity[i];

        if (record.removedFromPrefab)
        {
            // The entity is not in the scene, so the paste carries the mark instead of an entity.
            // It
            // names the same record, because the paste instantiates the same file.
            if (instanceIndex != InvalidIndex)
            {
                RemovedRecord marker;
                marker.record = record.guid;
                marker.name = record.name;
                marker.collection = targetCollection;
                this->prefabInstances[instanceIndex].removedRecords.Append(marker);
            }
            continue;
        }

        Editable editable;
        editable.guid.Generate();
        editable.name = this->UniqueEntityName(record.name);
        editable.collection = targetCollection;
        editable.entity = this->AllocateEntityId();
        editable.prefabInstance = instanceIndex;
        editable.prefabEntity = record.prefabEntity;

        if (instanceIndex != InvalidIndex)
        {
            // The field names the document carries are the fields the instance wrote, the same rule
            // the file writes by, so the mark is read off the document. Without it the pasted
            // entity
            // says nothing about which fields it wrote, and the next prefab update takes them back.
            for (IndexT c = 0; c < record.components.Size(); c++)
            {
                const DocumentComponent& component = record.components[c];
                if (component.component == ownerId || component.component == Game::ComponentId::Invalid())
                    continue;

                PrefabOverride override;
                override.component = component.component;
                override.overriddenFields = component.presentFields;
                override.componentAdded = component.addedToPrefab;
                override.componentRemoved = component.removedFromPrefab;
                editable.prefabOverrides.Append(override);
            }

            for (IndexT e = 0; e < record.extensions.Size(); e++)
            {
                const DocumentExtension& entry = record.extensions[e];
                if (entry.fields.IsEmpty())
                    continue;

                PrefabOverride override;
                override.extension = entry.name;
                override.fields = entry.fields;
                editable.prefabOverrides.Append(override);
            }
        }

        this->AllocateInstance(editable);
        created.Append(editable.entity);
        entityMap.Add(record.guid, editable.entity);

        if (instanceIndex != InvalidIndex)
            this->AppendInstanceEntity(instanceIndex, editable.entity);
    }

    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        const DocumentEntity& record = doc->entities[i];
        const IndexT entityIndex = entityMap.FindIndex(record.guid);
        if (entityIndex == InvalidIndex)
            continue;
        const Game::Entity entity = entityMap.ValueAtIndex(record.guid, entityIndex);
        const IndexT instanceIndex = instanceByDocEntity[i];

        // The chain the link walks, walked with the instance, so the hops carry the entities the
        // paste
        // made and a reference in a chain file can answer with one of them.
        Util::Array<ChainHop> hops;
        if (instanceIndex != InvalidIndex)
            this->WalkChain(this->prefabInstances[instanceIndex].source, record.prefabEntity, hops, instanceIndex);

        if (instanceIndex != InvalidIndex)
        {
            // The chain's values with the document's overrides over them. The document only carries
            // the fields the instance wrote, so the rest of the component comes from the prefab.
            Util::Array<Game::ComponentId> chainCids;
            Util::Array<Util::Blob> values;
            Util::Array<bool> present;
            this->MergeChainComponents(record, hops, chainCids, values, present);

            for (IndexT c = 0; c < chainCids.Size(); c++)
            {
                const Game::ComponentId component = chainCids[c];
                if (component == ownerId || !present[c])
                    continue;

                const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
                const void* value = (typeSize > 0 && values[c].IsValid()) ? values[c].GetPtr() : nullptr;

                if (this->authoringWorld->HasComponent(entity, component))
                {
                    if (value != nullptr)
                        this->authoringWorld->SetComponentValue(entity, component, (void*)value, typeSize);
                }
                else
                    this->authoringWorld->AddComponent(entity, component, value);
            }
        }
        else
        {
            for (IndexT c = 0; c < record.components.Size(); c++)
            {
                const DocumentComponent& component = record.components[c];
                if (component.component == Game::ComponentId::Invalid() || component.component == ownerId)
                    continue;

                const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component.component);
                const void* value = (typeSize > 0 && component.value.IsValid()) ? component.value.GetPtr() : nullptr;

                // The authoring world gives each entity its own set of components, so a component
                // the
                // entity already has takes the chain's value rather than going on a second time,
                // which
                // would put the same column on a table twice.
                if (this->authoringWorld->HasComponent(entity, component.component))
                {
                    if (value != nullptr)
                        this->authoringWorld->SetComponentValue(entity, component.component, (void*)value, typeSize);
                }
                else
                    this->authoringWorld->AddComponent(entity, component.component, value);
            }
        }
        this->authoringWorld->ExecuteAddComponentCommands();

        // A chain file's references name records of that file, and only the records the paste
        // contains
        // have entities to answer with.
        for (IndexT h = 0; h < hops.Size(); h++)
        {
            if (hops[h].fileMap.IsEmpty())
                continue;

            this->PatchRecordEntityRefs(hops[h].docEntity, entity, hops[h].doc, hops[h].fileMap);
        }

        // The references were read as guids and left the fields empty, so the entities the paste
        // made
        // go into them. A reference to something the document does not contain cannot be named, and
        // only the hierarchy keeps it: the paste's roots go under the entity the paste was asked
        // for.
        for (IndexT r = 0; r < doc->entityRefs.Size(); r++)
        {
            const DocumentEntityRef& ref = doc->entityRefs[r];
            if (ref.entity != i)
                continue;

            // A reference inside extension text is put back by the extension that wrote it, against the
            // same map, because the scene cannot write into a text it cannot read
            if (ref.extension.IsValid())
                continue;

            Game::ComponentInterface* interface =
                static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(ref.component));
            if (interface == nullptr)
                continue;

            const IndexT field = FindFieldIndex(interface, ref.field);
            if (field == InvalidIndex)
                continue;

            Game::Entity target = Game::Entity::Invalid();
            const IndexT mapIndex = entityMap.FindIndex(ref.target);
            if (mapIndex != InvalidIndex)
                target = entityMap.ValueAtIndex(ref.target, mapIndex);
            else if (ref.component == transform && ref.field == "parent" && parent != Game::Entity::Invalid())
                target = parent;

            byte* data = (byte*)this->GetComponentData(entity, ref.component);
            if (data == nullptr)
                continue;

            Memory::Copy(&target, data + interface->GetFieldByteOffsets()[field], sizeof(Game::Entity));
        }

        // The values were written to the authoring world, so the mirror has to be written again
        // after
        // the references.
        this->MirrorEntity(entity);

        // The extension data is applied after the components, because an extension that creates
        // something on the entity reads the entity the paste just made. A kept link carries only
        // the
        // keys the instance wrote, so the text is composed through the chain first.
        if (instanceIndex != InvalidIndex)
        {
            Util::Dictionary<Util::StringAtom, Util::String> texts;
            this->MergeChainExtensions(record, hops, texts, entityMap);

            Util::Array<DocumentExtension> entries;
            for (IndexT t = 0; t < texts.Size(); t++)
            {
                DocumentExtension entry;
                entry.name = texts.KeyAtIndex(t);
                entry.json = texts.ValueAtIndex(t);
                entries.Append(entry);
            }

            this->ApplyExtensions(record.name, entries, entity, entityMap);
        }
        else
        {
            this->ApplyExtensions(record.name, record.extensions, entity, entityMap);
        }
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
Scene::GetFilePath() const
{
    return this->filePath;
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::IsDirty() const
{
    return this->version != this->savedVersion;
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::Save(const Util::String& path)
{
    if (this->authoringWorld == nullptr)
        return false;

    Ptr<SceneDocument> doc = SceneDocument::Create();
    doc->source = path;

    Util::Array<Util::Guid> guidByEntityIndex;
    guidByEntityIndex.Resize(this->editables.Size());
    for (IndexT i = 0; i < this->editables.Size(); i++)
        guidByEntityIndex[i] = this->editables[i].guid;

    for (IndexT i = 0; i < this->collections.Size(); i++)
    {
        const Collection& collection = this->collections[i];
        DocumentCollection record;
        record.guid = collection.guid;
        record.name = collection.name;
        record.parent = collection.parent;
        record.order = i;
        doc->collections.Append(record);
    }

    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();

    // The guids this file writes are the scene's records, so a text that names entities names them
    // by
    // these. Built once, because the same map is used for every entity.
    Util::HashTable<Util::Guid, Game::Entity> sceneMap;
    this->BuildSceneMap(sceneMap);

    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        const Editable& editable = this->editables[i];
        if (!editable.guid.IsValid() || !this->HasInstance(editable.entity))
            continue;

        DocumentEntity record;
        record.guid = editable.guid;
        record.name = editable.name;
        record.collection = editable.collection;

        const bool linked = this->IsPrefabInstanceEntity(editable.entity);
        if (linked)
        {
            const PrefabInstance& instance = this->prefabInstances[editable.prefabInstance];
            record.prefabSource = instance.source;
            record.prefabEntity = editable.prefabEntity;
            record.instance = instance.guid;

            // An instance entity with no record is the instance's own addition, so the node says
            // that
            // instead of naming one.
            record.addedToPrefab = !editable.prefabEntity.IsValid();
        }

        const Util::Array<Game::ComponentId>& components = this->GetComponents(editable.entity);
        for (IndexT c = 0; c < components.Size(); c++)
        {
            const Game::ComponentId component = components[c];
            if (component == ownerId)
                continue;

            DocumentComponent recordComponent;
            recordComponent.component = component;

            const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
            auto writeValue = [&]()
            {
                if (typeSize > 0)
                {
                    recordComponent.value = Util::Blob(this->GetComponentData(editable.entity, component), typeSize);
                    for (IndexT field = 0; field < MAX_COMPONENT_FIELDS; field++)
                        recordComponent.presentFields.SetBit((uint64_t)field);
                }
            };

            if (!linked)
            {
                writeValue();
            }
            else if (!editable.prefabEntity.IsValid())
            {
                // The entity has no record, so there is no chain under it and everything it holds
                // is
                // its own.
                writeValue();
            }
            else
            {
                const PrefabOverride& override = this->FindPrefabOverride(editable.entity, component);
                if (override.componentAdded)
                {
                    recordComponent.addedToPrefab = true;
                    writeValue();
                }
                else
                {
                    // Nothing written and nothing removed is nothing to say about the component:
                    // the
                    // chain already gives what the entity holds.
                    if (override.overriddenFields.IsNull())
                        continue;

                    if (typeSize > 0)
                    {
                        recordComponent.value =
                            Util::Blob(this->GetComponentData(editable.entity, component), typeSize);
                        recordComponent.presentFields = override.overriddenFields;
                    }
                }
            }

            record.components.Append(recordComponent);
        }

        if (linked)
        {
            for (IndexT o = 0; o < editable.prefabOverrides.Size(); o++)
            {
                const PrefabOverride& override = editable.prefabOverrides[o];
                if (!override.componentRemoved)
                    continue;

                DocumentComponent recordComponent;
                recordComponent.component = override.component;
                recordComponent.removedFromPrefab = true;
                record.components.Append(recordComponent);
            }
        }

        // The feature data goes in as the text each extension writes. For an entity that is its own
        // that is its whole state; for a linked entity it is only the keys the instance wrote, the
        // same rule the components follow.
        if (!linked)
        {
            this->CaptureExtensions(editable.entity, record.extensions, sceneMap);
        }
        else
        {
            this->SetRefContext(sceneMap);

            const Util::Array<Game::LevelExtension>& extensions = Game::LevelExtensionRegistry::GetAllExtensions();
            for (IndexT e = 0; e < extensions.Size(); e++)
            {
                const PrefabOverride& override = this->FindExtensionOverride(editable.entity, extensions[e].name);
                if (override.extension != extensions[e].name || override.fields.IsEmpty())
                    continue;

                Util::String json;
                this->CaptureExtensionKeys(editable.entity, extensions[e].name, override.fields, json);
                if (json.IsEmpty())
                    continue;

                DocumentExtension entry;
                entry.name = extensions[e].name;
                entry.json = json;
                entry.fields = override.fields;
                record.extensions.Append(entry);
            }
        }

        const IndexT docIndex = doc->entities.Size();
        for (IndexT e = 0; e < record.extensions.Size(); e++)
            Scene::RecordDocumentRefs(doc, docIndex, record.extensions[e].name, record.extensions[e].json);

        doc->entities.Append(record);
    }

    // The entities the instance does not have. They are written after the entities, because a mark
    // names a record of another file rather than an entity, and there is no entity for the guid map
    // to
    // resolve.
    for (IndexT i = 0; i < this->prefabInstances.Size(); i++)
    {
        const PrefabInstance& instance = this->prefabInstances[i];
        if (!instance.valid)
            continue;

        for (IndexT r = 0; r < instance.removedRecords.Size(); r++)
        {
            const RemovedRecord& marker = instance.removedRecords[r];

            DocumentEntity record;
            record.guid = marker.record;
            record.name = marker.name;
            record.collection = marker.collection;
            record.removedFromPrefab = true;
            record.instance = instance.guid;
            record.prefabSource = instance.source;
            record.prefabEntity = marker.record;

            doc->entities.Append(record);
        }
    }

    if (!doc->ToFile(path, guidByEntityIndex))
        return false;

    this->filePath = path;
    this->savedVersion = this->version;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::Load(const Util::String& path)
{
    if (this->authoringWorld == nullptr)
        return false;

    Ptr<SceneDocument> doc = SceneDocument::FromFile(path);
    if (!doc.isvalid())
        return false;

    // A file that names a prefab it cannot read is broken, and opening it would fill the scene with
    // entities that have no chain. Nothing is taken before every chain answers, so a failed open
    // touches nothing.
    if (!this->ValidatePrefabLinks(doc))
        return false;

    // The guids the file gave are the guids the scene keeps, so two records with the same one have
    // to
    // be told apart before the entities are made.
    this->NormalizeDocumentGuids(doc);

    // Everything the scene holds goes first, the full way, so the loaded entities are allocated
    // fresh
    // and the undo stacks hold nothing that points at an old one.
    this->prefabInstances.Clear();
    this->documentCache.Clear();

    Util::Array<Game::Entity> authored;
    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        if (this->editables[i].guid.IsValid())
            authored.Append(this->editables[i].entity);
    }
    for (IndexT i = 0; i < authored.Size(); i++)
        this->DeleteEntity(authored[i]);

    this->collections.Clear();
    for (IndexT i = 0; i < doc->collections.Size(); i++)
    {
        const DocumentCollection& record = doc->collections[i];
        Collection collection;
        collection.guid = record.guid;
        collection.name = record.name;
        collection.parent = record.parent;
        collection.order = record.order;
        this->CreateCollection(collection, InvalidIndex);
    }

    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();

    // Every record gets its entity before any is filled, because a value may point at an entity
    // that
    // comes later.
    Util::HashTable<Util::Guid, Game::Entity> fileMap;
    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        const DocumentEntity& record = doc->entities[i];

        IndexT instanceIndex = InvalidIndex;
        if (!record.prefabSource.IsEmpty())
        {
            for (IndexT p = 0; p < this->prefabInstances.Size(); p++)
            {
                if (this->prefabInstances[p].valid && this->prefabInstances[p].guid == record.instance &&
                    this->prefabInstances[p].source == record.prefabSource)
                {
                    instanceIndex = p;
                    break;
                }
            }
            if (instanceIndex == InvalidIndex)
            {
                PrefabInstance instance;
                instance.valid = true;
                instance.guid = record.instance;
                instance.source = record.prefabSource;
                this->prefabInstances.Append(instance);
                instanceIndex = this->prefabInstances.Size() - 1;
            }
        }

        if (record.removedFromPrefab)
        {
            // The entity is not in the scene, so there is no record for it. The mark is the
            // instance's, and the node is what carried it here.
            if (instanceIndex != InvalidIndex)
            {
                RemovedRecord marker;
                marker.record = record.guid;
                marker.name = record.name;
                marker.collection = record.collection;
                this->prefabInstances[instanceIndex].removedRecords.Append(marker);
            }
            continue;
        }

        Editable editable;
        editable.guid = record.guid;
        editable.name = record.name;
        editable.collection = record.collection;
        editable.entity = this->AllocateEntityId();
        editable.prefabInstance = instanceIndex;
        editable.prefabEntity = record.prefabEntity;

        if (instanceIndex != InvalidIndex)
        {
            for (IndexT c = 0; c < record.components.Size(); c++)
            {
                const DocumentComponent& recordComponent = record.components[c];
                if (recordComponent.component == ownerId || recordComponent.component == Game::ComponentId::Invalid())
                    continue;

                PrefabOverride override;
                override.component = recordComponent.component;
                override.overriddenFields = recordComponent.presentFields;
                override.componentAdded = recordComponent.addedToPrefab;
                override.componentRemoved = recordComponent.removedFromPrefab;
                editable.prefabOverrides.Append(override);
            }

            // The keys the file carried for an extension are the keys that entity wrote, the same
            // rule
            // as the field names on a component node. The scene cannot read the text, so the
            // extension
            // named them at the read.
            for (IndexT e = 0; e < record.extensions.Size(); e++)
            {
                const DocumentExtension& entry = record.extensions[e];
                if (entry.fields.IsEmpty())
                    continue;

                PrefabOverride override;
                override.extension = entry.name;
                override.fields = entry.fields;
                editable.prefabOverrides.Append(override);
            }
        }

        this->AllocateInstance(editable);

        if (instanceIndex != InvalidIndex)
            this->AppendInstanceEntity(instanceIndex, editable.entity);

        fileMap.Add(record.guid, editable.entity);
    }

    for (IndexT i = 0; i < doc->entities.Size(); i++)
    {
        const DocumentEntity& record = doc->entities[i];
        const IndexT mapIndex = fileMap.FindIndex(record.guid);
        if (mapIndex == InvalidIndex)
            continue;
        const Game::Entity entity = fileMap.ValueAtIndex(record.guid, mapIndex);

        Util::Array<ChainHop> hops;
        // An added record is the instance's own, so there is no chain under it to resolve
        if (!record.addedToPrefab && !record.prefabSource.IsEmpty() &&
            !this->WalkChain(record.prefabSource, record.prefabEntity, hops))
        {
            n_warning("The prefab chain of '%s' from '%s' does not resolve, the entity keeps what the file has\n",
                record.name.AsCharPtr(), path.AsCharPtr());
        }

        this->MaterializeRecord(record, i, doc, entity, hops, fileMap);
    }

    this->filePath = path;
    this->savedVersion = this->version;
    return true;
}

//------------------------------------------------------------------------------
/**
    Put the component back to what the chain gives, which also puts back a component the instance
    removed and takes away one it added.
*/
bool
Scene::RevertComponent(Game::Entity entity, Game::ComponentId component)
{
    if (!this->IsPrefabInstanceEntity(entity))
        return false;

    Editable& editable = this->editables[entity.index];

    IndexT stateIndex = InvalidIndex;
    for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
    {
        if (editable.prefabOverrides[i].component == component)
        {
            stateIndex = i;
            break;
        }
    }

    const bool hasComponent = this->authoringWorld->HasComponent(entity, component);

    Util::Blob base;
    const bool inChain = this->GetPrefabBaseValue(entity, component, base);

    if (!inChain)
    {
        // The chain does not have the component, so the instance added it. Reverting it is taking
        // it
        // away.
        if (hasComponent)
            this->RemoveComponent(entity, component);
        if (stateIndex != InvalidIndex)
            editable.prefabOverrides.EraseIndex(stateIndex);
        this->version++;
        return true;
    }

    if (!hasComponent)
    {
        const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
        if (typeSize > 0)
            this->AddComponent(entity, component, base.GetPtr());
        else
            this->AddComponent(entity, component, nullptr);
    }
    else
    {
        const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
        if (typeSize > 0)
            this->authoringWorld->SetComponentValue(entity, component, base.GetPtr(), typeSize);
    }

    if (stateIndex != InvalidIndex)
        editable.prefabOverrides.EraseIndex(stateIndex);

    this->MirrorComponent(entity, component);
    this->version++;
    return true;
}

//------------------------------------------------------------------------------
/**
    Unlinking does not change any value: the entities already hold what the chain merged. What goes
    is the link, the overrides and the instance, which makes every entity an ordinary authored one.
*/
void
Scene::UnpackPrefab(Game::Entity entity)
{
    if (!this->IsPrefabInstanceEntity(entity))
        return;

    const IndexT instanceIndex = this->editables[entity.index].prefabInstance;
    PrefabInstance& instance = this->prefabInstances[instanceIndex];

    for (IndexT i = 0; i < instance.entities.Size(); i++)
    {
        const Game::Entity instanceEntity = instance.entities[i];
        if (instanceEntity.index >= this->editables.Size())
            continue;

        Editable& editable = this->editables[instanceEntity.index];
        editable.prefabInstance = InvalidIndex;
        editable.prefabEntity = Util::Guid();
        editable.prefabOverrides.Clear();
    }

    instance.valid = false;
    this->version++;
}

//------------------------------------------------------------------------------
/**
    A saved prefab file reaches its instances. Every scene drops its copy and walks what it has; for
    each entity whose chain reads the file, the fields the instance did not write take the new
    value, a component the chain added is added, and what the instance wrote stays. A chain that no
    longer resolves is left alone.
*/
void
Scene::UpdatePrefabInstances(const Util::String& path)
{
    this->InvalidateDocument(path);

    if (this->authoringWorld == nullptr)
        return;

    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();

    Util::HashTable<Util::Guid, Game::Entity> sceneMap;
    this->BuildSceneMap(sceneMap);

    // An entity whose record the file no longer has is not a copy of anything, so it is the
    // instance's
    // own addition.
    Ptr<SceneDocument> doc = this->LoadDocumentCached(path);
    if (doc.isvalid())
    {
        for (IndexT i = 0; i < this->editables.Size(); i++)
        {
            Editable& editable = this->editables[i];
            const IndexT instanceIndex = editable.prefabInstance;
            if (!editable.prefabEntity.IsValid() || instanceIndex == InvalidIndex ||
                instanceIndex >= (IndexT)this->prefabInstances.Size() ||
                !this->prefabInstances[instanceIndex].valid ||
                this->prefabInstances[instanceIndex].source != path)
                continue;

            if (doc->FindEntity(editable.prefabEntity) != InvalidIndex)
                continue;

            editable.prefabEntity = Util::Guid();
            this->version++;
        }
    }

    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        const Editable& editable = this->editables[i];
        if (!editable.guid.IsValid() || editable.prefabInstance == InvalidIndex ||
            editable.prefabInstance >= this->prefabInstances.Size() ||
            !this->prefabInstances[editable.prefabInstance].valid || !this->HasInstance(editable.entity))
        {
            continue;
        }

        const Game::Entity entity = editable.entity;
        Util::Array<ChainHop> hops;
        if (!this->WalkChain(this->prefabInstances[editable.prefabInstance].source, editable.prefabEntity, hops,
        editable.prefabInstance))
            continue;

        bool touches = false;
        for (IndexT h = 0; h < hops.Size(); h++)
        {
            if (hops[h].doc->source == path)
            {
                touches = true;
                break;
            }
        }
        if (!touches)
            continue;

        // The chain as the files give it now
        Util::Array<Game::ComponentId> chainCids;
        Util::Array<Util::Blob> values;
        Util::Array<bool> present;
        const DocumentEntity noSceneRecord;
        this->MergeChainComponents(noSceneRecord, hops, chainCids, values, present);

        for (IndexT c = 0; c < chainCids.Size(); c++)
        {
            const Game::ComponentId component = chainCids[c];
            if (component == ownerId || !present[c])
                continue;

            const PrefabOverride overrideState = this->FindPrefabOverride(entity, component);

            if (!this->authoringWorld->HasComponent(entity, component))
            {
                if (overrideState.componentRemoved)
                    continue;

                // A component the prefab gained. The instance takes it as the chain gives it
                const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
                const void* value = (typeSize > 0 && values[c].IsValid()) ? values[c].GetPtr() : nullptr;
                this->AddComponent(entity, component, const_cast<void*>(value));
                continue;
            }

            const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
            if (typeSize == 0 || !values[c].IsValid())
                continue;

            Game::ComponentInterface* interface =
                static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
            const byte* base = (const byte*)values[c].GetPtr();

            const byte* current = (const byte*)this->GetComponentData(entity, component);

            // The merged value is built in its own copy, because writing the row back through its
            // own
            // pointer is not a copy the memory layer takes.
            Util::Blob merged(current, typeSize);
            byte* dst = (byte*)merged.GetPtr();

            bool changed = false;
            for (IndexT field = 0; field < (IndexT)interface->GetNumFields(); field++)
            {
                if (overrideState.overriddenFields.IsSet((uint64_t)field))
                    continue;

                const SizeT offset = (SizeT)interface->GetFieldByteOffsets()[field];
                const SizeT size = this->FieldSpan(interface, component, field);
                if (BytesDiffer(current + offset, base + offset, size))
                {
                    Memory::Copy(base + offset, dst + offset, size);
                    changed = true;
                }
            }

            if (changed)
                this->WriteComponentValue(entity, component, dst);
        }

        // A merged text names guids from every file it was composed from, so the map covers all of
        // them, plus this scene's own records for whatever the entity wrote itself. Built before the
        // merge, because the merge puts each text against it.
        Util::HashTable<Util::Guid, Game::Entity> map = sceneMap;
        for (IndexT h = 0; h < hops.Size(); h++)
            map.Merge(hops[h].fileMap);

        // The extension texts as the chain's files give them now. The scene composes them through
        // the
        // extensions that wrote them, because it cannot read them.
        Util::Dictionary<Util::StringAtom, Util::String> chainTexts;
        this->MergeChainExtensions(noSceneRecord, hops, chainTexts, map);

        this->SetRefContext(map);

        const Util::Array<Game::LevelExtension>& extensions = Game::LevelExtensionRegistry::GetAllExtensions();
        for (IndexT e = 0; e < extensions.Size(); e++)
        {
            const Game::LevelExtension& extension = extensions[e];
            if (extension.merge == nullptr || extension.capture == nullptr || extension.apply == nullptr)
                continue;

            const PrefabOverride& overrideState = this->FindExtensionOverride(entity, extension.name);

            // The instance took the extension off its chain. The merge does not put it back
            if (overrideState.extension == extension.name && overrideState.componentRemoved)
                continue;

            const IndexT textIndex = chainTexts.FindIndex(extension.name);
            const char* base = textIndex == InvalidIndex ? nullptr : chainTexts.ValueAtIndex(textIndex).AsCharPtr();

            Util::String authored;
            if (overrideState.extension == extension.name && !overrideState.fields.IsEmpty())
                this->CaptureExtensionKeys(entity, extension.name, overrideState.fields, authored);

            // Nothing from the chain and nothing authored is nothing to say
            if (base == nullptr && authored.IsEmpty())
                continue;

            Util::String merged;
            extension.merge(base, authored.IsEmpty() ? nullptr : authored.AsCharPtr(), merged);

            // The text the entity produces is what it holds, so the merge only goes back when it
            // answers with something else. The comparison is on the bytes, because the scene cannot
            // read the text.
            Util::String current;
            extension.capture(entity, current);
            if (merged == current)
                continue;

            extension.apply(entity, merged.AsCharPtr());
        }
    }

    // The entity set of an instance is its file's records, so a record the instance has no entity
    // for
    // is a new entity the file gained. Only the instance's own file can gain one, because a record
    // further up the chain is reached through a record of this file. This is what makes a prefab
    // change reach every instance of it.
    for (IndexT i = 0; i < this->prefabInstances.Size(); i++)
    {
        const PrefabInstance& instance = this->prefabInstances[i];
        if (!instance.valid || instance.source != path)
            continue;

        Ptr<SceneDocument> doc = this->LoadDocumentCached(path);
        if (!doc.isvalid())
            continue;

        for (IndexT r = 0; r < doc->entities.Size(); r++)
        {
            const DocumentEntity& record = doc->entities[r];

            // The instance's entities name the file's records, so a record no entity names is one
            // the
            // file gained.
            bool has = false;
            for (IndexT e = 0; e < instance.entities.Size(); e++)
            {
                const Game::Entity instanceEntity = instance.entities[e];
                if (instanceEntity.index >= this->editables.Size())
                    continue;

                if (this->editables[instanceEntity.index].prefabEntity == record.guid)
                {
                    has = true;
                    break;
                }
            }
            if (has)
                continue;

            // The instance said it does not have this record. An update does not put it back,
            // because
            // the mark is the instance's own and only reaches the other instances when the file is
            // written.
            bool marked = false;
            for (IndexT m = 0; m < instance.removedRecords.Size(); m++)
            {
                if (instance.removedRecords[m].record == record.guid)
                {
                    marked = true;
                    break;
                }
            }
            if (marked)
                continue;

            Util::Guid guid;
            guid.Generate();
            this->CreateInstanceEntity(i, record, doc, r, guid, this->AllocateEntityId());
        }
    }
}

//------------------------------------------------------------------------------
/**
    The records an entity's chain walks, in order: the file the instance was made from first, then
    the files it names up to the plain record the chain ends at.
*/
void
Scene::GetPrefabChain(Game::Entity entity, Util::Array<ChainRecord>& out) const
{
    out.Clear();
    if (entity.index >= this->editables.Size())
        return;

    const Editable& editable = this->editables[entity.index];
    if (editable.prefabInstance == InvalidIndex || editable.prefabInstance >= this->prefabInstances.Size() ||
        !this->prefabInstances[editable.prefabInstance].valid)
        return;

    // Names are all the answer needs, so the walk is not asked for the maps a value read would
    // want.
    // The first hop is the file the instance was made from.
    Util::Array<ChainHop> hops;
    if (!this->WalkChain(this->prefabInstances[editable.prefabInstance].source, editable.prefabEntity, hops))
        return;

    for (IndexT h = 0; h < hops.Size(); h++)
    {
        ChainRecord entry;
        entry.source = hops[h].doc->source;
        entry.record = hops[h].doc->entities[hops[h].docEntity].name;
        out.Append(entry);
    }
}

//------------------------------------------------------------------------------
/**
    Which record each field of a component gets its value from. The topmost record that names the
    field gives it. A field no record names is the default at the record the component joins the
    chain at, and a field the instance wrote is an empty entry.
*/
void
Scene::GetFieldSources(Game::Entity entity, Game::ComponentId component, Util::Array<ChainRecord>& out) const
{
    out.Clear();
    if (entity.index >= this->editables.Size())
        return;

    Game::ComponentInterface* interface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    if (interface == nullptr)
        return;

    const Editable& editable = this->editables[entity.index];
    if (editable.prefabInstance == InvalidIndex || editable.prefabInstance >= this->prefabInstances.Size() ||
        !this->prefabInstances[editable.prefabInstance].valid)
        return;

    Util::Array<ChainHop> hops;
    if (!this->WalkChain(this->prefabInstances[editable.prefabInstance].source, editable.prefabEntity, hops))
        return;

    const PrefabOverride& state = this->FindPrefabOverride(entity, component);

    // The record the component joins the chain at. A field no record names is the default there,
    // and a
    // component the instance added has no such record.
    IndexT joinHop = InvalidIndex;
    for (IndexT h = 0; h < hops.Size(); h++)
    {
        const DocumentEntity& rec = hops[h].doc->entities[hops[h].docEntity];
        for (IndexT c = 0; c < rec.components.Size(); c++)
        {
            if (rec.components[c].component == component)
            {
                joinHop = h;
                break;
            }
        }
        if (joinHop != InvalidIndex)
            break;
    }

    out.Resize(interface->GetNumFields());
    for (IndexT field = 0; field < out.Size(); field++)
    {
        if (state.component == component && state.overriddenFields.IsSet((uint64_t)field))
            continue;

        for (IndexT h = 0; h < hops.Size(); h++)
        {
            const DocumentEntity& rec = hops[h].doc->entities[hops[h].docEntity];
            for (IndexT c = 0; c < rec.components.Size(); c++)
            {
                const DocumentComponent& recordComponent = rec.components[c];
                if (recordComponent.component != component || !recordComponent.presentFields.IsSet((uint64_t)field))
                    continue;

                out[field].source = hops[h].doc->source;
                out[field].record = rec.name;
                break;
            }
            if (!out[field].source.IsEmpty())
                break;
        }

        if (joinHop != InvalidIndex && out[field].source.IsEmpty())
        {
            const DocumentEntity& rec = hops[joinHop].doc->entities[hops[joinHop].docEntity];
            out[field].source = hops[joinHop].doc->source;
            out[field].record = rec.name;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
const Editable&
Scene::GetEditable(Game::Entity entity) const
{
    if (entity.index < this->editables.Size())
        return this->editables[entity.index];

    static Editable invalid;
    return invalid;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Editable>&
Scene::GetEditables() const
{
    return this->editables;
}

//------------------------------------------------------------------------------
/**
*/
uint64_t
Scene::GetVersion() const
{
    return this->version;
}

//------------------------------------------------------------------------------
/**
    A record with an invalid guid is a released index, so this counts the entities the scene has,
    not the size of the array.
*/
IndexT
Scene::NumEntities() const
{
    IndexT num = 0;
    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        // A command that holds an entity id keeps the record, so an entity whose instance an undo
        // took
        // out is not counted until the command runs again.
        if (this->editables[i].guid.IsValid() && this->HasInstance(this->editables[i].entity))
            num++;
    }
    return num;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
Scene::ToGameEntity(Game::Entity entity) const
{
    if (entity == Game::Entity::Invalid())
        return entity;

    if (entity.index < this->editables.Size())
    {
        Game::Entity const gameEntity = this->editables[entity.index].gameEntity;
        if (gameEntity != Game::Entity::Invalid())
            return gameEntity;
    }
    return entity;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
Scene::ToEditorEntity(Game::Entity entity) const
{
    if (entity == Game::Entity::Invalid())
        return entity;

    if (entity.world == this->authoringWorld->GetWorldId())
        return entity;

    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        if (this->editables[i].gameEntity == entity)
            return Game::Entity(this->authoringWorld->GetWorldId(), entity.generation, i);
    }
    return Game::Entity::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::StartPlay()
{
    if (this->playing)
        return;

    this->playing = true;

    // The mode is set before the copy, because a property is activated when its world starts
    // simulating. The entities are mirrored one by one, but the properties are not, so this is
    // where
    // the sim world gets them.
    this->simWorld->mode = Game::PlayMode;
    this->RebuildSimWorld();
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::StopPlay()
{
    if (!this->playing)
        return;

    this->playing = false;

    // The mode is set before the rebuild, because a property is activated when its world starts
    // simulating and deactivated when it stops. Setting it after would activate properties that are
    // about to stop.
    this->simWorld->mode = Game::EditorMode;
    this->RebuildSimWorld();
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::RestartPlay()
{
    if (!this->playing)
        return;

    this->simWorld->mode = Game::PlayMode;
    this->RebuildSimWorld();
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::IsPlaying() const
{
    return this->playing;
}

//------------------------------------------------------------------------------
/**
    Copy an entity's authored row into the sim world, and initialize it when the instance is new,
    because the authoring world never initializes.
*/
void
Scene::MirrorEntity(Game::Entity entity)
{
    // A scene behind a tab has no mirrors. The next Activate copies the data over the sim world
    // again,
    // which is where the values it changed show up.
    if (!this->inSimWorld)
        return;

    if (!this->authoringWorld->HasInstance(entity))
        return;

    if (entity.index >= this->editables.Size())
        this->editables.Resize(entity.index + 1);

    Editable& editable = this->editables[entity.index];
    Game::Entity gameEntity = editable.gameEntity;
    if (gameEntity == Game::Entity::Invalid())
        gameEntity = this->simWorld->AllocateEntityId();

    Game::EntityMapping const srcMapping = this->authoringWorld->GetEntityMapping(entity);
    MemDb::Table& srcTable = this->authoringWorld->GetDatabase()->GetTable(srcMapping.table);

    MemDb::TableId dstTableId = this->simWorld->GetDatabase()->FindTable(srcTable.GetSignature());
    if (dstTableId == MemDb::InvalidTableId)
    {
        Game::EntityTableCreateInfo info;
        info.name = srcTable.name.Value();
        info.components = srcTable.GetAttributes();
        dstTableId = this->simWorld->CreateEntityTable(info);
    }
    MemDb::Table& dstTable = this->simWorld->GetDatabase()->GetTable(dstTableId);

    bool newInstance = false;
    MemDb::RowId dstRow;
    if (this->simWorld->HasInstance(gameEntity))
    {
        Game::EntityMapping const dstMapping = this->simWorld->GetEntityMapping(gameEntity);
        if (dstMapping.table == dstTableId)
        {
            dstRow = dstMapping.instance;
        }
        else
        {
            // The component set changed, which means the entity belongs in a different table
            this->simWorld->DeallocateInstance(gameEntity);
            dstRow = this->simWorld->AllocateInstance(gameEntity, dstTableId);
            newInstance = true;
        }
    }
    else
    {
        dstRow = this->simWorld->AllocateInstance(gameEntity, dstTableId);
        newInstance = true;
    }

    editable.gameEntity = gameEntity;

    for (IndexT i = 0; i < srcTable.GetAttributes().Size(); i++)
    {
        Game::ComponentId const component = srcTable.GetAttributes()[i];
        SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
        if (typeSize == 0)
            continue;

        // The owner column is written by AllocateInstance and must not be copied from the authoring
        // world
        if (component == Game::GetComponentId<Game::Entity>())
            continue;

        void* dst = dstTable.GetValuePointer(dstTable.GetAttributeIndex(component), dstRow);
        Memory::Copy(this->GetComponentData(entity, component), dst, typeSize);
        this->RemapComponent(component, dst, true);

        // A component copied onto an instance that is already initialized is initialized again,
        // which
        // is how the engine treats a value set on a live instance. This is what lets the model
        // registry see a changed resource.
        if (!newInstance)
        {
            Game::ComponentInterface* cInterface =
                static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
            if (cInterface->Init != nullptr)
                cInterface->Init(this->simWorld, gameEntity, dst);
        }
    }

    if (newInstance)
    {
        this->simWorld->InitializeInstance(gameEntity, dstTableId, dstRow);

        // An instance created by a copy is computed at its initialization, and one whose parent was
        // not in a table yet goes to the pending set. The parent of a mirrored entity is often
        // mirrored after it, so the pending instances are computed here rather than at the end of a
        // command batch.
        this->simWorld->GetHierarchy()->ResolvePending();
    }
}

//------------------------------------------------------------------------------
/**
    Copy one component value into the sim world and mark the instance as modified, so processors
    that only run on modified instances see the change. It writes through World::SetComponentValue
    rather than the row pointer, because that path also reaches the entities below, so the
    references are remapped on the copy and the features are told.
*/
void
Scene::MirrorComponent(Game::Entity entity, Game::ComponentId component)
{
    if (!this->inSimWorld)
        return;

    Game::Entity const gameEntity = this->ToGameEntity(entity);
    if (gameEntity == Game::Entity::Invalid() || !this->simWorld->IsValid(gameEntity) ||
        !this->simWorld->HasInstance(gameEntity))
        return;

    SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
    if (typeSize == 0)
        return;

    Game::EntityMapping const dstMapping = this->simWorld->GetEntityMapping(gameEntity);
    MemDb::Table& dstTable = this->simWorld->GetDatabase()->GetTable(dstMapping.table);
    if (!dstTable.HasAttribute(component))
        return;

    Util::Blob value((const byte*)this->GetComponentData(entity, component), typeSize);
    this->RemapComponent(component, value.GetPtr(), true);
    this->simWorld->SetComponentValue(gameEntity, component, value.GetPtr(), typeSize);
    this->NotifyComponentWritten(gameEntity, component);

    void* dst = dstTable.GetValuePointer(dstTable.GetAttributeIndex(component), dstMapping.instance);

    // The instance stays initialized, so the component is initialized the way the engine
    // initializes a
    // component whose value is set on a live instance. A Model component is how the registry learns
    // the entity points at a different slot.
    Game::ComponentInterface* cInterface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    if (cInterface->Init != nullptr)
        cInterface->Init(this->simWorld, gameEntity, dst);

    this->simWorld->MarkAsModified(gameEntity);
}

//------------------------------------------------------------------------------
/**
    Component data holds entity references, which are the authored entity and the sim entity, so
    they have to be converted when data crosses between the worlds.
*/
void
Scene::RemapComponent(Game::ComponentId component, void* data, bool toGame)
{
    if (component == Game::GetComponentId<Game::Entity>())
        return;

    Game::ComponentInterface* cInterface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));

    for (IndexT field = 0; field < cInterface->GetNumFields(); field++)
    {
        if (Util::StringAtom(cInterface->GetFieldTypenames()[field]) == "Game::Entity"_atm)
        {
            Game::Entity* entity = (Game::Entity*)((byte*)data + cInterface->GetFieldByteOffsets()[field]);
            *entity = toGame ? this->ToGameEntity(*entity) : this->ToEditorEntity(*entity);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Throw the sim world away and copy the authored data over again. Entities made while playing have
    no record, so they disappear here. The mappings are set before any data is remapped, because a
    reference to an entity further down would otherwise resolve through a mapping that is not set
    yet. Extension data comes last, as the text each extension writes, after the instances are
    initialized.
*/
void
Scene::RebuildSimWorld()
{
    Game::GameServer::Instance()->CleanupWorld(this->simWorld);
    Game::GameServer::Instance()->SetupEmptyWorld(this->simWorld);
    Game::World::Override(this->authoringWorld, this->simWorld);

    Game::Filter filter = Game::FilterBuilder().Including<Game::Entity>().Build();
    Game::Dataset data = this->simWorld->Query(filter);

    for (uint32_t v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Game::Entity const* entities = (Game::Entity*)view.buffers[0];

        for (uint16_t i = 0; i < view.numInstances; i++)
        {
            if (!view.validInstances.IsSet(i))
                continue;

            Game::Entity const gameEntity = entities[i];
            if (gameEntity.index < this->editables.Size())
                this->editables[gameEntity.index].gameEntity = gameEntity;
        }
    }

    for (uint32_t v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Game::Entity const* entities = (Game::Entity*)view.buffers[0];

        for (uint16_t i = 0; i < view.numInstances; i++)
        {
            if (!view.validInstances.IsSet(i))
                continue;

            Game::Entity const gameEntity = entities[i];

            MemDb::Table& table = this->simWorld->GetDatabase()->GetTable(view.tableId);
            for (Game::ComponentId const component : table.GetAttributes())
            {
                SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
                if (typeSize == 0)
                    continue;

                void* buffer = this->simWorld->GetInstanceBuffer(view.tableId, view.partitionId, component);
                this->RemapComponent(component, (byte*)buffer + i * typeSize, true);
            }

            MemDb::RowId const row = { view.partitionId, i };
            this->simWorld->InitializeInstance(gameEntity, view.tableId, row);
        }
    }
    Game::DestroyFilter(filter);

    // An extension writes its text with the scene's guids and applies it onto the mirror entity, so
    // a
    // reference has to answer as the mirror entity. The two maps are the same guids read against
    // the
    // two worlds.
    const Util::Array<Game::LevelExtension>& extensions = Game::LevelExtensionRegistry::GetAllExtensions();

    Util::HashTable<Util::Guid, Game::Entity> authoringMap;
    this->BuildSceneMap(authoringMap);

    Util::HashTable<Util::Guid, Game::Entity> gameMap;
    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        if (this->editables[i].guid.IsValid() && this->editables[i].gameEntity != Game::Entity::Invalid())
            gameMap.Add(this->editables[i].guid, this->editables[i].gameEntity);
    }

    for (IndexT i = 0; i < this->editables.Size(); i++)
    {
        Game::Entity const gameEntity = this->editables[i].gameEntity;
        if (gameEntity == Game::Entity::Invalid())
            continue;

        for (IndexT e = 0; e < extensions.Size(); e++)
        {
            if (extensions[e].capture == nullptr || extensions[e].apply == nullptr)
                continue;

            this->SetRefContext(authoringMap);

            Util::String text;
            extensions[e].capture(this->editables[i].entity, text);
            if (text.IsEmpty())
                continue;

            this->SetRefContext(gameMap);
            extensions[e].apply(gameEntity, text.AsCharPtr());
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
Scene::HasComponent(Game::Entity entity, Game::ComponentId component) const
{
    return this->authoringWorld != nullptr && this->authoringWorld->HasComponent(entity, component);
}

//------------------------------------------------------------------------------
/**
    The one place that reads the link. An entity whose instance was released is not part of an
    instance, so this answers InvalidIndex for it too, and the other accessors build on it.
*/
IndexT
Scene::GetPrefabInstanceIndex(Game::Entity entity) const
{
    if (entity.index >= this->editables.Size())
        return InvalidIndex;

    const IndexT instanceIndex = this->editables[entity.index].prefabInstance;
    if (instanceIndex == InvalidIndex || instanceIndex >= (IndexT)this->prefabInstances.Size())
        return InvalidIndex;

    if (!this->prefabInstances[instanceIndex].valid)
        return InvalidIndex;

    return instanceIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::SetPrefabOverride(Game::Entity entity, const PrefabOverride& override)
{
    if (entity.index >= this->editables.Size() || override.component == Game::ComponentId::Invalid())
        return;

    Editable& editable = this->editables[entity.index];
    for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
    {
        if (editable.prefabOverrides[i].component == override.component)
        {
            editable.prefabOverrides[i] = override;
            this->version++;
            return;
        }
    }
    editable.prefabOverrides.Append(override);
    this->version++;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::ClearPrefabOverride(Game::Entity entity, Game::ComponentId component)
{
    if (entity.index >= this->editables.Size())
        return;

    Editable& editable = this->editables[entity.index];
    for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
    {
        if (editable.prefabOverrides[i].component == component)
        {
            editable.prefabOverrides.EraseIndex(i);
            this->version++;
            return;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::RestorePrefabInstance(IndexT index, const PrefabInstance& record)
{
    if (index >= this->prefabInstances.Size())
        return;

    this->prefabInstances[index] = record;
    this->version++;
}

//------------------------------------------------------------------------------
/**
    The slot stays, because the records and the saved files still name its index. Only whether the
    scene treats it as an instance changes, which keeps an undone instantiation out of the outline
    and the save until the redo.
*/
void
Scene::SetPrefabInstanceValid(IndexT index, bool valid)
{
    if (index >= this->prefabInstances.Size() || this->prefabInstances[index].valid == valid)
        return;

    this->prefabInstances[index].valid = valid;
    this->version++;
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::SetEntityPrefabLink(Game::Entity entity, IndexT prefabInstance, const Util::Guid& prefabEntity,
    const Util::Array<PrefabOverride>& overrides)
{
    if (entity.index >= this->editables.Size() || !this->editables[entity.index].guid.IsValid())
        return;

    Editable& editable = this->editables[entity.index];
    editable.prefabInstance = prefabInstance;
    editable.prefabEntity = prefabEntity;
    editable.prefabOverrides = overrides;
    this->version++;
}

//------------------------------------------------------------------------------
/**
    Put an entity into an instance's set, once. Both the path that reads a file and the path that
    puts an entity back on an undo can reach the same entity twice.
*/
void
Scene::AppendInstanceEntity(IndexT instanceIndex, Game::Entity entity)
{
    if (instanceIndex >= (IndexT)this->prefabInstances.Size())
        return;

    Util::Array<Game::Entity>& entities = this->prefabInstances[instanceIndex].entities;
    if (entities.FindIndex(entity) == InvalidIndex)
        entities.Append(entity);
}

//------------------------------------------------------------------------------
/**
    Put an entity into an instance as its own addition. This is what an entity the level created
    under an instance entity becomes. It has no record, which is what makes it an addition rather
    than a copy.
*/
void
Scene::JoinInstance(Game::Entity entity, IndexT instanceIndex)
{
    if (entity.index >= this->editables.Size() || instanceIndex >= this->prefabInstances.Size())
        return;

    Editable& editable = this->editables[entity.index];
    editable.prefabInstance = instanceIndex;
    editable.prefabEntity = Util::Guid();
    this->AppendInstanceEntity(instanceIndex, entity);
    this->version++;
}

//------------------------------------------------------------------------------
/**
    Take an entity off the instance it was added to. Only an addition can leave, because an entity
    with a record stands where its file puts it. The mark goes with it.
*/
void
Scene::LeaveInstance(Game::Entity entity)
{
    if (entity.index >= this->editables.Size())
        return;

    Editable& editable = this->editables[entity.index];
    if (editable.prefabInstance == InvalidIndex || editable.prefabInstance >= this->prefabInstances.Size())
        return;

    Util::Array<Game::Entity>& entities = this->prefabInstances[editable.prefabInstance].entities;
    const IndexT i = entities.FindIndex(entity);
    if (i != InvalidIndex)
        entities.EraseIndex(i);

    editable.prefabInstance = InvalidIndex;
    editable.prefabEntity = Util::Guid();
    this->version++;
}

//------------------------------------------------------------------------------
/**
    Decide which instance an entity the level created belongs to. Membership follows the parent, and
    with no parent the collection decides it: an entity created inside one of the instance's
    collections is inside the instance. An entity with a record is not placed by the level, so it is
    not resolved here.
*/
void
Scene::ResolveInstanceMembership(Game::Entity entity)
{
    if (entity.index >= this->editables.Size())
        return;

    const Editable& editable = this->editables[entity.index];
    if (editable.prefabEntity.IsValid())
        return;

    IndexT instanceIndex = InvalidIndex;

    const Game::Entity parent = this->GetParent(entity);
    if (parent != Game::Entity::Invalid() && parent.index < this->editables.Size())
    {
        instanceIndex = this->editables[parent.index].prefabInstance;
    }
    else
    {
        for (IndexT i = 0; i < (IndexT)this->prefabInstances.Size(); i++)
        {
            if (this->prefabInstances[i].valid &&
                this->prefabInstances[i].collections.FindIndex(editable.collection) != InvalidIndex)
            {
                instanceIndex = i;
                break;
            }
        }
    }

    if (instanceIndex != InvalidIndex &&
        (instanceIndex >= (IndexT)this->prefabInstances.Size() || !this->prefabInstances[instanceIndex].valid))
        instanceIndex = InvalidIndex;

    if (instanceIndex == editable.prefabInstance)
        return;

    if (editable.prefabInstance != InvalidIndex)
        this->LeaveInstance(entity);

    if (instanceIndex != InvalidIndex)
        this->JoinInstance(entity, instanceIndex);
}

//------------------------------------------------------------------------------
/**
    Mark a record of the file as one with no entity for it. The entity is not in the scene, so this
    mark is the only thing that says the instance differs from the file.
*/
void
Scene::MarkRecordRemoved(IndexT instanceIndex, const RemovedRecord& marker)
{
    if (instanceIndex >= this->prefabInstances.Size())
        return;

    this->prefabInstances[instanceIndex].removedRecords.Append(marker);
    this->version++;
}

//------------------------------------------------------------------------------
/**
    Take the mark off again, which is what an undo of a delete does.
*/
void
Scene::ClearRecordRemoved(IndexT instanceIndex, const Util::Guid& record)
{
    if (instanceIndex >= this->prefabInstances.Size())
        return;

    Util::Array<RemovedRecord>& markers = this->prefabInstances[instanceIndex].removedRecords;
    for (IndexT i = 0; i < markers.Size(); i++)
    {
        if (markers[i].record == record)
        {
            markers.EraseIndex(i);
            break;
        }
    }
    this->version++;
}

//------------------------------------------------------------------------------
/**
    Put a record the instance had marked removed back into the scene. The mark goes first, because
    the entity is what the record names.
*/
bool
Scene::RevertRecord(IndexT instanceIndex, const Util::Guid& record, const Util::Guid& guid, Game::Entity entity)
{
    if (instanceIndex >= this->prefabInstances.Size())
        return false;

    const PrefabInstance& instance = this->prefabInstances[instanceIndex];

    IndexT marker = InvalidIndex;
    for (IndexT i = 0; i < instance.removedRecords.Size(); i++)
    {
        if (instance.removedRecords[i].record == record)
        {
            marker = i;
            break;
        }
    }
    if (marker == InvalidIndex)
        return false;

    Ptr<SceneDocument> doc = this->LoadDocumentCached(instance.source);
    if (!doc.isvalid())
        return false;

    const IndexT recordIndex = doc->FindEntity(record);
    if (recordIndex == InvalidIndex)
        return false;

    this->ClearRecordRemoved(instanceIndex, record);
    this->CreateInstanceEntity(instanceIndex, doc->entities[recordIndex], doc, recordIndex, guid, entity);
    return true;
}

} // namespace Editor
