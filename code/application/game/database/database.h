#pragma once
//------------------------------------------------------------------------------
/**
    Game::Database

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/arrayallocator.h"
#include "table.h"
#include "columndata.h"
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"

namespace Game
{

namespace Db
{

struct TableCreateInfo
{
    Util::String name;
    Util::FixedArray<Column> columns;
};

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

    /// get number of rows in a table
    SizeT GetNumRows(TableId table);

    /// Get the column descriptors for a table
    Util::Array<Column> const& GetColumns(TableId table);

    /// Adds a custom POD data column to table.
    template<typename TYPE>
    ColumnData<typename TYPE> AddDataColumn(TableId tid);

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
    Util::ArrayAllocator<Table> tables;
};

//------------------------------------------------------------------------------
/**
*/
inline void**
Database::GetPersistantBuffer(TableId table, ColumnId cid)
{
    Table& tbl = this->tables.Get<0>(Ids::Index(table.id));
    return &tbl.columns.Get<1>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ATTR>
inline ColumnData<typename ATTR::TYPE>
Database::GetColumnData(TableId table)
{
    Table& tbl = this->tables.Get<0>(Ids::Index(table.id));
    ColumnId cid = this->GetColumnId(table, ATTR::Id());
    return ColumnData<ATTR::TYPE>(cid, &tbl.columns.Get<1>(cid.id), &tbl.numRows);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline ColumnData<TYPE>
Database::AddDataColumn(TableId tid)
{
    static_assert(std::is_standard_layout<TYPE>(), "Type is not standard layout!");
    static_assert(std::is_trivially_copyable<TYPE>(), "Type is not trivially copyable!");
    Table& table = this->tables.Get<0>(Ids::Index(tid.id));

    uint32_t col = table.states.Alloc();

    Table::ColumnBuffer& buffer = table.states.Get<1>(col);

    // setup a state description with the default values from the type
    StateDescription desc = StateDescription(TYPE());
    buffer = this->AllocateState(tid, desc);

    table.states.Get<0>(col) = std::move(desc);

    return ColumnData<TYPE>(col, &table.states.Get<1>(col), &table.numRows, true);
}

} // namespace Db

} // namespace Game
