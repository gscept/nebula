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
    n_assert2(
        Ids::Index(id.id) < MAX_NUM_TABLES,
        "Tried to allocate more tables than currently allowed! Increase MAX_NUM_TABLES if this keeps occuring!\n");

    Table& table = this->tables[Ids::Index(id.id)];
    // Make sure we don't use any old data
    table = Table();
    table.tid = id;
    table.name = info.name;

    const SizeT numColumns = info.numComponents;
    for (IndexT i = 0; i < numColumns; i++)
    {
        table.AddAttribute(info.components[i], false);
    }

    TableSignature& signature = this->tables[Ids::Index(id.id)].signature;
    signature = TableSignature(info.components, info.numComponents);

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
    table = Table();
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
Table&
Database::GetTable(TableId tid)
{
    n_assert(this->IsValid(tid));
    return this->tables[Ids::Index(tid.id)];
}

//------------------------------------------------------------------------------
/**
*/
TableId
Database::FindTable(TableSignature const& signature) const
{
    for (IndexT tableIndex = 0; tableIndex < this->numTables; tableIndex++)
    {
        if (signature == this->tables[tableIndex].signature)
            return this->tables[tableIndex].tid;
    }
    return TableId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
Database::Reset()
{
    for (int i = 0; i < MAX_NUM_TABLES; i++)
    {
        this->tables[i] = Table();
    }
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
        if (!MemDb::TableSignature::CheckBits(this->tables[tableIndex].signature, filterset.Inclusive()))
            continue;

        if (filterset.Exclusive().IsValid())
        {
            if (MemDb::TableSignature::HasAny(this->tables[tableIndex].signature, filterset.Exclusive()))
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
            if (tbl.totalNumRows == 0) // ignore empty tables
                continue;

            for (Table::Partition* partition : tbl.partitions)
            {
                Util::StackArray<void*, 16> buffers;
                buffers.Reserve(filterset.PropertyIds().Size());

                IndexT i = 0;
                for (auto attrid : filterset.PropertyIds())
                {
                    ColumnIndex colId = tbl.GetAttributeIndex(attrid);
                    if (colId != InvalidIndex) // There might be some non-typed components
                        buffers.Append(partition->columns[colId.id]);
                }

                Dataset::View view;
                view.tid = tbl.tid;
                view.numInstances = partition->numRows;
                view.buffers = std::move(buffers);
#ifdef NEBULA_DEBUG
                view.tableName = tbl.name.Value();
                view.tableName += ".partition_";
                view.tableName.AppendInt(partition->partitionId);
#endif
                set.tables.Append(std::move(view));
            }
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
        if (!MemDb::TableSignature::CheckBits(this->tables[tableIndex].signature, inclusive))
            continue;

        if (exclusive.IsValid())
        {
            if (MemDb::TableSignature::HasAny(this->tables[tableIndex].signature, exclusive))
                continue;
        }

        Table const& tbl = this->tables[Ids::Index(tableIndex)];
        if (this->IsValid(tbl.tid) && tbl.totalNumRows > 0)
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
    for (IndexT i = 0; i < MAX_NUM_TABLES; i++)
    {
        Table const& srcTable = this->tables[i];

        // only copy valid tables
        if (!this->IsValid(srcTable.tid))
            continue;

        TableId dstTid = dst->FindTable(this->tables[i].signature);
        if (dstTid == TableId::Invalid())
        {
            TableCreateInfo info;
            info.name = srcTable.name.Value();
            info.numComponents = srcTable.attributes.Size();
            info.components = srcTable.attributes.Begin();
            dstTid = dst->CreateTable(info);
        }

        Table& dstTable = dst->GetTable(dstTid);
        dstTable.totalNumRows = srcTable.totalNumRows;

        for (IndexT partitionIndex = 0; partitionIndex < srcTable.partitions.Size(); partitionIndex++)
        {
            Table::Partition* srcPart = srcTable.partitions[partitionIndex];
            if (srcPart == nullptr)
            {
                dstTable.partitions.Append(nullptr);
                continue;
            }
            Table::Partition* dstPart = dstTable.NewPartition();
            dstPart->partitionId = srcPart->partitionId;
            dstPart->numRows = srcPart->numRows;
            dstPart->modified = srcPart->modified;
            dstPart->modifiedRows = srcPart->modifiedRows;
            dstPart->freeIds = srcPart->freeIds;
            dstPart->table = &dstTable;
            dstPart->version = srcPart->version;

            dstTable.currentPartition = srcPart == srcTable.currentPartition ? dstPart : dstTable.currentPartition;

            for (IndexT p = 0; p < srcPart->columns.Size(); p++)
            {
                void const* const srcBuffer = srcPart->columns[p];
                void*& dstBuffer = dstPart->columns[p];

                AttributeId const descriptor = srcTable.attributes[p];
                AttributeDescription const* const desc = TypeRegistry::GetDescription(descriptor);
                uint32_t const byteSize = desc->typeSize;
                uint32_t const numBytes = byteSize * srcPart->CAPACITY;

                Memory::Copy(srcBuffer, dstBuffer, (size_t)byteSize * (size_t)srcPart->numRows);
            }
        }
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
