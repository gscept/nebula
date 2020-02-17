#pragma once
//------------------------------------------------------------------------------
/**
    Game::Database

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/arrayallocator.h"
#include "table.h"
#include "valuetable.h"
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"

namespace Game
{

struct TableCreateInfo
{
    Util::String name;
    Util::FixedArray<Column> columns;
};

template<typename TYPE>
class ColumnData
{
public:
    ColumnData() : data(nullptr)
    {
    }
    ColumnData(ColumnId const columnId, void** ptrptr) : data((void**)ptrptr), cid(columnId)
    {
    }
    
    ~ColumnData() = default;
    TYPE& operator[](IndexT index)
    {
        n_assert(this->data != nullptr);
        n_assert(*this->data != nullptr);
        void* dataptr = *this->data;
        TYPE* ptr = reinterpret_cast<TYPE*>(dataptr);
        return (ptr[index]);
    }
private:
    void** data;
    ColumnId cid;
};

class Database : public Core::RefCounted
{
    __DeclareClass(Game::Database);
public:
    Database();
    ~Database();

    TableId CreateTable(TableCreateInfo const& info);
    void DeleteTable(TableId table);
    bool IsValid(TableId table);
    
    Column GetColumn(TableId table, ColumnId columnId);
    ColumnId GetColumnId(TableId table, Column column);

    ColumnId AddColumn(TableId table, Column column);

    IndexT AllocateRow(TableId table);
    void DeallocateRow(TableId table, IndexT row);

    /// Set all row values to default
    void SetToDefault(TableId table, IndexT row);

    SizeT GetNumRows(TableId table);
    
    /// Get the column descriptors for a table
    Util::Array<Column> const& GetColumns(TableId table);

    /// Adds a custom POD data column to table.
    template<typename TYPE>
    Game::ColumnData<typename TYPE> AddDataColumn(TableId tid)
    {
        static_assert(std::is_trivial<TYPE>(), "Type is not trivial!");
        Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
        
        uint32_t col = table.states.Alloc();

        Table::ColumnBuffer& buffer = table.columns.Get<1>(col);
        table.columns.Get<0>(col) = column;

        buffer = this->AllocateColumn(tid, column);

        return Game::ColumnData<ATTR::TYPE>(cid, &tbl.columns.Get<1>(cid.id));
    }

    template<typename ATTR>
    Game::ColumnData<typename ATTR::TYPE> GetColumnData(TableId table)
    {
        Game::Database::Table& tbl = this->tables.Get<0>(Ids::Index(table.id));
        ColumnId cid = this->GetColumnId(table, ATTR::Id());
        return Game::ColumnData<ATTR::TYPE>(cid, &tbl.columns.Get<1>(cid.id));
    }

private:
    void GrowTable(TableId tid);
    
    void* AllocateColumn(TableId tid, Column column);
    
    struct Table
    {
        using ColumnBuffer = void*;

        Util::StringAtom name;
        Util::ArrayAllocator<Column, ColumnBuffer> columns;
        /// These are additional columns that does not have a specific column type
        Util::Array<ColumnBuffer> states;
        uint32_t numRows = 0;
        uint32_t capacity = 128;
        uint32_t grow = 128;
        // Holds freed indices to be reused in the attribute table.
        Util::Array<IndexT> freeIds;
        
        static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::ObjectArrayHeap;
    };

    Ids::IdGenerationPool tableIdPool;
    Util::ArrayAllocator<Table> tables;
};

} // namespace Game
