#pragma once
//------------------------------------------------------------------------------
/**
    MemDb::Database

    In-memory, minimally (memory) fragmented, non-relational database.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/arrayallocator.h"
#include "table.h"
#include "columnview.h"
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"
#include "columndescription.h"

namespace MemDb
{

struct FilterSet
{
    /// categories must include all attributes in this array
    Util::Array<ColumnDescriptor> inclusive;
    /// categories must NOT contain any attributes in this array
    Util::Array<ColumnDescriptor> exclusive;
};

struct Dataset
{
    /// A view into a category table.
    struct View
    {
        TableId tid;
        SizeT numInstances = 0;
        Util::ArrayStack<void*, 16> buffers;
    };

    /// the filter that has been used to attain this dataset
    FilterSet filter;

    /// views into the tables
    Util::Array<View> tables;
};

class Database : public Core::RefCounted
{
    __DeclareClass(MemDb::Database);
public:
    Database();
    ~Database();

    TableId CreateTable(TableCreateInfo const& info);
    void DeleteTable(TableId table);
    bool IsValid(TableId table) const;
    
    bool HasColumn(TableId table, ColumnDescriptor col);

    /// Returns a column descriptor
    ColumnDescriptor GetColumn(TableId table, ColumnId columnId);
    /// Returns a column descriptor or Invalid if not registered
    ColumnDescriptor GetColumn(Util::StringAtom name);

    ColumnId GetColumnId(TableId table, ColumnDescriptor column);

    ColumnId AddColumn(TableId table, ColumnDescriptor column);

    IndexT AllocateRow(TableId table);
    void DeallocateRow(TableId table, IndexT row);

    /// Set all row values to default
    void SetToDefault(TableId table, IndexT row);

    /// Set an attribute value
    //void Set(TableId table, ColumnId columnId, IndexT row, AttributeValue const& value);

    /// get number of rows in a table
    SizeT GetNumRows(TableId table) const;

    /// Get the column descriptors for a table
    Util::Array<ColumnDescriptor> const& GetColumns(TableId table);

    /// Adds a custom state data column to table.
    template<typename TYPE>
    ColumnView<typename TYPE> AddStateColumn(TableId tid, Util::StringAtom name);

    /// gets a custom state data column from table.
    template<typename TYPE>
    const ColumnView<typename TYPE> GetStateColumn(TableId tid, ColumnDescriptor descriptor);

    /// returns a persistant array accessor
    template<typename ATTR>
    ColumnView<typename ATTR::TYPE> GetColumnData(TableId table);
    
    /// Get a persistant buffer. Only use this if you know what you're doing!
    void** GetPersistantBuffer(TableId table, ColumnId cid);
    
    /// retrieve a table.
    Table& GetTable(TableId tid);

    SizeT Defragment(TableId tid, std::function<void(IndexT, IndexT)> const& moveCallback);

    /// Adds a custom state data column to table.
    template<typename TYPE>
    ColumnDescriptor CreateColumn(Util::StringAtom name);

    /// Query the database for a dataset of categories
    Dataset Query(FilterSet const& filterset);

private:
    void EraseSwapIndex(Table& table, IndexT instance);

    void GrowTable(TableId tid);

    void* AllocateBuffer(TableId tid, ColumnDescription* desc);

    Ids::IdGenerationPool tableIdPool;

    // @note    Keep this a fixed size array, because we want to be able to keep persistent references to the tables, and their buffers within
    static constexpr uint32_t MAX_NUM_TABLES = 512;
    Table tables[MAX_NUM_TABLES];
    SizeT numTables = 0;

    Util::Array<ColumnDescription*> columnDescriptions;
    Util::Dictionary<Util::StringAtom, ColumnDescriptor> columnRegistry;
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
inline ColumnView<typename ATTR::TYPE>
Database::GetColumnData(TableId table)
{
    n_assert(this->IsValid(table));
    Table& tbl = this->tables[Ids::Index(table.id)];
    ColumnId cid = this->GetColumnId(table, ATTR::Id());
    return ColumnView<ATTR::TYPE>(cid, &tbl.columns.Get<1>(cid.id), &tbl.numRows);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline ColumnView<TYPE>
Database::AddStateColumn(TableId tid, Util::StringAtom name)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    uint32_t col = table.states.Alloc();

    Table::ColumnBuffer& buffer = table.states.Get<1>(col);

    ColumnDescription* desc;
    ColumnDescriptor descriptor;
    if (this->columnRegistry.Contains(name))
    {
        descriptor = this->columnRegistry[name];
        desc = this->columnDescriptions[descriptor.id];
    }
    else
    {
        // Create state descriptor
        descriptor = this->CreateColumn<TYPE>(name);
        desc = this->columnDescriptions[descriptor.id];
    }

    buffer = this->AllocateState(tid, desc);
    table.states.Get<0>(col) = descriptor;

    return ColumnView<TYPE>(col, &table.states.Get<1>(col), &table.numRows, true);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline const ColumnView<TYPE>
Database::GetStateColumn(TableId tid, ColumnDescriptor descriptor)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    auto const& descriptors = table.states.GetArray<0>();
    for (int i = 0; i < descriptors.Size(); i++)
    {
        auto const& desc = descriptors[i];

        if (desc == descriptor)
        {
            return ColumnView<TYPE>(i, &table.states.Get<1>(i), &table.numRows, true);
        }
    }

    n_error("State does not exist in table!\n");

    return ColumnView<TYPE>(NULL, nullptr, nullptr, true);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline ColumnDescriptor
Database::CreateColumn(Util::StringAtom name)
{
    // setup a state description with the default values from the type
    ColumnDescription* desc = n_new(ColumnDescription(name, TYPE()));

    ColumnDescriptor descriptor = this->columnDescriptions.Size();
    this->columnDescriptions.Append(desc);
    this->columnRegistry.Add(name, descriptor);
    return descriptor;
}

//------------------------------------------------------------------------------
/**
*/
inline ColumnDescriptor
Database::GetColumn(Util::StringAtom name)
{
    IndexT index = this->columnRegistry.FindIndex(name);
    if (index != InvalidIndex)
    {
        return this->columnRegistry.ValueAtIndex(index);
    }
    
    return ColumnDescriptor::Invalid();
}

} // namespace MemDb
