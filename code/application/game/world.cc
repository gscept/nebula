//------------------------------------------------------------------------------
//  @file entitypool.cc
//  @copyright (C) 2021-2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "world.h"
#include "gameserver.h"
#include "api.h"
#include "category.h"
#include "util/queue.h"
#include "memdb/database.h"
#include "memdb/table.h"
#include "entitypool.h"
#include "memory/arenaallocator.h"
#include "ids/idallocator.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "imgui.h"
#include "game/componentinspection.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "io/binaryreader.h"
#include "io/ioserver.h"
#include "basegamefeature/level.h"
#include "flat/game/level.h"
#include "util/blob.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
World*
GetWorld(WorldHash hash)
{
    return GameServer::Instance()->GetWorld(hash);
}

//------------------------------------------------------------------------------
/**
*/
World*
GetWorld(WorldId id)
{
    return GameServer::Instance()->GetWorld(id);
}

//------------------------------------------------------------------------------
/**
*/
World::World(WorldHash hash, WorldId id)
    : numEntities(0),
      hash(hash),
      worldId(id),
      pipeline(this)
{
    this->db = MemDb::Database::Create();

    // Create a table that can hold new, empty entities
    MemDb::AttributeId attributes[4] = {
        Game::GetComponentId<Game::Entity>(),
        Game::GetComponentId<Game::Position>(),
        Game::GetComponentId<Game::Orientation>(),
        Game::GetComponentId<Game::Scale>()};
    MemDb::TableCreateInfo info = {.name = "Empty", .attributeIds = attributes, .numAttributes = 4};
    this->defaultTableId = this->db->CreateTable(info);

    // clang-format off
    this->pipeline.RegisterFrameEvent( 10,   "OnBeginFrame");
    this->pipeline.RegisterFrameEvent( 100,  "OnFrame");
    this->pipeline.RegisterFrameEvent( 200,  "OnEndFrame");
    // clang-format on
}

//------------------------------------------------------------------------------
/**
*/
World::~World()
{
    this->db = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
PackedLevel*
World::PreloadLevel(Util::String const& path)
{
    PackedLevel* level = new PackedLevel();
    level->world = this;

    Ptr<IO::BinaryReader> reader = IO::BinaryReader::Create();
    reader->SetStream(IO::IoServer::Instance()->CreateStream(path));
    reader->SetMemoryMappingEnabled(true);
    reader->Open();

    ubyte* data = reader->mapCursor;

    auto flatLevel = Game::Serialization::GetLevel(data);
    auto flatTables = flatLevel->tables();

    Util::FixedArray<ComponentId> componentIds(flatLevel->component_descriptions()->size());
    uint componentIndex = 0;
    for (auto desc : *flatLevel->component_descriptions())
    {
        const char* componentName = desc->name()->c_str();
        ComponentId cid = MemDb::AttributeRegistry::GetAttributeId(componentName);
        componentIds[componentIndex++] = cid;

#ifndef PUBLIC_BUILD
        Game::ComponentInterface const* cInterface =
            static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(cid));

        // TODO: Validate all fields types as well and assert if incorrect!
        n_assert(cInterface->GetNumFields() == desc->fields()->size());
#endif
    }

    Util::FixedArray<Util::StringAtom> strings(flatLevel->strings()->size());

    for (uint32_t i = 0; i < flatLevel->strings()->size(); i++)
    {
        Util::StringAtom strAtm = (*flatLevel->strings())[i]->data();
        strings[i] = strAtm;
    }

    for (auto table : *flatTables)
    {
        Game::PackedLevel::EntityGroup entityGroup;

        size_t const numTableComponents = table->components()->size();
        Util::FixedArray<ComponentId> components((SizeT)numTableComponents);
        componentIndex = 0;
        for (auto c : *table->components())
        {
            ComponentId cid = componentIds[c];
            components[componentIndex++] = cid;
        }
        MemDb::TableId const tableId = this->CreateEntityTable({.name = "", .components = components});
        entityGroup.dstTable = tableId;
        entityGroup.numRows = table->num_rows();

        n_assert(entityGroup.numRows > 0);

        size_t bytesInWholeTable = 0;
        for (auto column : *table->columns())
        {
            bytesInWholeTable += column->bytes()->size();
        }

        n_assert(bytesInWholeTable > 0);

        entityGroup.columns = new byte[bytesInWholeTable];

        size_t offset = 0;
        for (auto column : *table->columns())
        {
            Memory::Copy(column->bytes()->data(), entityGroup.columns + offset, column->bytes()->size());
            offset += column->bytes()->size();
        }

        // Patch strings
        offset = 0;
        size_t const numComponents = components.Size();
        for (componentIndex = 0; componentIndex < numComponents; componentIndex++)
        {
            ComponentId const cid = components[componentIndex];
            auto component_description = (*flatLevel->component_descriptions())[(*table->components())[componentIndex]];
            Game::ComponentInterface const* cInterface =
                static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(cid));

            size_t const bytesInColumn = entityGroup.numRows * cInterface->typeSize;

            size_t const numFields = cInterface->GetNumFields();
            for (IndexT i = 0; i < numFields; i++)
            {
                auto fieldTypename = cInterface->GetFieldTypenames()[i];
                auto component_field = (*component_description->fields())[i];

                // Check for strings and unpack them
                if (component_field->feature() == Game::Serialization::ComponentFieldFeature_StringAtom)
                {
                    ubyte* it = entityGroup.columns + offset;
                    it += cInterface->GetFieldByteOffsets()[i];
                    while (it < entityGroup.columns + offset + bytesInColumn)
                    {
                        static_assert(sizeof(Util::StringAtom) == sizeof(uint64_t));

                        Util::StringAtom* asStringAtom = reinterpret_cast<Util::StringAtom*>(it);
                        uint64_t asInt = *reinterpret_cast<uint64_t*>(it);

                        *asStringAtom = strings[asInt];

                        it += cInterface->typeSize;
                    }
                }
            }
            offset += (*table->columns())[componentIndex]->bytes()->size();
        }
        level->tables.Append(std::move(entityGroup));
    }

    reader->Close();

    return level;
}

//------------------------------------------------------------------------------
/**
*/
void
World::UnloadLevel(PackedLevel* level)
{
    delete level;
}

//------------------------------------------------------------------------------
/**
*/
void
World::ExportLevel(Util::String const& path)
{
    using namespace Game::Serialization;
    using namespace flatbuffers;

    flatbuffers::FlatBufferBuilder builder;

    Util::HashTable<Game::ComponentId, IndexT> componentsUsed;

    std::vector<Offset<ComponentDescription>> descriptions;

    Ptr<MemDb::Database> db = this->GetDatabase();

    std::vector<Offset<EntityGroup>> entityGroups;

    std::vector<Offset<String>> strings;
    Util::HashTable<Util::StringAtom, uint> stringTable;

    db->ForEachTable(
        [&](MemDb::TableId tid)
        {
            // necessary to defragment first, since we might have invalid instances in the partitions.
            this->Defragment(tid);

            std::vector<uint> components;
            std::vector<Offset<Column>> columns;

            MemDb::Table& table = db->GetTable(tid);

            if (table.GetNumRows() == 0)
                return;

            auto const& attributes = table.GetAttributes();
            for (IndexT columnIndex = 0; columnIndex < attributes.Size(); columnIndex++)
            {
                Game::ComponentId cid = attributes[columnIndex];
                Game::ComponentInterface const* cInterface =
                    static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(cid));

                if (!componentsUsed.Contains(cid))
                {
                    std::vector<Offset<ComponentField>> fields;

                    for (IndexT i = 0; i < cInterface->GetNumFields(); i++)
                    {
                        auto field_name = builder.CreateString(cInterface->GetFieldNames()[i]);
                        // TODO: Add field type and size for validation

                        ComponentFieldFeature feature = ComponentFieldFeature::ComponentFieldFeature_Undefined;

                        const char* fieldTypename = cInterface->GetFieldTypenames()[i];
                        if (Util::String::StrCmp(fieldTypename, "Resources::ResourceName") == 0 ||
                            Util::String::StrCmp(fieldTypename, "string") == 0 ||
                            Util::String::StrCmp(fieldTypename, "Util::StringAtom") == 0)
                        {
                            feature = ComponentFieldFeature::ComponentFieldFeature_StringAtom;
                        }
                        else if (Util::String::StrCmp(fieldTypename, "Game::Entity") == 0 || Util::String::StrCmp(fieldTypename, "entity") == 0)
                        {
                            feature = ComponentFieldFeature::ComponentFieldFeature_EntityId;
                        }

                        auto component_field = CreateComponentField(builder, field_name, feature);

                        fields.push_back(component_field);
                    }

                    auto vector_fields = builder.CreateVector(fields);
                    auto component_name = builder.CreateString(cInterface->GetName());
                    auto component_description =
                        CreateComponentDescription(builder, component_name, cInterface->typeSize, vector_fields);

                    descriptions.push_back(component_description);
                    componentsUsed.Add(cid, (IndexT)(descriptions.size() - 1));
                    components.push_back((uint32_t)(descriptions.size() - 1));
                }
                else
                {
                    components.push_back(componentsUsed[cid]);
                }

                // pack data from all partitions into single buffer
                SizeT const columnDataSize = table.GetNumRows() * cInterface->typeSize;
                ubyte* columnData = new ubyte[columnDataSize];

                MemDb::Table::Partition* currentPartition = table.GetFirstActivePartition();

                IndexT columnDataOffset = 0;
                while (currentPartition != nullptr)
                {
                    SizeT numBytesToCopy = currentPartition->numRows * cInterface->typeSize;
                    Memory::Copy((ubyte*)currentPartition->columns[columnIndex], columnData + columnDataOffset, numBytesToCopy);
                    columnDataOffset += numBytesToCopy;

                    currentPartition = currentPartition->next;
                }

                for (IndexT i = 0; i < cInterface->GetNumFields(); i++)
                {
                    auto fieldTypename = cInterface->GetFieldTypenames()[i];
                    // Check for strings and serialize them
                    // TODO: This could be improved and generalized so that we can
                    //       do the same for entity references and other reference types
                    if (Util::String::StrCmp(fieldTypename, "Resources::ResourceName") == 0 ||
                        Util::String::StrCmp(fieldTypename, "string") == 0 ||
                        Util::String::StrCmp(fieldTypename, "Util::StringAtom") == 0)
                    {
                        // This is a stringatom field, so we to serialize the
                        // string into the string table, and replace the
                        // pointer with an index into this table
                        ubyte* it = columnData;
                        it += cInterface->GetFieldByteOffsets()[i];
                        while (it < columnData + columnDataSize)
                        {
                            static_assert(sizeof(Util::StringAtom) == sizeof(uint64_t));

                            Util::StringAtom* asStringAtom = reinterpret_cast<Util::StringAtom*>(it);
                            uint64_t* asInt = reinterpret_cast<uint64_t*>(it);

                            IndexT const stringTableIndex = stringTable.FindIndex(*asStringAtom);
                            uint64_t stringIndex;
                            if (stringTableIndex == InvalidIndex)
                            {
                                auto flat_string = builder.CreateString(asStringAtom->Value());
                                stringIndex = strings.size();
                                strings.push_back(flat_string);

                                stringTable.Add(*asStringAtom, stringIndex);
                            }
                            else
                            {
                                stringIndex = stringTable.ValueAtIndex(*asStringAtom, stringTableIndex);
                            }
                            *asInt = stringIndex;

                            it += cInterface->typeSize;
                        }
                    }
                }

                auto vector_bytes = builder.CreateVector((ubyte*)columnData, columnDataSize);
                auto flat_column = CreateColumn(builder, vector_bytes);

                columns.push_back(flat_column);

                delete[] columnData;
            }

            auto vector_components = builder.CreateVector(components);
            auto vector_columns = builder.CreateVector(columns);

            auto flat_table = CreateEntityGroup(builder, vector_components, table.GetNumRows(), vector_columns);

            entityGroups.push_back(flat_table);
        }
    );

    auto vector_entity_groups = builder.CreateVector(entityGroups);
    auto vector_descs = builder.CreateVector(descriptions);
    auto vector_strings = builder.CreateVector(strings);

    auto flat_level = CreateLevel(builder, vector_descs, vector_entity_groups, vector_strings);

    builder.Finish(flat_level);

    Ptr<IO::BinaryWriter> writer = IO::BinaryWriter::Create();
    writer->SetStream(IO::IoServer::Instance()->CreateStream(path));
    writer->Open();
    writer->WriteRawData(builder.GetBufferPointer(), builder.GetSize());
    writer->Close();
}

//------------------------------------------------------------------------------
/**
*/
Entity
World::AllocateEntityId()
{
    Entity entity;
    if (this->pool.Allocate(entity))
    {
        this->entityMap.Append({MemDb::InvalidTableId, MemDb::InvalidRow});
    }
    entity.world = (uint32_t)this->worldId;
    this->numEntities++;
    return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateEntityId(Entity entity)
{
    n_assert(!this->HasInstance(entity));
    this->pool.Deallocate(entity);
    this->numEntities--;
}

//------------------------------------------------------------------------------
/**
*/
void
World::Start()
{
    // "Prefilter" the processors with the new table (insert the table in the cache that accepts it)
    this->pipeline.CacheTable(this->defaultTableId, this->db->GetTable(this->defaultTableId).GetSignature());
    this->pipeline.Begin();
}

//------------------------------------------------------------------------------
/**
*/
void
World::BeginFrame()
{
    this->pipeline.RunThru("OnBeginFrame");
    ExecuteAddComponentCommands();
}

//------------------------------------------------------------------------------
/**
*/
void
World::SimFrame()
{
    this->pipeline.RunThru("OnFrame");
    ExecuteAddComponentCommands();
}

//------------------------------------------------------------------------------
/**
*/
void
World::EndFrame()
{
    this->pipeline.RunThru("OnEndFrame");

    // remove first, then add
    ExecuteRemoveComponentCommands();
    ExecuteAddComponentCommands();

    this->pipeline.RunRemaining();
    this->pipeline.Reset();
}

//------------------------------------------------------------------------------
/**
*/
void
World::OnLoad()
{
    // TODO: Implement me
}

//------------------------------------------------------------------------------
/**
*/
void
World::OnSave()
{
    // TODO: Implement me
}

//------------------------------------------------------------------------------
/**
*/
void
World::PrefilterProcessors()
{
    this->pipeline.Prefilter(!this->cacheValid);
    this->cacheValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void
World::ManageEntities()
{
    // NOTE: The order of the following loops are important!

    // Clean up entities
    while (!this->deallocQueue.IsEmpty())
    {
        auto const cmd = this->deallocQueue.Dequeue();
        if (this->IsValid(cmd.entity))
        {
            MemDb::TableId const table = this->entityMap[cmd.entity.index].table;
            MemDb::RowId const row = this->entityMap[cmd.entity.index].instance;
            this->DeallocateInstance(table, row);
            this->entityMap[cmd.entity.index].table = MemDb::InvalidTableId;
            this->entityMap[cmd.entity.index].instance = MemDb::InvalidRow;
            this->DeallocateEntityId(cmd.entity);
        }
    }

    // Allocate instances for new entities, reuse invalid instances if possible
    while (!this->allocQueue.IsEmpty())
    {
        auto const cmd = this->allocQueue.Dequeue();
        n_assert(this->IsValid(cmd.entity));
        //this->AllocateInstance(cmd.entity, cmd.tid);
        this->FinalizeAllocate(cmd.entity);
    }

    // Delete all remaining invalid instances
    Ptr<MemDb::Database> const& db = this->db;

    if (db.isvalid())
    {
        db->ForEachTable(
            [this](MemDb::TableId tid)
            {
                this->Defragment(tid);
            }
        );
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::Reset()
{
    this->cacheValid = false;
    this->db->Reset();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<MemDb::Database>
World::GetDatabase()
{
    return this->db;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
World::CreateEntity(bool immediate)
{
    Entity const entity = this->AllocateEntityId();

    World::AllocateInstanceCommand cmd;
    cmd.entity = entity;

    if (immediate)
    {
        this->AllocateInstance(cmd.entity, this->defaultTableId);
    }
    else
    {
        this->allocQueue.Enqueue(std::move(cmd));
    }

    return entity;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
World::CreateEntity(EntityCreateInfo const& info)
{
    World::AllocateInstanceCommand cmd;
    if (info.templateId != TemplateId::Invalid())
    {
        cmd.tid = info.templateId;
    }
    else
    {
        n_warning("Trying to instantiate an invalid template!");
        return Game::Entity::Invalid();
    }
    Entity const entity = this->AllocateEntityId();
    cmd.entity = entity;

    if (!info.immediate)
    {
        this->AllocateInstance(cmd.entity, cmd.tid, false);
        this->allocQueue.Enqueue(std::move(cmd));
    }
    else
    {
        this->AllocateInstance(cmd.entity, cmd.tid, true);
    }

    return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeleteEntity(Game::Entity entity)
{
    n_assert(this->IsValid(entity));

    if (this->HasInstance(entity))
    {
        World::DeallocInstanceCommand cmd;
        cmd.entity = entity;

        this->deallocQueue.Enqueue(std::move(cmd));
    }
    else
    {
        // entity hasn't been instantiated, can just delete the id straight away.
        this->DeallocateEntityId(entity);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::DecayComponent(Game::ComponentId component, MemDb::TableId tableId, MemDb::ColumnIndex column, MemDb::RowId instance)
{
    if (MemDb::AttributeRegistry::Flags(component) & ComponentFlags::COMPONENTFLAG_DECAY)
    {
        if (component.id >= this->componentDecayTable.Size())
            this->componentDecayTable.Resize(
                component.id + 16
            ); // increment with a couple of extra elements, instead of doubling size, just to avoid extreme overallocation
        ComponentDecayBuffer& decayBuffer = this->componentDecayTable[component.id];

        uint64_t const typeSize = (uint64_t)MemDb::AttributeRegistry::TypeSize(component);

        if (decayBuffer.capacity == 0)
        {
            decayBuffer.size = 0;
            decayBuffer.capacity = 64;
            decayBuffer.buffer = Memory::Alloc(Memory::HeapType::DefaultHeap, decayBuffer.capacity * typeSize);
        }

        if (decayBuffer.capacity == decayBuffer.size)
        {
            void* oldBuffer = decayBuffer.buffer;
            decayBuffer.capacity *= 2;
            decayBuffer.buffer = Memory::Alloc(Memory::HeapType::DefaultHeap, decayBuffer.capacity * typeSize);
            Memory::Copy(oldBuffer, decayBuffer.buffer, typeSize * (uint64_t)decayBuffer.size);
            Memory::Free(Memory::HeapType::DefaultHeap, oldBuffer);
        }

        void* dst = ((byte*)decayBuffer.buffer) + (typeSize * (uint64_t)decayBuffer.size);
        decayBuffer.size++;
        Memory::Copy(this->db->GetTable(tableId).GetValuePointer(column, instance), dst, typeSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
void*
World::AddComponent(Entity entity, Game::ComponentId id)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(), "Adding component to entities while in an async processor is currently not supported!"
    );
#endif
    SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(id);
    void* data = this->componentStageAllocator.Alloc(typeSize);
    const void* defaultValue = MemDb::AttributeRegistry::DefaultValue(id);
    Memory::Copy(defaultValue, data, typeSize);

    AddStagedComponentCommand cmd = {
        .entity = entity,
        .componentId = id,
        .dataSize = typeSize,
        .data = data,
    };
    this->addStagedQueue.Append(cmd);

    if (this->componentInitializationEnabled)
    {
        MemDb::Attribute* attr = MemDb::AttributeRegistry::GetAttribute(id);
        ComponentInterface* cInterface;
        cInterface = static_cast<ComponentInterface*>(attr);

        if (cInterface->Init != nullptr)
        {
            // run initialization function if it exists.
            cInterface->Init(this, entity, data);
        }
    }

    return data;
}

//------------------------------------------------------------------------------
/**
*/
void
World::RemoveComponent(Entity entity, ComponentId component)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Removing components from entities while executing an async processor is currently not supported!"
    );
#endif
    RemoveComponentCommand cmd = {
        .entity = entity,
        .componentId = component,
    };
    this->removeComponentQueue.Append(cmd);
}

//------------------------------------------------------------------------------
/**
*/
ComponentDecayBuffer const
World::GetDecayBuffer(Game::ComponentId component)
{
    if (component < this->componentDecayTable.Size())
        return this->componentDecayTable[component.id];
    else
        return ComponentDecayBuffer();
}

//------------------------------------------------------------------------------
/**
*/
void
World::ClearDecayBuffers()
{
    for (auto& cdb : this->componentDecayTable)
    {
        // TODO: shrink buffers if they're unreasonably big.
        cdb.size = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
World::IsValid(Entity e) const
{
    return (uint32_t)this->worldId == e.world && this->pool.IsValid(e);
}

//------------------------------------------------------------------------------
/**
*/
bool
World::HasInstance(Entity e) const
{
    n_assert(this->IsValid(e));
    return this->entityMap[e.index].instance != MemDb::InvalidRow;
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
World::GetEntityMapping(Game::Entity entity) const
{
    n_assert(this->IsValid(entity));
    n_assert(this->HasInstance(entity));
    return this->entityMap[entity.index];
}

//------------------------------------------------------------------------------
/**
   TODO: This is not thread safe!
*/
bool
World::HasComponent(Game::Entity const entity, ComponentId const component) const
{
    EntityMapping mapping = this->GetEntityMapping(entity);
    return this->db->GetTable(mapping.table).HasAttribute(component);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
World::GetNumInstances(MemDb::TableId tid)
{
    return this->db->GetTable(tid).GetNumRows();
}

//------------------------------------------------------------------------------
/**
*/
void
World::ExecuteAddComponentCommands()
{
    auto sortFunc = [](const void* lhs, const void* rhs) -> int
    {
        Game::Entity arg1 = ((const AddStagedComponentCommand*)lhs)->entity;
        Game::Entity arg2 = ((const AddStagedComponentCommand*)rhs)->entity;
        return (arg1 > arg2) - (arg1 < arg2);
    };

    this->addStagedQueue.QuickSortWithFunc(sortFunc);

    auto* currentCmd = this->addStagedQueue.Begin();
    auto* end = this->addStagedQueue.End();
    while (currentCmd != end)
    {
        Entity currentEntity = currentCmd->entity;
        auto* firstCmdOfEntity = currentCmd;
        SizeT numEntityCmds = 0;
        while (currentCmd->entity == currentEntity && currentCmd != end)
        {
            numEntityCmds++;
            currentCmd++;
        }
        AddStagedComponentsToEntity(currentEntity, firstCmdOfEntity, numEntityCmds);
    }
    // release all memory of the staged components
    componentStageAllocator.Release();
    addStagedQueue.Reset();
}

//------------------------------------------------------------------------------
/**
*/
void
World::ExecuteRemoveComponentCommands()
{
    auto sortFunc = [](const void* lhs, const void* rhs) -> int
    {
        Game::Entity arg1 = ((const RemoveComponentCommand*)lhs)->entity;
        Game::Entity arg2 = ((const RemoveComponentCommand*)rhs)->entity;
        return (arg1 > arg2) - (arg1 < arg2);
    };

    this->removeComponentQueue.QuickSortWithFunc(sortFunc);

    auto* currentCmd = this->removeComponentQueue.Begin();
    auto* end = this->removeComponentQueue.End();
    while (currentCmd != end)
    {
        Entity currentEntity = currentCmd->entity;
        auto* firstCmdOfEntity = currentCmd;
        SizeT numEntityCmds = 0;
        while (currentCmd->entity == currentEntity && currentCmd != end)
        {
            numEntityCmds++;
            currentCmd++;
        }
        RemoveComponentsFromEntity(currentEntity, firstCmdOfEntity, numEntityCmds);
    }

    removeComponentQueue.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
World::AddStagedComponentsToEntity(Entity entity, AddStagedComponentCommand* cmds, SizeT numCmds)
{
    MemDb::TableSignature signature;

    if (this->HasInstance(entity))
    {
        EntityMapping const mapping = this->GetEntityMapping(entity);
        MemDb::Table const& tbl = this->db->GetTable(mapping.table);
        signature = tbl.GetSignature();
    }

    SizeT i;
    for (i = 0; i < numCmds; i++)
    {
        auto const* cmd = cmds + i;
        signature.SetBit(cmd->componentId);
    }

    MemDb::TableId newCategoryId = this->db->FindTable(signature);
    if (newCategoryId == MemDb::InvalidTableId)
    {
        EntityTableCreateInfo info;

        IndexT i = 0;
        if (this->HasInstance(entity))
        {
            EntityMapping const mapping = this->GetEntityMapping(entity);
            MemDb::Table const& tbl = this->db->GetTable(mapping.table);
            Util::Array<Game::ComponentId> const& cols = tbl.GetAttributes();
            info.components.SetSize(cols.Size() + numCmds);

            for (i = 0; i < cols.Size(); ++i)
            {
                info.components[i] = cols[i];
            }
        }
        else
        {
            info.components.SetSize(numCmds);
        }

        SizeT end = i + numCmds;
        IndexT cmdIndex = 0;
        for (; i < end; ++i, ++cmdIndex)
        {
            info.components[i] = cmds[cmdIndex].componentId;
        }

        newCategoryId = this->CreateEntityTable(info);
    }

    MemDb::RowId newInstance = this->Migrate(entity, newCategoryId);

    MemDb::Table& newTable = this->db->GetTable(newCategoryId);

    for (i = 0; i < numCmds; i++)
    {
        auto const* cmd = cmds + i;

        auto attrIndex = newTable.GetAttributeIndex(cmd->componentId);
        void* ptr = newTable.GetValuePointer(attrIndex, newInstance);
        Memory::Copy(cmd->data, ptr, cmd->dataSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::RemoveComponentsFromEntity(Entity entity, RemoveComponentCommand* cmds, SizeT numCmds)
{
    EntityMapping const mapping = this->GetEntityMapping(entity);
    MemDb::Table& tbl = this->db->GetTable(mapping.table);
    MemDb::TableSignature signature = this->db->GetTable(mapping.table).GetSignature();

    SizeT i;
    for (i = 0; i < numCmds; i++)
    {
        auto const* cmd = cmds + i;
#if NEBULA_DEBUG
        /*
        if (!signature.IsSet(cmd->componentId))
        {
            Util::String errorMsg = Util::String::Sprintf("Tried to remove component '%s' from an entity does not have the given component!", MemDb::AttributeRegistry::GetAttribute(cmd->componentId)->name.Value());
            n_error(errorMsg.AsCharPtr());
        }
        */
#endif
        signature.ClearBit(cmd->componentId);
    }

    MemDb::TableId newCategoryId = this->db->FindTable(signature);
    if (newCategoryId == MemDb::InvalidTableId)
    {
        EntityTableCreateInfo info;
        auto const& attributes = tbl.GetAttributes();
        info.components.SetSize(attributes.Size() - numCmds);
        IndexT cIndex = 0;
        for (IndexT i = 0; i < attributes.Size(); ++i)
        {
            IndexT k = 0;
            for (k = 0; k < numCmds; k++)
            {
                // check if the component should remain in the entity
                if (attributes[i] == cmds[k].componentId)
                    break;
            }
            if (k == numCmds) // keep the component, otherwise discard it
                info.components[cIndex++] = attributes[i];
        }

        newCategoryId = this->CreateEntityTable(info);
    }

    for (i = 0; i < numCmds; i++)
    {
        auto const* cmd = cmds + i;
        this->DecayComponent(cmd->componentId, mapping.table, tbl.GetAttributeIndex(cmd->componentId), mapping.instance);
    }

    this->Migrate(entity, newCategoryId);
}

//------------------------------------------------------------------------------
/**
*/
void*
World::GetInstanceBuffer(MemDb::TableId const tid, uint16_t partitionId, ComponentId const component)
{
    MemDb::Table& tbl = this->db->GetTable(tid);
    auto attrIndex = tbl.GetAttributeIndex(component);
#if NEBULA_DEBUG
    n_assert_fmt(
        attrIndex != MemDb::ColumnIndex::Invalid(),
        "GetInstanceBuffer: Entity table does not have component with id '%i'!\n",
        component.id
    );
#endif
    return tbl.GetBuffer(partitionId, attrIndex);
}

//------------------------------------------------------------------------------
/**
*/
void*
World::GetColumnData(MemDb::TableId const tid, uint16_t partitionId, MemDb::ColumnIndex const columnIndex)
{
    MemDb::Table& tbl = this->db->GetTable(tid);
    return tbl.GetBuffer(partitionId, columnIndex);
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::GetInstance(Entity entity) const
{
    return this->GetEntityMapping(entity).instance;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::TableId
World::CreateEntityTable(EntityTableCreateInfo const& info)
{
    MemDb::TableSignature const oldSignature(info.components);

    MemDb::TableId categoryId = this->db->FindTable(oldSignature);
    if (categoryId != MemDb::TableId::Invalid())
    {
        return categoryId;
    }

    constexpr ushort NUM_PROPS = 256;
    ComponentId components[NUM_PROPS];

    MemDb::TableCreateInfo tableInfo;
    tableInfo.name = info.name;
    tableInfo.numAttributes = 0;

    if (info.components[Game::Entity::Traits::fixed_column_index] != GetComponentId<Game::Entity>() &&
        info.components[Game::Position::Traits::fixed_column_index] != GetComponentId<Game::Position>() &&
        info.components[Game::Orientation::Traits::fixed_column_index] != GetComponentId<Game::Orientation>() &&
        info.components[Game::Scale::Traits::fixed_column_index] != GetComponentId<Game::Scale>())
    {
        // always have owner and transform as first columns
        components[Game::Entity::Traits::fixed_column_index] = GetComponentId<Entity>();
        components[Game::Position::Traits::fixed_column_index] = GetComponentId<Position>();
        components[Game::Orientation::Traits::fixed_column_index] = GetComponentId<Orientation>();
        components[Game::Scale::Traits::fixed_column_index] = GetComponentId<Scale>();

        tableInfo.numAttributes = 4 + info.components.Size();

        n_assert(tableInfo.numAttributes < NUM_PROPS);

        for (int i = 0; i < info.components.Size(); i++)
        {
            components[i + 4] = info.components[i];
        }
        tableInfo.attributeIds = components;
    }
    else
    {
        tableInfo.numAttributes = info.components.Size();
        n_assert(tableInfo.numAttributes < NUM_PROPS);
        tableInfo.attributeIds = &info.components[0];
    }

    // Create an instance table
    categoryId = this->db->CreateTable(tableInfo);

    // "Prefilter" the processors with the new table (insert the table in the cache that accepts it)
    this->pipeline.CacheTable(categoryId, this->db->GetTable(categoryId).GetSignature());

    return categoryId;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::AllocateInstance(Entity entity, MemDb::TableId table, Util::Blob const* const data)
{
    n_assert(this->IsValid(entity));
    n_assert(this->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity already registered!\n");
        return MemDb::InvalidRow;
    }

    MemDb::Table& tbl = this->db->GetTable(table);

    MemDb::RowId instance = tbl.AddRow();

    this->entityMap[entity.index] = {table, instance};

#if _DEBUG
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Entity>()) == Game::Entity::Traits::fixed_column_index);
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Position>()) == Game::Position::Traits::fixed_column_index);
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Orientation>()) == Game::Orientation::Traits::fixed_column_index);
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Scale>()) == Game::Scale::Traits::fixed_column_index);
#endif

    if (data != nullptr)
    {
        tbl.DeserializeInstance(*data, instance);
    }

    // Set the owner of this instance.
    Game::Entity* owners = (Game::Entity*)tbl.GetBuffer(instance.partition, Game::Entity::Traits::fixed_column_index);
    owners[instance.index] = entity;

    InitializeAllComponents(entity, table, instance);

    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
World::InitializeAllComponents(Entity entity, MemDb::TableId tableId, MemDb::RowId row)
{
    if (!this->componentInitializationEnabled)
        return;

    n_assert(this->IsValid(entity));

    MemDb::Table& tbl = this->db->GetTable(tableId);
    auto const& attributes = tbl.GetAttributes();
    for (IndexT i = 4; i < attributes.Size(); i++) // skip first four, since they're always owner and TRS
    {
        MemDb::Attribute* attr = MemDb::AttributeRegistry::GetAttribute(attributes[i]);
        ComponentInterface* cInterface;
        cInterface = static_cast<ComponentInterface*>(attr);

        if (cInterface->Init != nullptr)
        {
            void* data = tbl.GetValuePointer(i, row);
            cInterface->Init(this, entity, data);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::AllocateInstance(Entity entity, BlueprintId blueprint)
{
    n_assert(this->IsValid(entity));
    n_assert(this->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity already registered!\n");
        return MemDb::InvalidRow;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(this, blueprint);
    this->entityMap[entity.index] = mapping;

#if _DEBUG
    // make sure the first column in always owner
    n_assert(
        this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Entity>()) ==
        Game::Entity::Traits::fixed_column_index
    );
    n_assert(
        this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Position>()) ==
        Game::Position::Traits::fixed_column_index
    );
    n_assert(
        this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Orientation>()) ==
        Game::Orientation::Traits::fixed_column_index
    );
    n_assert(
        this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Scale>()) ==
        Game::Scale::Traits::fixed_column_index
    );
#endif

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetTable(mapping.table)
                               .GetBuffer(mapping.instance.partition, Game::Entity::Traits::fixed_column_index);
    owners[mapping.instance.index] = entity;

    InitializeAllComponents(entity, mapping.table, mapping.instance);

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::AllocateInstance(Entity entity, TemplateId templateId, bool performInitialize)
{
    n_assert(this->IsValid(entity));
    n_assert(this->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity instance already allocated!\n");
        return MemDb::InvalidRow;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(this, templateId);
    this->entityMap[entity.index] = mapping;

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetTable(mapping.table)
                               .GetBuffer(mapping.instance.partition, Game::Entity::Traits::fixed_column_index);
    owners[mapping.instance.index] = entity;

    if (performInitialize)
    {
        InitializeAllComponents(entity, mapping.table, mapping.instance);
    }

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
void
World::FinalizeAllocate(Entity entity)
{
    n_assert(this->IsValid(entity));
    EntityMapping& mapping = this->entityMap[entity.index];
    InitializeAllComponents(entity, mapping.table, mapping.instance);
}
//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateInstance(MemDb::TableId table, MemDb::RowId instance)
{
    n_assert(instance != MemDb::InvalidRow);

    // migrate managed properies to decay buffers so that we can allow the managers
    // to clean up any externally allocated resources.
    Util::Array<ComponentId> const& componentIds = this->db->GetTable(table).GetAttributes();
    const MemDb::ColumnIndex numColumns = componentIds.Size();
    for (MemDb::ColumnIndex column = 0; column < numColumns.id; column.id++)
    {
        Game::ComponentId component = componentIds[column.id];
        this->DecayComponent(component, table, column, instance);
    }

    this->db->GetTable(table).RemoveRow(instance);
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateInstance(Entity entity)
{
    n_assert(this->IsValid(entity));

    MemDb::TableId& table = this->entityMap[entity.index].table;
    MemDb::RowId& instance = this->entityMap[entity.index].instance;

    this->DeallocateInstance(table, instance);

    table = MemDb::InvalidTableId;
    instance = MemDb::InvalidRow;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::Migrate(Entity entity, MemDb::TableId newTableId)
{
    n_assert(this->IsValid(entity));
    n_assert(this->HasInstance(entity));
    EntityMapping mapping = this->GetEntityMapping(entity);
    MemDb::RowId newInstance =
        MemDb::Table::MigrateInstance(this->db->GetTable(mapping.table), mapping.instance, this->db->GetTable(newTableId), false);

    this->entityMap[entity.index] = {newTableId, newInstance};
    return newInstance;
}

//------------------------------------------------------------------------------
/**
    @param newInstances     Will be filled with the new instance ids in the destination table.
    @note   This assumes ALL entities in the entity array is of same table!
*/
void
World::Migrate(
    Util::Array<Entity> const& entities,
    MemDb::TableId fromTableId,
    MemDb::TableId newTableId,
    Util::FixedArray<MemDb::RowId>& newInstances
)
{
    if (newInstances.Size() != entities.Size())
    {
        newInstances.SetSize(entities.Size());
    }

    Util::Array<MemDb::RowId> instances;
    SizeT const num = entities.Size();
    instances.Reserve(num);

    for (auto entity : entities)
    {
        EntityMapping mapping = this->GetEntityMapping(entity);
#ifdef NEBULA_DEBUG
        n_assert(mapping.table == fromTableId);
#endif // NEBULA_DEBUG
        instances.Append(mapping.instance);
    }

    MemDb::Table::MigrateInstances(
        this->db->GetTable(fromTableId), instances, this->db->GetTable(newTableId), newInstances, false
    );

    // Defrag here to avoid entities existing in multiple tables
    this->Defragment(fromTableId);

    for (IndexT i = 0; i < num; i++)
    {
        this->entityMap[entities[i].index] = {newTableId, newInstances[i]};
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::MoveInstance(MemDb::Table::Partition* partition, MemDb::RowId from, MemDb::RowId to)
{
    Game::Entity fromEntity = ((Game::Entity*)partition->columns[Game::Entity::Traits::fixed_column_index])[from.index];
    Game::Entity toEntity = ((Game::Entity*)partition->columns[Game::Entity::Traits::fixed_column_index])[to.index];
    if (!this->IsValid(fromEntity))
    {
        // we need to add these instances new index to the to the freeids list, since it's been deleted.
        // the 'from' instance will be swapped with the 'to' instance, so we just add the 'to' id to the list;
        // and it will automatically be defragged
        partition->freeIds.Append(to.index);
    }
    else if (this->entityMap[fromEntity.index].table == this->entityMap[toEntity.index].table)
    {
        // just swap the instances
        this->entityMap[fromEntity.index].instance = to;
        this->entityMap[toEntity.index].instance = from;
    }
    else
    {
        // if the entities does not belong to the same table, only update the
        // instance of the one that has been moved.
        // This is most likely due to an entity migration
        this->entityMap[fromEntity.index].instance = to;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::Defragment(MemDb::TableId tableId)
{
    if (!this->db->IsValid(tableId))
        return;

#if NEBULA_DEBUG
    MemDb::ColumnIndex ownerColumnId = this->db->GetTable(tableId).GetAttributeIndex(GetComponentId<Game::Entity>());
    n_assert(ownerColumnId == 0);
#endif
    // defragment the table. Any instances that has been deleted will be swap'n'popped,
    // which means we need to update the entity mapping.
    // The move callback is signaled BEFORE the swap has happened.
    UNUSED(SizeT)
    numErased = this->db->GetTable(tableId).Defragment(
        [this](MemDb::Table::Partition* partition, MemDb::RowId from, MemDb::RowId to)
        {
            this->MoveInstance(partition, from, to);
        }
    );
}

//------------------------------------------------------------------------------
/**
*/
void
World::SetComponentValue(Game::Entity entity, Game::ComponentId component, void* value, uint64_t size)
{
#if NEBULA_DEBUG
    n_assert2(
        size == MemDb::AttributeRegistry::TypeSize(component),
        "SetComponent: Provided value's type is not the correct size for the given ComponentId."
    );
#endif
    EntityMapping mapping = this->GetEntityMapping(entity);
    byte* const ptr = (byte*)this->GetInstanceBuffer(mapping.table, mapping.instance.partition, component);
    byte* valuePtr = ptr + (mapping.instance.index * size);
    Memory::Copy(value, valuePtr, size);
}

//------------------------------------------------------------------------------
/**
*/
void
World::ReinitializeComponent(Game::Entity entity, Game::ComponentId component, void* value, uint64_t size)
{
#if NEBULA_DEBUG
    n_assert2(
        size == MemDb::AttributeRegistry::TypeSize(component),
        "ReinitializeComponent: Provided value's type is not the correct size for the given ComponentId."
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    MemDb::ColumnIndex const columnIndex = this->db->GetTable(mapping.table).GetAttributeIndex(component);

    // Decay the old component, this will allow managers to clean up any resources used before reinitializing
    this->DecayComponent(component, mapping.table, columnIndex, mapping.instance);

    byte* const ptr = (byte*)this->GetInstanceBuffer(mapping.table, mapping.instance.partition, component);
    byte* valuePtr = ptr + (mapping.instance.index * size);
    Memory::Copy(value, valuePtr, size);

    if (this->componentInitializationEnabled)
    {
        MemDb::Attribute* attr = MemDb::AttributeRegistry::GetAttribute(component);
        ComponentInterface* cInterface;
        cInterface = static_cast<ComponentInterface*>(attr);

        if (cInterface->Init != nullptr)
        {
            // run initialization function if it exists.
            cInterface->Init(this, entity, valuePtr);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::RenderDebug()
{
    ImGui::Text("World Hash: %s", Util::FourCC(this->hash.id).AsString().AsCharPtr());
    ImGui::Separator();
    static bool showProcessors = true;
    ImGui::Checkbox("Show Frame Pipeline", &showProcessors);
    if (showProcessors)
    {
        ImGui::Text("Pipeline (?):");
        if (ImGui::IsItemHovered())
        {
            //ImGui::SetTooltip("Processors are executed _after_ feature units for each event.");
            ImGui::SetTooltip("The pipeline that makes up the frame.\nThis consists of multiple frame events, possibly bundled "
                              "in frame batches.");
        }

        auto const events = this->pipeline.GetFrameEvents();

        for (IndexT i = 0; i < events.Size(); i++)
        {
            ImGui::TextColored({0.8f, 0.4f, 0.8f, 1.0f}, "Event: %s", events[i]->name.Value());
            ImGui::SameLine();
            ImGui::Text(" | Order: %i", events[i]->order);

            auto const batches = events[i]->GetBatches();
            ImGui::Indent();
            for (IndexT b = 0; b < batches.Size(); b++)
            {
                auto const batch = batches[b];
                ImGui::TextColored({0.4f, 0.8f, 0.8f, 1.0f}, "Batch #%i", b);
                ImGui::SameLine();
                ImGui::Text(" | Order: %i", batch->order);
                ImGui::SameLine();
                ImGui::Text(" | Async: %s", batch->async ? "true" : "false");
                auto const processors = batch->GetProcessors();
                ImGui::Indent();
                for (IndexT p = 0; p < processors.Size(); p++)
                {
                    Game::Processor const* const processor = processors[p];
                    ImGui::TextColored({0.8f, 0.8f, 0.4f, 1.0f}, processor->name.AsCharPtr());
                    ImGui::SameLine();
                    ImGui::Text(" | Async: %s", processor->async ? "true" : "false");
                    ImGui::SameLine();
                    ImGui::TextColored({0.5, 0.5, 0.9f, 1.0f}, " | Filter : %i", processor->filter);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::Indent();
                        auto const compsInc = Game::ComponentsInFilter(processor->filter);
                        if (compsInc.Size() > 0)
                        {
                            ImGui::TextColored({0.1f, 0.8f, 0.1f, 1.0f}, "Includes entities with components:");
                            ImGui::BeginChild(1, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * compsInc.Size()));
                            {
                                for (auto c : compsInc)
                                {
                                    const char* componentName = MemDb::AttributeRegistry::GetAttribute(c)->name.Value();
                                    ImGui::Selectable(componentName);
                                }
                                ImGui::EndChild();
                            }
                        }
                        auto const compsEx = Game::ExcludedComponentsInFilter(processor->filter);
                        if (compsEx.Size() > 0)
                        {
                            ImGui::TextColored({0.8, 0.2, 0.2, 1.0f}, "Excludes entities with components:");
                            ImGui::BeginChild(2, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * compsEx.Size()));
                            {
                                for (auto c : compsEx)
                                {
                                    const char* componentName = MemDb::AttributeRegistry::GetAttribute(c)->name.Value();
                                    ImGui::Selectable(componentName);
                                }
                                ImGui::EndChild();
                            }
                        }
                        ImGui::Unindent();
                    }
                }
                ImGui::Unindent();
            }
            ImGui::Unindent();
        }
        /*
        auto PrintCallbackInfo = [](Game::World::CallbackInfo const& callback)
        {
            Game::ProcessorInfo const& info = Game::GameServer::Instance()->GetProcessorInfo(callback.handle);
            ImGui::Text(info.name.Value());
            ImGui::SameLine();
            ImGui::Text(" | Async: %s", info.async ? "true" : "false");
            ImGui::SameLine();
            ImGui::Text(" | Filter : %i", info.filter);
        };

        ImGui::TextDisabled("-- OnBeginFrame --");
        for (auto const& callback : this->onBeginFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnFrame --");
        for (auto const& callback : this->onFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnEndFrame --");
        for (auto const& callback : this->onEndFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnSave --");
        for (auto const& callback : this->onSaveCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnLoad --");
        for (auto const& callback : this->onLoadCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnActivate --");
        for (auto const& callback : this->onActivateCallbacks)
            PrintCallbackInfo(callback);
        */
        ImGui::Separator();
    }

    static bool listInactive = false;
    ImGui::Checkbox("List inactive instances", &listInactive);
    ImGui::Text("Entity map:");
    ImGui::BeginChild(
        "ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar
    );
    {
        for (uint entityIndex = 0; entityIndex < this->entityMap.Size(); entityIndex++)
        {
            Game::EntityMapping entityMapping = this->entityMap[entityIndex];
            
            if (!listInactive && (entityMapping.table == MemDb::InvalidTableId || entityMapping.instance == MemDb::InvalidRow))
                continue;

            ImGui::BeginGroup();
            ImGui::Text("[%i] ", entityIndex);
            ImGui::SameLine();
            ImGui::TextColored(
                {1, 0.3f, 0, 1},
                "tid:%i, partition: %i, index: %i",
                entityMapping.table,
                entityMapping.instance.partition,
                entityMapping.instance.index
            );
            if (entityMapping.table != MemDb::TableId::Invalid())
            {
                ImGui::TextDisabled("- %s", this->db->GetTable(entityMapping.table).name.Value());
                ImGui::EndGroup();
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextDisabled("- %s", this->db->GetTable(entityMapping.table).name.Value());
                    MemDb::TableId const table = entityMapping.table;
                    MemDb::RowId const row = entityMapping.instance;

                    bool prevDebugState = Game::ComponentInspection::Instance()->debug;
                    Game::ComponentInspection::Instance()->debug = true;
                    auto const& components = this->db->GetTable(table).GetAttributes();
                    for (auto component : components)
                    {
                        Game::ComponentInterface* cInterface =
                            (Game::ComponentInterface*)MemDb::AttributeRegistry::GetAttribute(component);
                        if (cInterface->GetNumFields() == 0)
                        {
                            // Type is flag type, just print the name, and then continue
                            ImGui::Text("_flag_: %s", MemDb::AttributeRegistry::GetAttribute(component)->name.Value());
                            ImGui::Separator();
                            continue;
                        }
                        else
                        {
                            ImGui::Text(MemDb::AttributeRegistry::GetAttribute(component)->name.Value());
                        }
                        SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
                        void* data = this->GetInstanceBuffer(table, row.partition, component);
                        data = (byte*)data + (row.index * typeSize);

                        void* ownerBuffer = this->GetInstanceBuffer(table, row.partition, Game::Entity::Traits::fixed_column_index);
                        ownerBuffer = (byte*)data + (row.index * sizeof(Game::Entity));
                        Game::Entity owner = *(Game::Entity*)ownerBuffer;

                        const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders |
                                                      ImGuiTableFlags_RowBg | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
                        ImGui::PushID(row.index + 0xF23);
                        if (ImGui::BeginTable(MemDb::AttributeRegistry::GetAttribute(component)->name.Value(), 2, flags))
                        {
                            ImGui::TableSetupColumn("FieldName", ImGuiTableColumnFlags_WidthFixed);
                            ImGui::TableSetupColumn("FieldValue", ImGuiTableColumnFlags_WidthStretch);

                            ImGui::TableNextRow();
                            bool commitChange = false;
                            
                            Game::ComponentInspection::DrawInspector(owner, component, data, &commitChange);
                            ImGui::EndTable();
                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Spacing();
                        }
                        ImGui::PopID();

                        ImGui::Separator();
                    }
                    Game::ComponentInspection::Instance()->debug = prevDebugState;
                    ImGui::EndTooltip();
                }
            }
            else
            {
                ImGui::EndGroup();
                ImGui::TextDisabled("- ");
            }
            ImGui::Separator();
        }
    }
    ImGui::EndChild();
}

//------------------------------------------------------------------------------
/**
*/
void
World::Override(World* src, World* dst)
{
    dst->blueprintToTableMap = src->blueprintToTableMap;
    dst->entityMap = src->entityMap;
    dst->numEntities = src->numEntities;
    dst->pool = src->pool;
    dst->db = nullptr;
    dst->db = MemDb::Database::Create();
    src->db->Copy(dst->db);

    if (src->componentInitializationEnabled == false && dst->componentInitializationEnabled)
    {
        // Initialize all component if the source db haven't already.
        Game::Filter filter = Game::FilterBuilder().Including<Game::Entity>().Build();
        Game::Dataset data = dst->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            Game::Entity* entities = (Game::Entity*)view.buffers[0];

            for (uint16_t i = 0; i < view.numInstances; ++i)
            {
                Game::Entity& entity = entities[i];
                entity.world = dst->worldId;
                MemDb::RowId row = {.partition = view.partitionId, .index = i};
                dst->InitializeAllComponents(entity, view.tableId, row);
            }
        }
        Game::DestroyFilter(filter);
    }

    dst->PrefilterProcessors();
}

//------------------------------------------------------------------------------
/**
*/
FramePipeline&
World::GetFramePipeline()
{
    return this->pipeline;
}

//------------------------------------------------------------------------------
/**
*/
void
World::MarkAsModified(Game::Entity entity)
{
    EntityMapping mapping = this->GetEntityMapping(entity);
    MemDb::Table& table = this->db->GetTable(mapping.table);
    MemDb::Table::Partition* partition = table.GetPartition(mapping.instance.partition);
    partition->modifiedRows.SetBit(mapping.instance.index);
}

//------------------------------------------------------------------------------
/**
    @returns    Dataset with category table views.

    @note       The category table view buffer can be NULL if the filter contains
                a non-typed/flag component.
*/
Dataset
World::Query(Filter filter)
{
    //#if NEBULA_ENABLE_PROFILING
    //    //N_COUNTER_INCR("Calls to Game::Query", 1);
    //    N_SCOPE_ACCUM(QueryTime, EntitySystem);
    //#endif
    Util::Array<MemDb::TableId> tids = this->db->Query(GetInclusiveTableMask(filter), GetExclusiveTableMask(filter));

    return this->Query(filter, tids);
}

//------------------------------------------------------------------------------
/**
*/
Dataset
World::Query(Filter filter, Util::Array<MemDb::TableId>& tids)
{
    return Game::Query(this->db, tids, filter);
}

//------------------------------------------------------------------------------
/**
*/
template <>
Game::Position
World::GetComponent(Entity entity)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Getting components from entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that position is always in column 1
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Position::Traits::fixed_column_index;
    Game::Position* ptr = (Game::Position*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    return *(ptr + mapping.instance.index);
}

//------------------------------------------------------------------------------
/**
*/
template <>
Game::Orientation
World::GetComponent(Entity entity)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Getting components from entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that orientation is always in column 2
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Orientation::Traits::fixed_column_index;
    Game::Orientation* ptr = (Game::Orientation*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    return *(ptr + mapping.instance.index);
}

//------------------------------------------------------------------------------
/**
*/
template <>
Game::Scale
World::GetComponent(Entity entity)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Getting components from entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that scale is always in column 3
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Scale::Traits::fixed_column_index;
    Game::Scale* ptr = (Game::Scale*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    return *(ptr + mapping.instance.index);
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
World::SetComponent(Entity entity, Game::Position const& value)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Settings components in arbitrary entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that position is always in column 1
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Position::Traits::fixed_column_index;
    Game::Position* ptr = (Game::Position*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    *(ptr + mapping.instance.index) = value;
}

//------------------------------------------------------------------------------�
/**
*/
template <>
void
World::SetComponent(Entity entity, Game::Orientation const& value)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Settings components in arbitrary entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that orientation is always in column 2
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Orientation::Traits::fixed_column_index;
    Game::Orientation* ptr = (Game::Orientation*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    *(ptr + mapping.instance.index) = value;
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
World::SetComponent(Entity entity, Game::Scale const& value)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Settings components in arbitrary entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that scale is always in column 3
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Scale::Traits::fixed_column_index;
    Game::Scale* ptr = (Game::Scale*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    *(ptr + mapping.instance.index) = value;
}

} // namespace Game
