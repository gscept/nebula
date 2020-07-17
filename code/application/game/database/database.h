#pragma once
//------------------------------------------------------------------------------
/**
    Game::Database

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/arrayallocator.h"
#include "table.h"
#include "columndata.h"
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"

namespace Game
{

namespace Db
{

class Database : public Core::RefCounted
{
    __DeclareClass(Game::Db::Database);
public:
    Database();
    ~Database();

    TableId CreateTable(TableCreateInfo const& info);
    void DeleteTable(TableId table);
    bool IsValid(TableId table);
    
    bool HasColumn(TableId table, Column col);

    Column GetColumn(TableId table, ColumnId columnId);
    ColumnId GetColumnId(TableId table, Column column);

    ColumnId AddColumn(TableId table, Column column);

    IndexT AllocateRow(TableId table);
    void DeallocateRow(TableId table, IndexT row);

    /// Set all row values to default
    void SetToDefault(TableId table, IndexT row);

    /// Set an attribute value
    void Set(TableId table, ColumnId columnId, IndexT row, AttributeValue const& value);

    /// get number of rows in a table
    SizeT GetNumRows(TableId table);

    /// Get the column descriptors for a table
    Util::Array<Column> const& GetColumns(TableId table);

    /// Adds a custom state data column to table.
    template<typename TYPE>
    ColumnData<typename TYPE> AddStateColumn(TableId tid, Util::StringAtom name);

    /// gets a custom state data column from table.
    template<typename TYPE>
    const ColumnData<typename TYPE> GetStateColumn(TableId tid, StateDescriptor descriptor);

    /// returns a persistant array accessor
    template<typename ATTR>
    ColumnData<typename ATTR::TYPE> GetColumnData(TableId table);
    
    /// Get a persistant buffer. Only use this if you know what you're doing!
    void** GetPersistantBuffer(TableId table, ColumnId cid);
    
    /// retrieve a table.
    Table& GetTable(TableId tid);

    SizeT Defragment(TableId tid, std::function<void(InstanceId, InstanceId)> const& moveCallback);

    /// Adds a custom state data column to table.
    template<typename TYPE>
    StateDescriptor CreateStateDescriptor(Util::StringAtom name);

    /// Returns a state descriptor or Invalid if not registered
    StateDescriptor GetStateDescriptor(Util::StringAtom name);

    ColumnId AddState(TableId table, StateDescriptor state);

private:
    void EraseSwapIndex(Table& table, InstanceId instance);

    void GrowTable(TableId tid);

    void* AllocateColumn(TableId tid, Column column);
    void* AllocateState(TableId tid, StateDescription* desc);

    Ids::IdGenerationPool tableIdPool;

    // @note    Keep this a fixed size array, because we want to be able to keep persistent references to the tables, and their buffers within
    static constexpr uint32_t MAX_NUM_TABLES = 512;
    Table tables[MAX_NUM_TABLES];
    SizeT numTables = 0;

    Util::Array<StateDescription*> stateDescriptions;
    Util::Dictionary<Util::StringAtom, StateDescriptor> stateRegistry;
};

//------------------------------------------------------------------------------
/**
*/
inline void**
Database::GetPersistantBuffer(TableId table, ColumnId cid)
{
    n_assert(this->IsValid(table));
    Table& tbl = this->tables[Ids::Index(table.id)];
    void* ptr = &tbl.columns.Get<1>(cid.id);
    return &tbl.columns.Get<1>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ATTR>
inline ColumnData<typename ATTR::TYPE>
Database::GetColumnData(TableId table)
{
    n_assert(this->IsValid(table));
    Table& tbl = this->tables[Ids::Index(table.id)];
    ColumnId cid = this->GetColumnId(table, ATTR::Id());
    return ColumnData<ATTR::TYPE>(cid, &tbl.columns.Get<1>(cid.id), &tbl.numRows);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline ColumnData<TYPE>
Database::AddStateColumn(TableId tid, Util::StringAtom name)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    uint32_t col = table.states.Alloc();

    Table::ColumnBuffer& buffer = table.states.Get<1>(col);

    StateDescription* desc;
    StateDescriptor descriptor;
    if (this->stateRegistry.Contains(name))
    {
        descriptor = this->stateRegistry[name];
        desc = this->stateDescriptions[descriptor.id];
    }
    else
    {
        // Create state descriptor
        descriptor = this->CreateStateDescriptor<TYPE>(name);
        desc = this->stateDescriptions[descriptor.id];
    }

    buffer = this->AllocateState(tid, desc);
    table.states.Get<0>(col) = descriptor;

    return ColumnData<TYPE>(col, &table.states.Get<1>(col), &table.numRows, true);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline const ColumnData<TYPE>
Database::GetStateColumn(TableId tid, StateDescriptor descriptor)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    auto const& descriptors = table.states.GetArray<0>();
    for (int i = 0; i < descriptors.Size(); i++)
    {
        auto const& desc = descriptors[i];

        if (desc == descriptor)
        {
            return ColumnData<TYPE>(i, &table.states.Get<1>(i), &table.numRows, true);
        }
    }

    n_error("State does not exist in table!\n");

    return ColumnData<TYPE>(NULL, nullptr, nullptr, true);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline StateDescriptor
Database::CreateStateDescriptor(Util::StringAtom name)
{
    // setup a state description with the default values from the type
    StateDescription* desc = n_new(StateDescription(name, TYPE()));

    StateDescriptor descriptor = this->stateDescriptions.Size();
    this->stateDescriptions.Append(desc);
    this->stateRegistry.Add(name, descriptor);
    return descriptor;
}

//------------------------------------------------------------------------------
/**
*/
inline StateDescriptor
Database::GetStateDescriptor(Util::StringAtom name)
{
    IndexT index = this->stateRegistry.FindIndex(name);
    if (index != InvalidIndex)
    {
        return this->stateRegistry.ValueAtIndex(index);
    }
    
    return StateDescriptor::Invalid();
}

} // namespace Db

} // namespace Game
