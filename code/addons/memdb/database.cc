//------------------------------------------------------------------------------
//  database.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "database.h"
#include "io/binaryreader.h"
#include "io/binarywriter.h"
#include "io/memorystream.h"

namespace MemDb
{

__ImplementClass(MemDb::Database, 'MmDb', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
void
Dataset::Validate()
{
    for (IndexT i = 0; i < this->tables.Size();)
    {
        auto& view = this->tables[i];
        if (!db->IsValid(view.tid))
        {
            this->tables.EraseIndexSwap(i);
            continue;
        }

        view.numInstances = db->GetNumRows(view.tid);
        i++;
    }
}

//------------------------------------------------------------------------------
/**
*/
Database::Database()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Database::~Database()
{
    for (IndexT tableIndex = 0; tableIndex < this->MAX_NUM_TABLES; ++tableIndex)
    {
        if (this->IsValid(this->tables[tableIndex].tid))
            this->DeleteTable(this->tables[tableIndex].tid);
    }
}

//------------------------------------------------------------------------------
/**
*/
TableId
Database::CreateTable(TableCreateInfo const& info)
{
    TableId id;
    this->tableIdPool.Allocate(id.id);
    n_assert2(Ids::Index(id.id) < MAX_NUM_TABLES,
              "Tried to allocate more tables than currently allowed! Increase MAX_NUM_TABLES if this keeps occuring!\n");

    Table& table = this->tables[Ids::Index(id.id)];
    // Make sure we don't use any old data
    table = Table();
    table.tid = id;
    table.name = info.name;

    const SizeT numColumns = info.numProperties;
    for (IndexT i = 0; i < numColumns; i++)
    {
        this->AddColumn(id, info.properties[i], false);
    }

    TableSignature& signature = this->tableSignatures[Ids::Index(id.id)];
    signature = TableSignature(info.properties, info.numProperties);

    this->numTables = (Ids::Index(id.id) + 1 > this->numTables ? Ids::Index(id.id) + 1 : this->numTables);

    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
Database::DeleteTable(TableId tid)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];
    const SizeT numColumns = table.columns.Size();
    for (IndexT i = 0; i < numColumns; ++i)
    {
        PropertyId descriptor = table.columns.Get<0>(i);
        void*& buf = table.columns.Get<1>(i);
        if (buf != nullptr)
        {
            Memory::Free(Table::HEAP_MEMORY_TYPE, buf);
            buf = nullptr;
        }
    }
    this->tableIdPool.Deallocate(tid.id);
}

//------------------------------------------------------------------------------
/**
*/
bool
Database::IsValid(TableId table) const
{
    return this->tableIdPool.IsValid(table.id);
}

//------------------------------------------------------------------------------
/**
*/
bool
Database::HasProperty(TableId table, PropertyId col)
{
    n_assert(this->IsValid(table));
    return this->tableSignatures[Ids::Index(table.id)].IsSet(col);
}

//------------------------------------------------------------------------------
/**
*/
PropertyId
Database::GetPropertyId(TableId table, ColumnIndex columnId)
{
    n_assert(this->IsValid(table));
    return this->tables[Ids::Index(table.id)].columns.Get<0>(columnId.id);
}

//------------------------------------------------------------------------------
/**
*/
ColumnIndex
Database::GetColumnId(TableId table, PropertyId descriptor)
{
    n_assert(this->IsValid(table));
    n_assert(descriptor != PropertyId::Invalid());
    auto& reg = this->tables[Ids::Index(table.id)].columnRegistry;
    IndexT index = reg.FindIndex(descriptor);
    if (index != InvalidIndex)
        return reg.ValueAtIndex(descriptor, index);
    return ColumnIndex::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
IndexT
Database::AllocateRow(TableId tid)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    IndexT index = this->AllocateRowIndex(tid);

    const SizeT numColumns = table.columns.Size();
    for (IndexT i = 0; i < numColumns; ++i)
    {
        PropertyId descriptor = table.columns.Get<0>(i);
        PropertyDescription* desc = TypeRegistry::GetDescription(descriptor.id);

        void*& buf = table.columns.Get<1>(i);
        void* val = (char*)buf + ((size_t)index * desc->typeSize);
        Memory::Copy(desc->defVal, val, desc->typeSize);
    }
    return index;
}

//------------------------------------------------------------------------------
/**
*/
void
Database::DeallocateRow(TableId tid, IndexT row)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];
    n_assert(row < table.numRows);
    table.freeIds.InsertSorted(row);
}

//------------------------------------------------------------------------------
/**
*/
void
Database::SetToDefault(TableId tid, IndexT row)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];
    n_assert(row < table.numRows);

    const SizeT numColumns = table.columns.Size();
    for (IndexT i = 0; i < numColumns; ++i)
    {
        PropertyId descriptor = table.columns.Get<0>(i);
        PropertyDescription* desc = TypeRegistry::GetDescription(descriptor.id);
        void*& buf = table.columns.Get<1>(i);
        void* val = (char*)buf + ((size_t)row * desc->typeSize);
        Memory::Copy(desc->defVal, val, desc->typeSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
SizeT
Database::GetNumRows(TableId table) const
{
    n_assert(this->IsValid(table));
    return this->tables[Ids::Index(table.id)].numRows;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<PropertyId> const&
Database::GetColumns(TableId tid)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];
    auto& colTypes = table.columns.GetArray<0>();
    return colTypes;
}

//------------------------------------------------------------------------------
/**
    @returns    New index/row in destination table
    @note       This might be destructive if the destination table is missing some of the source tables columns!
    @note       This is an instant erase swap on src table, which means any external references to rows (instance ids) will be invalidated!
*/
IndexT
Database::MigrateInstance(TableId srcTid, IndexT srcRow, TableId dstTid, bool defragment, std::function<void(IndexT, IndexT)> const& moveCallback)
{
    n_assert(srcTid != dstTid);
    IndexT dstRow = this->DuplicateInstance(srcTid, srcRow, dstTid);
    if (defragment)
    {
        Table& table = GetTable(srcTid);
        if (moveCallback != nullptr)
        {
            IndexT lastIndex = table.numRows - 1;
            moveCallback(lastIndex, srcRow);
        }
        this->EraseSwapIndex(table, srcRow);
    }
    else
        this->DeallocateRow(srcTid, srcRow);
    return dstRow;
}

//------------------------------------------------------------------------------
/**
    @returns    New index/row in destination table
    @note       This might be destructive if the destination table is missing some of the source tables columns!
    @note       This is an instant erase swap on src table, which means any external references to rows (instance ids) will be invalidated!
*/
IndexT
Database::MigrateInstance(TableId srcTid, IndexT srcRow, Ptr<Database> const& dstDb, TableId dstTid, bool defragment, std::function<void(IndexT, IndexT)> const& moveCallback)
{
    IndexT dstRow = this->DuplicateInstance(srcTid, srcRow, dstDb, dstTid);
    if (defragment)
    {
        Table& table = GetTable(srcTid);
        if (moveCallback != nullptr)
        {
            IndexT lastIndex = table.numRows - 1;
            moveCallback(lastIndex, srcRow);
        }
        this->EraseSwapIndex(table, srcRow);
    }
    else
        this->DeallocateRow(srcTid, srcRow);
    return dstRow;
}

//------------------------------------------------------------------------------
/**
    @returns    New index/row in destination table
    @note       This might be destructive if the destination table is missing some of the source tables columns!
*/
IndexT
Database::DuplicateInstance(TableId srcTid, IndexT srcRow, TableId dstTid)
{
    n_assert(this->IsValid(srcTid));
    n_assert(this->IsValid(dstTid));

    Table& src = this->tables[Ids::Index(srcTid.id)];
    Table& dst = this->tables[Ids::Index(dstTid.id)];

    IndexT dstRow = this->AllocateRowIndex(dstTid);

    auto& buffers = src.columns.GetArray<1>();
    auto const& dstCols = dst.columns.GetArray<0>();
    auto& dstBuffers = dst.columns.GetArray<1>();

    const SizeT numDstCols = dst.columns.Size();

    for (IndexT i = 0; i < numDstCols; ++i)
    {
        PropertyId descriptor = dstCols[i];
        PropertyDescription const* const desc = TypeRegistry::GetDescription(descriptor.id);
        void*& dstBuf = dstBuffers[i];
        SizeT const byteSize = desc->typeSize;

        ColumnIndex const srcColId = this->GetColumnId(srcTid, descriptor);
        if (srcColId != ColumnIndex::Invalid())
        {
            // Copy value from src
            void*& srcBuf = buffers[srcColId.id];
            Memory::Copy((char*)srcBuf + ((size_t)byteSize * srcRow), (char*)dstBuf + ((size_t)byteSize * dstRow), byteSize);
        }
        else
        {
            // Set default value
            void* val = (char*)dstBuf + ((size_t)dstRow * desc->typeSize);
            Memory::Copy(desc->defVal, val, desc->typeSize);
        }
    }

    return dstRow;
}

//------------------------------------------------------------------------------
/**
    @returns    New index/row in destination table
    @note       This might be destructive if the destination table is missing some of the source tables columns!
*/
IndexT
Database::DuplicateInstance(TableId srcTid, IndexT srcRow, Ptr<Database> const& dstDb, TableId dstTid)
{
    n_assert(this->IsValid(srcTid));
    n_assert(dstDb->IsValid(dstTid));

    Table& src = this->tables[Ids::Index(srcTid.id)];
    Table& dst = dstDb->tables[Ids::Index(dstTid.id)];

    IndexT dstRow = dstDb->AllocateRowIndex(dstTid);

    auto& buffers = src.columns.GetArray<1>();
    auto const& dstCols = dst.columns.GetArray<0>();
    auto& dstBuffers = dst.columns.GetArray<1>();

    const SizeT numDstCols = dst.columns.Size();

    for (IndexT i = 0; i < numDstCols; ++i)
    {
        PropertyId descriptor = dstCols[i];
        PropertyDescription const* const desc = TypeRegistry::GetDescription(descriptor.id);
        void*& dstBuf = dstBuffers[i];
        SizeT const byteSize = desc->typeSize;

        ColumnIndex const srcColId = this->GetColumnId(srcTid, descriptor);
        if (srcColId != ColumnIndex::Invalid())
        {
            // Copy value from src
            void*& srcBuf = buffers[srcColId.id];
            Memory::Copy((char*)srcBuf + ((size_t)byteSize * srcRow), (char*)dstBuf + ((size_t)byteSize * dstRow), byteSize);
        }
        else
        {
            // Set default value
            void* val = (char*)dstBuf + ((size_t)dstRow * desc->typeSize);
            Memory::Copy(desc->defVal, val, desc->typeSize);
        }
    }

    return dstRow;
}

//------------------------------------------------------------------------------
/**
    @param srcRows      Array of source table instances that should be moved.
    @param dstRows      Array reference that is filled with new indices/rows in destionation table
    @param defragment   Erase swap old instances straight away, or recycle the rows (leaving invalid instances in place)

    @note   This might be destructive if the destination table is missing some of the source tables columns!
    @note   This is an instant erase swap on src table, which means any external references to rows (instance ids) will be invalidated!
*/
void
Database::MigrateInstances(TableId srcTid, Util::Array<IndexT> const& srcRows, TableId dstTid, Util::FixedArray<IndexT>& dstRows, bool defragment, std::function<void(IndexT, IndexT)> const& moveCallback)
{
    n_assert(srcTid != dstTid);
    this->DuplicateInstances(srcTid, srcRows, dstTid, dstRows);
    const SizeT num = srcRows.Size();
    if (defragment)
    {
        Table& table = GetTable(srcTid);
        if (moveCallback != nullptr)
        {
            for (IndexT i = 0; i < num; i++)
            {
                if (moveCallback != nullptr)
                {
                    IndexT lastIndex = table.numRows - 1;
                    moveCallback(lastIndex, srcRows[i]);
                }
                this->EraseSwapIndex(table, srcRows[i]);
            }
        }
        else
        {
            for (IndexT i = 0; i < num; i++)
                this->EraseSwapIndex(table, srcRows[i]);
        }
    }
    else
    {
        for (IndexT i = 0; i < num; i++)
            this->DeallocateRow(srcTid, srcRows[i]);
    }
}

//------------------------------------------------------------------------------
/**
    @param srcRows  Array of source table instances that should be duplicated.
    @param dstRows  Array reference that is filled with new indices/rows in destionation table
    @note       This might be destructive if the destination table is missing some of the source tables columns!
*/
void
Database::DuplicateInstances(TableId srcTid, Util::Array<IndexT> const& srcRows, TableId dstTid, Util::FixedArray<IndexT>& dstRows)
{
    n_assert(this->IsValid(srcTid));
    n_assert(this->IsValid(dstTid));

    Table& src = this->tables[Ids::Index(srcTid.id)];
    Table& dst = this->tables[Ids::Index(dstTid.id)];

    SizeT const numRows = dstRows.Size();
    for (IndexT i = 0; i < numRows; i++)
    {
        dstRows[i] = this->AllocateRowIndex(dstTid);
    }

    auto& buffers = src.columns.GetArray<1>();
    auto const& dstCols = dst.columns.GetArray<0>();
    auto& dstBuffers = dst.columns.GetArray<1>();

    SizeT const numDstCols = dst.columns.Size();

    for (IndexT i = 0; i < numDstCols; ++i)
    {
        PropertyId descriptor = dstCols[i];
        PropertyDescription const* const desc = TypeRegistry::GetDescription(descriptor.id);
        void*& dstBuf = dstBuffers[i];
        SizeT const byteSize = desc->typeSize;

        ColumnIndex const srcColId = this->GetColumnId(srcTid, descriptor);
        if (srcColId != ColumnIndex::Invalid())
        {
            for (IndexT rowIndex = 0; rowIndex < numRows; rowIndex++)
            {
                // Copy value from src
                void*& srcBuf = buffers[srcColId.id];
                Memory::Copy((char*)srcBuf + ((size_t)byteSize * srcRows[rowIndex]), (char*)dstBuf + ((size_t)byteSize * dstRows[rowIndex]), byteSize);
            }
        }
        else
        {
            for (IndexT rowIndex = 0; rowIndex < numRows; rowIndex++)
            {
                // Set default value
                void* val = (char*)dstBuf + ((size_t)dstRows[rowIndex] * desc->typeSize);
                Memory::Copy(desc->defVal, val, desc->typeSize);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Table&
Database::GetTable(TableId tid)
{
    n_assert(this->IsValid(tid));
    return this->tables[Ids::Index(tid.id)];
}

//------------------------------------------------------------------------------
/**
*/
TableSignature const&
Database::GetTableSignature(TableId tid) const
{
    n_assert(this->IsValid(tid));
    return this->tableSignatures[Ids::Index(tid.id)];
}

//------------------------------------------------------------------------------
/**
*/
TableId
Database::FindTable(TableSignature const& signature) const
{
    for (IndexT tableIndex = 0; tableIndex < this->numTables; tableIndex++)
    {
        if (signature == this->tableSignatures[tableIndex])
            return this->tables[tableIndex].tid;
    }
    return TableId::Invalid();
}

//------------------------------------------------------------------------------
/**
    Defragments a table and call the move callback BEFORE moving elements.

    @param tid              Table identifier
    @param moveCallback     Callback when moving an instance. First parameter of callback is 'from index' and second is 'to index'

    @returns    Number of erased instances.
*/
SizeT
Database::Defragment(TableId tid, std::function<void(IndexT, IndexT)> const& moveCallback)
{
    n_assert(this->IsValid(tid));
    Table& table = this->GetTable(tid);

    SizeT numErased = 0;

    IndexT index;
    IndexT lastIndex;

    // Pack arrays
    while (table.freeIds.Size() != 0)
    {
        index = table.freeIds.Back();
        table.freeIds.EraseBack();

        if (index >= table.numRows)
        {
            // This might happen if we've swapped out an instance that is also in the freeids array.
            // Just ignore it, since its new index should already be added to the array.
            continue;
        }

        lastIndex = table.numRows - 1;
        moveCallback(lastIndex, index);
        this->EraseSwapIndex(table, index);
        ++numErased;
    }

    table.freeIds.Clear();

    return numErased;
}

//------------------------------------------------------------------------------
/**
    @note   This does not care for any external references. Everything is just reset.
*/
void
Database::Clean(TableId tid)
{
    if (this->IsValid(tid))
    {
        Table& table = this->GetTable(Ids::Index(tid.id));
        table.numRows = 0;
        table.freeIds.Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Database::Reset()
{
    for (int i = 0; i < MAX_NUM_TABLES; i++)
    {
        this->Clean(this->tables[i].tid);
    }
}

//------------------------------------------------------------------------------
/**
*/
IndexT
Database::AllocateRowIndex(TableId tid)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    IndexT index;
    if (table.freeIds.Size() > 0)
    {
        index = table.freeIds.Back();
        table.freeIds.EraseBack();
    }
    else
    {
        index = table.numRows;
        if (index >= table.capacity)
        {
            this->GrowTable(tid);
        }

        table.numRows++;
    }
    return index;
}

//------------------------------------------------------------------------------
/**
*/
void
Database::EraseSwapIndex(Table& table, IndexT instance)
{
    // Swap the element with the last element, and decrement size of array.
    auto const& cols = table.columns.GetArray<0>();
    auto& buffers = table.columns.GetArray<1>();
    uint32_t const end = table.numRows - 1;

    if (end != instance)
    {
        // erase swap index in column buffers
        const SizeT numColumns = table.columns.Size();
        for (IndexT i = 0; i < numColumns; ++i)
        {
            PropertyId descriptor = cols[i];
            PropertyDescription* desc = TypeRegistry::GetDescription(descriptor.id);
            void*& buf = buffers[i];
            const SizeT byteSize = desc->typeSize;
            Memory::Copy((char*)buf + ((size_t)byteSize * end), (char*)buf + ((size_t)byteSize * instance), byteSize);
        }
    }

    table.numRows--;
}

//------------------------------------------------------------------------------
/**
*/
void
Database::GrowTable(TableId tid)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    int oldCapacity = table.capacity;
    table.capacity += table.grow;
    table.grow *= 2;

    // Grow column buffers
    const SizeT numColumns = table.columns.Size();
    for (int i = 0; i < numColumns; ++i)
    {
        PropertyId descriptor = table.columns.Get<0>(i);
        PropertyDescription* desc = TypeRegistry::GetDescription(descriptor);
        void*& buf = table.columns.Get<1>(i);

        const SizeT byteSize = desc->typeSize;

        int newNumBytes = byteSize * table.capacity;
        void* newData = Memory::Alloc(Table::HEAP_MEMORY_TYPE, newNumBytes);

        Memory::Copy(buf, newData, (size_t)table.numRows * byteSize);
        Memory::Free(Table::HEAP_MEMORY_TYPE, buf);
        buf = newData;
    }
}

//------------------------------------------------------------------------------
/**
*/
void*
Database::AllocateBuffer(TableId tid, PropertyDescription* desc)
{
    n_assert(this->IsValid(tid));
    n_assert(desc->defVal != nullptr);
    n_assert(desc->typeSize != 0);

    Table& table = this->tables[Ids::Index(tid.id)];

    void* buffer = Memory::Alloc(Table::HEAP_MEMORY_TYPE, desc->typeSize * table.capacity);

    for (IndexT i = 0; i < table.numRows; ++i)
    {
        void* val = (char*)buffer + (i * desc->typeSize);
        Memory::Copy(desc->defVal, val, desc->typeSize);
    }

    return buffer;
}

//------------------------------------------------------------------------------
/**
    @param  updateSignature     Set to true if you are just casually adding a property to the table
                                IMPORTANT: If this is set to false, make sure you add it to the signature manually!

    @returns    Index of column in table, or InvalidIndex if it was not added
*/
ColumnIndex
Database::AddColumn(TableId tid, PropertyId descriptor, bool updateSignature)
{
    n_assert(this->IsValid(tid));
    n_assert(descriptor != PropertyId::Invalid());
    Table& table = this->tables[Ids::Index(tid.id)];

    IndexT found = table.columns.GetArray<0>().FindIndex(descriptor);
    if (found != InvalidIndex)
    {
        n_printf("Warning: Adding multiple columns of same type to a table is not supported and will result in only one column!\n");
        return found;
    }

    table.properties.Append(descriptor);

    if (updateSignature)
    {
        TableSignature& signature = this->tableSignatures[Ids::Index(tid.id)];
#if NEBULA_DEBUG
        // Bit should not be set if the property has not already been registered
        n_assert(!signature.IsSet(descriptor));
#endif
        signature.FlipBit(descriptor);
    }

    PropertyDescription const* const desc = TypeRegistry::GetDescription(descriptor);
    if (desc->typeSize > 0)
    {
        uint32_t col = table.columns.Alloc();

        table.columnRegistry.Add(descriptor, col);

        Table::ColumnBuffer& buffer = table.columns.Get<1>(col);
        table.columns.Get<0>(col) = descriptor;
        buffer = this->AllocateBuffer(tid, TypeRegistry::GetDescription(descriptor.id));

        return col;
    }

    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
Dataset
Database::Query(FilterSet const& filterset)
{
    Dataset set;

    IndexT potentialTables[MAX_NUM_TABLES];
    SizeT numValid = 0;

    for (IndexT tableIndex = 0; tableIndex < this->numTables; tableIndex++)
    {
        if (!MemDb::TableSignature::CheckBits(this->tableSignatures[tableIndex], filterset.Inclusive()))
            continue;

        if (filterset.Exclusive().IsValid())
        {
            if (MemDb::TableSignature::HasAny(this->tableSignatures[tableIndex], filterset.Exclusive()))
                continue;
        }

        potentialTables[numValid] = tableIndex;
        numValid++;
    }

    for (IndexT index = 0; index < numValid; index++)
    {
        IndexT tableIndex = potentialTables[index];
        Table const& tbl = this->tables[tableIndex];
        if (this->IsValid(tbl.tid))
        {
            if (tbl.numRows == 0) // ignore empty tables
                continue;

            Util::ArrayStack<void*, 16> buffers;
            buffers.Reserve(filterset.PropertyIds().Size());

            IndexT i = 0;
            for (auto attrid : filterset.PropertyIds())
            {
                ColumnIndex colId = this->GetColumnId(tbl.tid, attrid);
                if (colId != InvalidIndex) // There might be some non-typed properties
                    buffers.Append(tbl.columns.Get<1>(colId.id));
            }

            Dataset::View view;
            view.tid = tbl.tid;
            view.numInstances = this->GetNumRows(tbl.tid);
            view.buffers = std::move(buffers);
#ifdef NEBULA_DEBUG
            view.tableName = tbl.name.Value();
#endif
            set.tables.Append(std::move(view));
        }
    }

    set.db = this;
    return set;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<TableId>
Database::Query(TableSignature const& inclusive, TableSignature const& exclusive)
{
    Util::Array<TableId> result;

    for (IndexT tableIndex = 0; tableIndex < this->numTables; tableIndex++)
    {
        if (!MemDb::TableSignature::CheckBits(this->tableSignatures[tableIndex], inclusive))
            continue;

        if (exclusive.IsValid())
        {
            if (MemDb::TableSignature::HasAny(this->tableSignatures[tableIndex], exclusive))
                continue;
        }

        Table const& tbl = this->tables[Ids::Index(tableIndex)];
        if (this->IsValid(tbl.tid) && tbl.numRows > 0)
            result.Append(tbl.tid);
    }

    return result;
}

//------------------------------------------------------------------------------
/**
    Note that this function will override an old table if the signature exists
*/
void
Database::Copy(Ptr<MemDb::Database> const& dst) const
{
    for (int i = 0; i < MAX_NUM_TABLES; i++)
    {
        Table const& srcTable = this->tables[i];
        
        // only copy valid tables
        if (!this->IsValid(srcTable.tid))
            continue;
        

        TableId dstTid = dst->FindTable(this->tableSignatures[i]);
        if (dstTid == TableId::Invalid())
        {
            TableCreateInfo info;
            info.name = srcTable.name.Value();
            info.numProperties = srcTable.properties.Size();
            info.properties = srcTable.properties.Begin();
            dstTid = dst->CreateTable(info);
        }
        
        Table& dstTable = dst->GetTable(dstTid);
        dstTable.grow = srcTable.grow;
        dstTable.capacity = srcTable.capacity;
        dstTable.numRows = srcTable.numRows;
        dstTable.freeIds = srcTable.freeIds;

        for (int p = 0; p < srcTable.columns.Size(); p++)
        {
            void const* const srcBuffer = srcTable.columns.Get<1>(p);
            void*& dstBuffer = dstTable.columns.Get<1>(p);
            
            PropertyId const descriptor = srcTable.columns.Get<0>(p);
            PropertyDescription const* const desc = TypeRegistry::GetDescription(descriptor);
            uint32_t const byteSize = desc->typeSize;
            uint32_t const numBytes = byteSize * srcTable.capacity;
            
            // free the preallocated buffer
            Memory::Free(Table::HEAP_MEMORY_TYPE, dstBuffer);
            // allocate new buffer with the same capacity, grow etc. as the src tables
            dstBuffer = Memory::Alloc(Table::HEAP_MEMORY_TYPE, numBytes);

            Memory::Copy(srcBuffer, dstBuffer, (size_t)byteSize * (size_t)srcTable.numRows);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Blob
Database::SerializeInstance(TableId table, IndexT row)
{
    Util::Blob blob;
    Table const& tbl = this->GetTable(table);
    
    // initial size adds room for n amount of pids
    uint32_t instanceSize = tbl.columns.Size() * sizeof(PropertyId);
    for (uint32_t i = 0; i < tbl.columns.Size(); i++)
    {
        PropertyId const pid = tbl.columns.Get<0>(i);
        SizeT const typeSize = TypeRegistry::TypeSize(pid);
        instanceSize += typeSize;
    }
    blob.Reserve(instanceSize);

    uint32_t offset = 0;
    for (uint32_t i = 0; i < tbl.columns.Size(); i++)
    {
        PropertyId const pid = tbl.columns.Get<0>(i);
        blob.SetChunk(&pid, sizeof(PropertyId), offset);
        offset += sizeof(PropertyId);
        SizeT const typeSize = TypeRegistry::TypeSize(pid);
        byte const* const valuePtr = (byte*)tbl.columns.Get<1>(i) + (row * (size_t)typeSize);
        blob.SetChunk(valuePtr, typeSize, offset);
        offset += typeSize;
    }

    return blob;
}

//------------------------------------------------------------------------------
/**
*/
void
Database::DeserializeInstance(Util::Blob const& data, TableId table, IndexT row)
{
    n_assert(this->IsValid(table));
    Table& tbl = this->GetTable(table);
    n_assert(tbl.numRows > row && row != InvalidIndex);
    
    size_t bytesRead = 0;
    byte const* ptr = (byte*)data.GetPtr();
    size_t const numBytes = data.Size();
    while (bytesRead < numBytes)
    {
        PropertyId const pid = *reinterpret_cast<PropertyId const*>(ptr);
        ptr += sizeof(PropertyId);
        SizeT const typeSize = TypeRegistry::TypeSize(pid);
        IndexT const bucket = tbl.columnRegistry.FindIndex(pid);
        if (bucket != InvalidIndex)
        {
            byte* valuePtr = (byte*)tbl.columns.Get<1>(tbl.columnRegistry.ValueAtIndex(pid, bucket));
            Memory::Copy(ptr, valuePtr, typeSize);
        }
        ptr += typeSize;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Database::ForEachTable(std::function<void(TableId)> const& callback)
{
    for (IndexT tableIndex = 0; tableIndex < this->numTables; tableIndex++)
    {
        if (this->IsValid(this->tables[tableIndex].tid))
            callback(this->tables[tableIndex].tid);
    }
}

} // namespace MemDb
