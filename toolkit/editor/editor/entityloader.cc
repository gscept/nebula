//------------------------------------------------------------------------------
//  entityloader.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "entityloader.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
#include "io/ioserver.h"
#include "editor.h"
#include "cmds.h"
#include "commandmanager.h"
#include "game/componentserialization.h"
#include "editor/components/editorcomponents.h"

namespace Editor
{

__ImplementClass(Editor::EntityLoader, 'EELo', BaseGameFeature::LevelParser);

//------------------------------------------------------------------------------
/**
*/
void
WriteEntityGuid(Ptr<IO::JsonWriter> const& writer, const char* name, void* value)
{
    Game::Entity const entity = *(Game::Entity*)value;
    if (entity == Game::Entity::Invalid() || !Editor::state.editorWorld->IsValid(entity) ||
        !Editor::state.editorWorld->HasInstance(entity))
    {
        return; // no need to write invalid data
    }
    
    Editor::Editable const& edit = Editor::state.editables[entity.index];
    writer->Add(edit.guid.AsString(), name);
}

//------------------------------------------------------------------------------
/**
*/
bool
SaveEntities(const char* filePath)
{
    IO::URI const file = filePath;
    if (!IO::IoServer::Instance()->EnsureDirectoriesForFile(file))
    {
        n_warning("Could not create directory for JSON level '%s'\n", filePath);
        return false;
    }
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(IO::IoServer::Instance()->CreateStream(file));

    // TODO: Maybe move this to a SceneSerializer class that can be used outside of the editor as well.

    // TODO: only set once, both serialize and deserialize
    Game::ComponentSerialization::OverrideType(
        Game::ComponentSerialization::ENTITY,
        nullptr,
        &WriteEntityGuid
    );

    if (writer->Open())
    {
        writer->BeginObject("level");
        
        writer->Add(100, "version");

        writer->BeginObject("editor");
        writer->BeginObject("collections");
        for (Collection const& collection : state.collections)
        {
            writer->BeginObject(collection.guid.AsString().AsCharPtr());
            writer->Add(collection.name, "name");
            if (collection.parent.IsValid())
            {
                writer->Add(collection.parent, "parent");
            }
            writer->Add(collection.order, "order");
            writer->End();
        }
        writer->End(); // end collections

        writer->BeginObject("entities");
        Game::Filter editorFilter = Game::FilterBuilder().Including<Game::Entity>().Build();
        Game::Dataset editorData = state.editorWorld->Query(editorFilter);
        for (int v = 0; v < editorData.numViews; v++)
        {
            Game::Dataset::View const& view = editorData.views[v];
            Editor::Entity const* const entities = (Editor::Entity*)view.buffers[0];
            for (IndexT i = 0; i < view.numInstances; i++)
            {
                Editable const& editable = state.editables[entities[i].index];
                writer->BeginObject(editable.guid.AsString().AsCharPtr());
                if (editable.collection.IsValid())
                {
                    writer->Add(editable.collection, "collection");
                }
                writer->Add(editable.collectionOrder, "order");
                writer->End();
            }
        }
        Game::DestroyFilter(editorFilter);
        writer->End(); // end editor entities
        writer->End(); // end editor

        writer->BeginObject("entities");

        Game::Filter filter = Game::FilterBuilder().Including<Game::Entity>().Build();
        Game::Dataset data = state.editorWorld->Query(filter);
        Game::ComponentId entityPID = Game::GetComponentId<Game::Entity>();

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            Editor::Entity const* const entities = (Editor::Entity*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Editor::Entity const& editorEntity = entities[i];
                Editable& edit = state.editables[editorEntity.index];
                Game::EntityMapping const mapping = Editor::state.editorWorld->GetEntityMapping(editorEntity);
                writer->BeginObject(edit.guid.AsString().AsCharPtr());
                writer->Add(edit.name, "name");
                MemDb::Table const& table = Editor::state.editorWorld->GetDatabase()->GetTable(view.tableId);
                IndexT col = 0;
                if (table.GetAttributes().Size() > 1)
                {
                    writer->BeginObject("components");
                    for (auto component : table.GetAttributes())
                    {
                        uint32_t const flags = MemDb::AttributeRegistry::Flags(component);
                        if (component != entityPID && (flags & Game::ComponentFlags::COMPONENTFLAG_DECAY) == 0)
                        {
                            SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
                            if (typeSize > 0)
                            {
                                void* buffer = Editor::state.editorWorld->GetInstanceBuffer(
                                    view.tableId, mapping.instance.partition, component
                                );
                                n_assert(buffer != nullptr);
                                Game::ComponentSerialization::Serialize(
                                    writer,
                                    component,
                                    ((byte*)buffer) + MemDb::AttributeRegistry::TypeSize(component) * mapping.instance.index
                                );
                            }
                            else
                            {
                                writer->Add("null", MemDb::AttributeRegistry::GetAttribute(component)->name.Value());
                            }
                        }

                        col++;
                    }
                    writer->End(); // end components
                }
                writer->End(); // end entity (GUID)
            }
        }

        Game::DestroyFilter(filter);

        writer->End(); // end entities
        writer->End(); // end level
        writer->Close();
        Game::ComponentSerialization::OverrideType(Game::ComponentSerialization::ENTITY, nullptr, nullptr);
        return true;
    }

    Game::ComponentSerialization::OverrideType(Game::ComponentSerialization::ENTITY, nullptr, nullptr);
    return false;
}

//------------------------------------------------------------------------------
/**
*/
EntityLoader::EntityLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EntityLoader::~EntityLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EntityLoader::SetGenerateGuids(bool generate)
{
    this->generateGuids = generate;
}

//------------------------------------------------------------------------------
/**
*/
void
EntityLoader::BeginLoad()
{
    this->sourceEntities.Clear();
    this->loadedEntities.Clear();
    Edit::CommandManager::BeginMacro("Load entities", false);
}

//------------------------------------------------------------------------------
/**
*/
void
EntityLoader::AddEntity(Game::Entity entity, Util::Guid const& guid)
{
    while (Editor::state.editables.Size() <= entity.index)
        Editor::state.editables.Append({});

    // TODO: We need to add entities with guids before actually encountering the
    //       actual entity sometimes if the entity is referenced before it's
    //       instantiated.

    Editable& editable = Editor::state.editables[entity.index];
    
    n_assert(editable.gameEntity == Game::Entity::Invalid());
    
    if (this->generateGuids)
    {
        editable.guid.Generate();
    }
    else
    {
        editable.guid = guid;
    }

    editable.version++;
    editable.collection = Util::Guid();
    editable.collectionOrder = 0;
    this->sourceEntities.Add(guid, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
EntityLoader::LoadCollections(const Ptr<IO::JsonReader>& reader)
{
    reader->SetToRoot();
    if (!reader->SetToNode("/level/editor"))
    {
        return;
    }

    Util::HashTable<Util::Guid, Util::Guid> collectionGuids;
    Util::Array<IndexT> loadedCollections;
    Util::Array<Util::Guid> sourceParents;

    if (reader->SetToFirstChild("collections") && reader->SetToFirstChild())
    {
        do
        {
            Util::Guid const sourceGuid = Util::Guid::FromString(reader->GetCurrentNodeName());
            Collection collection;
            collection.guid = sourceGuid;
            if (this->generateGuids)
            {
                collection.guid.Generate();
            }
            collection.name = reader->GetOptString("name", "Collection");
            collection.order = reader->GetOptInt("order", 0);

            Util::Guid sourceParent;
            reader->GetOpt<Util::Guid>(sourceParent, "parent");

            collectionGuids.Add(sourceGuid, collection.guid);
            sourceParents.Append(sourceParent);
            loadedCollections.Append(Editor::state.collections.Size());
            Editor::state.collections.Append(collection);
        }
        while (reader->SetToNextChild());
    }

    for (IndexT i = 0; i < loadedCollections.Size(); i++)
    {
        if (!sourceParents[i].IsValid())
        {
            continue;
        }
        IndexT const parentIndex = collectionGuids.FindIndex(sourceParents[i]);
        if (parentIndex != InvalidIndex)
        {
            Editor::state.collections[loadedCollections[i]].parent = collectionGuids.ValueAtIndex(sourceParents[i], parentIndex);
        }
    }

    for (IndexT const collectionIndex : loadedCollections)
    {
        Collection& collection = Editor::state.collections[collectionIndex];
        Util::Guid ancestor = collection.parent;
        SizeT depth = 0;
        while (ancestor.IsValid() && depth++ <= Editor::state.collections.Size())
        {
            if (ancestor == collection.guid)
            {
                n_warning("Collection hierarchy contains a cycle at '%s'; moving it to the scene root\n", collection.name.AsCharPtr());
                collection.parent = Util::Guid();
                break;
            }
            IndexT const ancestorIndex = Editor::FindCollection(ancestor);
            if (ancestorIndex == InvalidIndex)
            {
                collection.parent = Util::Guid();
                break;
            }
            ancestor = Editor::state.collections[ancestorIndex].parent;
        }
    }

    reader->SetToRoot();
    if (!reader->SetToNode("/level/editor") || !reader->SetToFirstChild("entities") || !reader->SetToFirstChild())
    {
        return;
    }

    do
    {
        Util::Guid const sourceEntityGuid = Util::Guid::FromString(reader->GetCurrentNodeName());
        IndexT const entityIndex = this->sourceEntities.FindIndex(sourceEntityGuid);
        if (entityIndex == InvalidIndex)
        {
            continue;
        }

        Game::Entity const entity = this->sourceEntities.ValueAtIndex(sourceEntityGuid, entityIndex);
        Editable& editable = Editor::state.editables[entity.index];
        editable.collectionOrder = reader->GetOptInt("order", 0);

        Util::Guid sourceCollection;
        if (reader->GetOpt<Util::Guid>(sourceCollection, "collection"))
        {
            IndexT const collectionIndex = collectionGuids.FindIndex(sourceCollection);
            if (collectionIndex != InvalidIndex)
            {
                editable.collection = collectionGuids.ValueAtIndex(sourceCollection, collectionIndex);
            }
        }
    }
    while (reader->SetToNextChild());
}

//------------------------------------------------------------------------------
/**
*/
void
EntityLoader::SetName(Game::Entity entity, const Util::String& name)
{
    Editable& editable = Editor::state.editables[entity.index];
    editable.name = name;
}

//------------------------------------------------------------------------------
/**
*/
void
EntityLoader::CommitEntity(Editor::Entity editorEntity)
{
    Editable& editable = Editor::state.editables[editorEntity.index];
    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    
    Game::Entity gameEntity = gameWorld->AllocateEntityId();
    
    editable.gameEntity = gameEntity;
    editable.version++;

    this->loadedEntities.Append(editorEntity);
}

//------------------------------------------------------------------------------
/**
*/
void
EntityLoader::CommitLevel()
{
    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    Game::World* editorWorld = Editor::state.editorWorld;
    for (Editor::Entity editorEntity : this->loadedEntities)
    {
        Editable& editable = Editor::state.editables[editorEntity.index];

        auto const mapping = editorWorld->GetEntityMapping(editorEntity);
        MemDb::Table const& editorTable = editorWorld->GetDatabase()->GetTable(mapping.table);
        MemDb::TableSignature const& signature = editorTable.GetSignature();

        MemDb::TableId gameTableId = gameWorld->GetDatabase()->FindTable(signature);
        if (gameTableId == MemDb::TableId::Invalid())
        {
            Game::EntityTableCreateInfo info;
            info.components = editorTable.GetAttributes();
            info.name = editorTable.name.Value();
            gameTableId = gameWorld->CreateEntityTable(info);
        }

        Util::Blob entityData = editorTable.SerializeInstance(mapping.instance);
        MemDb::RowId const gameInstance = gameWorld->AllocateInstance(editable.gameEntity, gameTableId, &entityData);
        Editor::RemapInstanceToGame(gameWorld, editable.gameEntity);
        gameWorld->InitializeInstance(editable.gameEntity, gameTableId, gameInstance);

        Editor::EditorEntity* editorEntityComponent = gameWorld->AddComponent<Editor::EditorEntity>(editable.gameEntity);
        editorEntityComponent->id = (uint64_t)editorEntity;
    }
    Edit::CommandManager::EndMacro();
}

} // namespace Editor
