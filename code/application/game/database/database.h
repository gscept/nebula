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
    ColumnData<typename TYPE> AddStateColumn(TableId tid);

    /// gets a custom state data column from table.
    template<typename TYPE>
    const ColumnData<typename TYPE> GetStateColumn(TableId tid);

    /// check if state column exists by id
    bool HasStateColumn(TableId tid, Util::FourCC id);

    /// returns a persistant array accessor
    template<typename ATTR>
    ColumnData<typename ATTR::TYPE> GetColumnData(TableId table);
    
    /// Get a persistant buffer. Only use this if you know what you're doing!
    void** GetPersistantBuffer(TableId table, ColumnId cid);
    
    /// retrieve a table.
    Table& GetTable(TableId tid);

    SizeT Defragment(TableId tid, std::function<void(InstanceId, InstanceId)> const& moveCallback);

private:
    void EraseSwapIndex(Table& table, InstanceId instance);

    void GrowTable(TableId tid);

    void* AllocateColumn(TableId tid, Column column);
    void* AllocateState(TableId tid, StateDescription const& desc);

    Ids::IdGenerationPool tableIdPool;

    // @note    Keep this a fixed size array, because we want to be able to keep persistent references to the tables, and their buffers within
    static constexpr uint32_t MAX_NUM_TABLES = 512;
    Table tables[MAX_NUM_TABLES];
    SizeT numTables = 0;
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
Database::AddStateColumn(TableId tid)
{
    static_assert(std::is_standard_layout<TYPE>(), "Type is not standard layout!");
    static_assert(std::is_trivially_copyable<TYPE>(), "Type is not trivially copyable!");
    static_assert(std::is_trivially_destructible<TYPE>(), "Type is not trivially destructible!");
    
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    uint32_t col = table.states.Alloc();

    Table::ColumnBuffer& buffer = table.states.Get<1>(col);

    // setup a state description with the default values from the type
    StateDescription desc = StateDescription(TYPE());
    buffer = this->AllocateState(tid, desc);

    table.states.Get<0>(col) = std::move(desc);

    return ColumnData<TYPE>(col, &table.states.Get<1>(col), &table.numRows, true);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline const ColumnData<TYPE>
Database::GetStateColumn(TableId tid)
{
    static_assert(std::is_standard_layout<TYPE>(), "Type is not standard layout!");
    static_assert(std::is_trivially_copyable<TYPE>(), "Type is not trivially copyable!");
    static_assert(std::is_trivially_destructible<TYPE>(), "Type is not trivially destructible!");

    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    auto const& descriptions = table.states.GetArray<0>();
    for (int i = 0; i < descriptions.Size(); i++)
    {
        auto const& desc = descriptions[i];

        if (desc.fourcc == TYPE::ID)
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
inline bool
Database::HasStateColumn(TableId tid, Util::FourCC id)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    auto const& descriptions = table.states.GetArray<0>();
    for (int i = 0; i < descriptions.Size(); i++)
    {
        auto const& desc = descriptions[i];

        if (desc.fourcc == id)
        {
            return true;
        }
    }

    return false;
}

} // namespace Db

} // namespace Game
