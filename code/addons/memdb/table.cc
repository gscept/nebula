//------------------------------------------------------------------------------
//  @file table.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "table.h"
#include "attribute.h"
#include "attributeregistry.h"
#include "util/blob.h"

namespace MemDb
{



//------------------------------------------------------------------------------
/**
*/
void*
AllocateBuffer(Attribute* desc, SizeT capacity, SizeT numRows)
{
    n_assert(desc->defVal != nullptr);
    n_assert(desc->typeSize != 0);

    void* buffer = Memory::Alloc(Table::HEAP_MEMORY_TYPE, desc->typeSize * capacity);
    
    for (IndexT i = 0; i < numRows; ++i)
    {
        void* val = (char*)buffer + (i * desc->typeSize);
        Memory::Copy(desc->defVal, val, desc->typeSize);
    }

    return buffer;
}

//------------------------------------------------------------------------------
/**
*/
Table::~Table()
{
    this->Reset();
}

//------------------------------------------------------------------------------
/**
*/
Table::Partition*
Table::NewPartition()
{
    Partition* partition = nullptr;

    if (freePartitions.IsEmpty())
    {
        partition = new Partition();
        if (nullPartitions.IsEmpty())
        {
            partition->partitionId = this->partitions.Size();
            this->partitions.Append(partition);
        }
        else
        {
            uint16_t pid = nullPartitions.PopBack();
            this->partitions[pid] = partition;
            partition->partitionId = pid;
        }
        
        partition->table = this;

        for (IndexT col = 0; col < this->attributes.Size(); col++)
        {
            AttributeId const& attr = this->attributes[col];

            Attribute const* const desc = AttributeRegistry::GetAttribute(attr);
            partition->columns.Append(nullptr);
            if (desc->typeSize > 0)
            {
                Table::ColumnBuffer& buffer = partition->columns[col];
                buffer = AllocateBuffer(AttributeRegistry::GetAttribute(attr), partition->CAPACITY, partition->numRows);
            }
        }
    }
    else
    {
        partition = this->freePartitions.PopBack();
    }

    


    partition->next = this->firstActivePartition;
    if (this->firstActivePartition != nullptr)
    {
        this->firstActivePartition->previous = partition;
    }
    this->firstActivePartition = partition;

    this->numActivePartitions++;

    return partition;
}

//------------------------------------------------------------------------------
/**
*/
bool
Table::HasAttribute(AttributeId attribute) const
{
    return this->signature.IsSet(attribute);
}

//------------------------------------------------------------------------------
/**
*/
AttributeId
Table::GetAttributeId(ColumnIndex columnIndex) const
{
    return this->attributes[columnIndex.id];
}

//------------------------------------------------------------------------------
/**
*/
ColumnIndex
Table::GetAttributeIndex(AttributeId attribute) const
{
    n_assert(attribute != AttributeId::Invalid());
    IndexT index = this->columnRegistry.FindIndex(attribute);
    if (index != InvalidIndex)
        return this->columnRegistry.ValueAtIndex(attribute, index);
    return ColumnIndex::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<AttributeId> const&
Table::GetAttributes() const
{
    return this->attributes;
}

//------------------------------------------------------------------------------
/**
*/
TableSignature const&
Table::GetSignature() const
{
    return this->signature;
}

//------------------------------------------------------------------------------
/**
*/
ColumnIndex
Table::AddAttribute(AttributeId attribute, bool updateSignature)
{
    n_assert(attribute != AttributeId::Invalid());

    IndexT found = this->attributes.FindIndex(attribute);
    if (found != InvalidIndex)
    {
        n_printf("Warning: Adding multiple columns of same type to a table is not supported and will result in only "
                 "one column!\n");
        return found;
    }

    uint32_t const col = this->attributes.Size();
    this->attributes.Append(attribute);
    this->columnRegistry.Add(attribute, col);

    if (updateSignature)
    {
        TableSignature& signature = this->signature;
#if NEBULA_DEBUG
        // Bit should not be set if the attribute has not already been registered
        n_assert(!signature.IsSet(attribute));
#endif
        signature.FlipBit(attribute);
    }

    Attribute const* const desc = AttributeRegistry::GetAttribute(attribute);

    for (Partition* part : this->partitions)
    {
        part->columns.Append(nullptr);
        if (desc->typeSize > 0)
        {
            Table::ColumnBuffer& buffer = part->columns[col];
            buffer = AllocateBuffer(AttributeRegistry::GetAttribute(attribute), part->CAPACITY, part->numRows);
        }
        return col;
    }

    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
RowId
Table::AddRow()
{
    if (this->currentPartition == nullptr || this->currentPartition->numRows == this->currentPartition->CAPACITY)
    {
        this->currentPartition = NewPartition();
    }

    this->totalNumRows -= this->currentPartition->numRows;
    uint16_t index = this->currentPartition->AllocateRowIndex();
    this->totalNumRows += this->currentPartition->numRows;

    const SizeT numColumns = this->attributes.Size();
    for (IndexT i = 0; i < numColumns; ++i)
    {
        AttributeId attribute = this->attributes[i];
        Attribute* desc = AttributeRegistry::GetAttribute(attribute.id);

        void*& buf = this->currentPartition->columns[i];
        void* val = (char*)buf + ((size_t)index * desc->typeSize);
        Memory::Copy(desc->defVal, val, desc->typeSize);
    }

    return {this->currentPartition->partitionId, index};
}

//------------------------------------------------------------------------------
/**
*/
void
Table::RemoveRow(RowId row)
{
    Partition* part = this->partitions[row.partition];
    part->FreeIndex(row.index);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
Table::GetNumRows() const
{
    return this->totalNumRows;
}

//------------------------------------------------------------------------------
/**
*/
void
Table::SetToDefault(RowId row)
{
    Partition* part = this->partitions[row.partition];
    n_assert(row.index < part->numRows);

    const SizeT numAttributes = this->attributes.Size();
    for (IndexT i = 0; i < numAttributes; ++i)
    {
        AttributeId attribute = this->attributes[i];
        Attribute* desc = AttributeRegistry::GetAttribute(attribute.id);
        void*& buf = part->columns[i];
        void* val = (char*)buf + ((size_t)row.index * desc->typeSize);
        Memory::Copy(desc->defVal, val, desc->typeSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
SizeT
Table::Defragment(std::function<void(Partition*, MemDb::RowId, MemDb::RowId)> const& moveCallback)
{
    SizeT numErased = 0;

    uint16_t index;
    uint16_t lastIndex;

    Partition* part = this->firstActivePartition;
    while (part != nullptr)
    {
        // Very important that this is sorted, since we defragment by swapping values.
        // This means i.e. if we swap index 1 before 2, and index 2 is also in the
        // reeids array, we won't be able to clean it up because we cannot keep track if it's been moved.
        part->freeIds.QuickSort();

        // Pack arrays
        while (part->freeIds.Size() != 0)
        {
            index = part->freeIds.Back();
            part->freeIds.EraseBack();

            if (index >= part->numRows)
            {
                // This might happen if we've swapped out an instance that is also in the freeids array.
                // Just ignore it, since its new index should already be added to the array.
                continue;
            }

            lastIndex = part->numRows - 1;
            if (index != lastIndex)
                moveCallback(part, RowId {part->partitionId, lastIndex}, RowId {part->partitionId, index});
            part->EraseSwapIndex(index);
            ++numErased;
        }

        part->freeIds.Clear();

        part->modifiedRows.Clear();

        if (part->numRows == 0 && part->partitionId != this->currentPartition->partitionId)
        {
            // recycle partition
            if (part->next != nullptr)
                part->next->previous = part->previous;
            if (part->previous != nullptr)
                part->previous->next = part->next;
            Partition* nextPart = part->next;
            this->freePartitions.Append(part);

            part->validRows.Clear();
            part->modifiedRows.Clear();
            part->version++;

            this->numActivePartitions--;
            part = nextPart;
        }
        else
        {
            part = part->next;
        }
    }

    // Lazily erase n partitions per frame, so that we clean up memory at some point
    for (size_t i = 0; i < 2 && !this->freePartitions.IsEmpty(); i++)
    {
        Partition* part = this->freePartitions.PopBack();
        n_assert(part != nullptr);

        uint16_t pid = part->partitionId;
        this->partitions[pid] = nullptr;
        delete part;
        this->nullPartitions.Append(pid);
        if (this->freePartitions.IsEmpty())
        {
            // free up some memory once we hit zero free partitions in our buffer
            this->freePartitions.Free();
            break;
        }
    }

    this->totalNumRows -= numErased;
    return numErased;
}

//------------------------------------------------------------------------------
/**
*/
void
Table::Clean()
{
    for (IndexT i = 0; i < this->partitions.Size(); i++)
    {
        if (this->partitions[i] != nullptr)
        {
            this->partitions[i]->numRows = 0;
            this->partitions[i]->freeIds.Clear();
        }
    }
    this->currentPartition = this->partitions[0];
    this->totalNumRows = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
Table::Reset()
{
    for (IndexT i = 0; i < this->partitions.Size(); i++)
    {
        Partition* part = this->partitions[i];
        for (auto& col : part->columns)
        {
            if (col != nullptr)
            {
                Memory::Free(Table::HEAP_MEMORY_TYPE, col);
                col = nullptr;
            }
        }
    }
    this->partitions.Reset();
    this->currentPartition = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
Table::Partition*
Table::GetFirstActivePartition()
{
    return this->firstActivePartition;
}

//------------------------------------------------------------------------------
/**
*/
uint16_t
Table::GetNumActivePartitions()
{
    return this->numActivePartitions;
}

//------------------------------------------------------------------------------
/**
*/
uint16_t
Table::GetNumPartitions()
{
    return this->partitions.Size();
}

//------------------------------------------------------------------------------
/**
*/
Table::Partition*
Table::GetPartition(uint16_t partitionId)
{
    n_assert(partitionId < this->partitions.Size());
    return this->partitions[partitionId];
}

//------------------------------------------------------------------------------
/**
*/
void*
Table::GetValuePointer(ColumnIndex cid, RowId row)
{
    n_assert(cid != ColumnIndex::Invalid());
    n_assert(this->partitions[row.partition] != nullptr);

    Partition* part = this->partitions[row.partition];
    AttributeId attribute = this->attributes[cid.id];
    Attribute* desc = MemDb::AttributeRegistry::GetAttribute(attribute);
    if (desc->typeSize > 0)
        return ((byte*)part->columns[cid.id]) + (desc->typeSize * row.index);

    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void*
Table::GetBuffer(uint16_t partition, ColumnIndex cid)
{
    n_assert(this->partitions[partition] != nullptr);
    return this->partitions[partition]->columns[cid.id];
}

//------------------------------------------------------------------------------
/**
*/
Util::Blob
Table::SerializeInstance(RowId row)
{
    Util::Blob blob;
    Partition* part = this->partitions[row.partition];
    n_assert(part != nullptr);

    // initial size adds room for n amount of pids
    uint32_t instanceSize = this->attributes.Size() * sizeof(AttributeId);
    for (uint32_t i = 0; i < this->attributes.Size(); i++)
    {
        AttributeId const attribute = this->attributes[i];
        SizeT const typeSize = AttributeRegistry::TypeSize(attribute);
        instanceSize += typeSize;
    }
    blob.Reserve(instanceSize);

    uint32_t offset = 0;
    for (uint32_t i = 0; i < this->attributes.Size(); i++)
    {
        AttributeId const attribute = this->attributes[i];
        blob.SetChunk(&attribute, sizeof(AttributeId), offset);
        offset += sizeof(AttributeId);
        SizeT const typeSize = AttributeRegistry::TypeSize(attribute);
        byte const* const valuePtr = (byte*)part->columns[i] + (row.index * (size_t)typeSize);
        blob.SetChunk(valuePtr, typeSize, offset);
        offset += typeSize;
    }

    return blob;
}

//------------------------------------------------------------------------------
/**
*/
void
Table::DeserializeInstance(Util::Blob const& data, RowId row)
{
    Partition* part = this->partitions[row.partition];
    n_assert(part != nullptr);
    n_assert(part->numRows > row.index);

    size_t bytesRead = 0;
    byte const* const basePtr = (byte*)data.GetPtr();
    byte const* ptr = basePtr;
    size_t const numBytes = data.Size();
    while ((size_t)(ptr - basePtr) < numBytes)
    {
        AttributeId const attribute = *reinterpret_cast<AttributeId const*>(ptr);
        ptr += sizeof(AttributeId);
        SizeT const typeSize = AttributeRegistry::TypeSize(attribute);
        IndexT const bucket = this->columnRegistry.FindIndex(attribute);
        if (bucket != InvalidIndex)
        {
            byte* valuePtr = (byte*)part->columns[this->columnRegistry.ValueAtIndex(attribute, bucket)] +
                             (row.index * (size_t)typeSize);
            Memory::Copy(ptr, valuePtr, typeSize);
        }
        ptr += typeSize;
    }
}

//------------------------------------------------------------------------------
/**
*/
RowId
Table::MigrateInstance(
    Table& src,
    RowId srcRow,
    Table& dst,
    bool defragment,
    std::function<void(Partition*, RowId, RowId)> const& moveCallback
)
{
    n_assert(src.tid != dst.tid);
    RowId dstRow = Table::DuplicateInstance(src, srcRow, dst);
    if (defragment)
    {
        Partition* srcPart = src.partitions[srcRow.partition];
        if (moveCallback != nullptr)
        {
            RowId lastRow = { .partition = srcPart->partitionId, .index = (uint16_t)(srcPart->numRows - 1)};
            moveCallback(srcPart, lastRow, srcRow);
        }
        srcPart->EraseSwapIndex(srcRow.index);
    }
    else
        src.RemoveRow(srcRow);
    return dstRow;
}

//------------------------------------------------------------------------------
/**
    @returns    New index/row in destination table
    @note       This might be destructive if the destination table is missing some of the source tables columns!
*/
RowId
Table::DuplicateInstance(Table const& src, RowId srcRow, Table& dst)
{
    RowId dstRow = dst.AddRow();

    Partition* srcPart = src.partitions[srcRow.partition];
    Partition* dstPart = dst.partitions[dstRow.partition];

    auto& buffers = srcPart->columns;
    auto const& dstAttrs = dst.attributes;
    auto& dstBuffers = dstPart->columns;

    const SizeT numDstAttrs = dst.attributes.Size();
    for (IndexT i = 0; i < numDstAttrs; ++i)
    {
        AttributeId attribute = dstAttrs[i];
        Attribute const* const desc = AttributeRegistry::GetAttribute(attribute.id);
        void*& dstBuf = dstBuffers[i];
        SizeT const byteSize = desc->typeSize;

        ColumnIndex const srcColId = src.GetAttributeIndex(attribute);
        if (srcColId != ColumnIndex::Invalid())
        {
            // Copy value from src
            void*& srcBuf = buffers[srcColId.id];
            Memory::Copy(
                (char*)srcBuf + ((size_t)byteSize * srcRow.index),
                (char*)dstBuf + ((size_t)byteSize * dstRow.index),
                byteSize
            );
        }
        else
        {
            // Set default value
            void* val = (char*)dstBuf + ((size_t)dstRow.index * desc->typeSize);
            Memory::Copy(desc->defVal, val, desc->typeSize);
        }
    }

    return dstRow;
}

//------------------------------------------------------------------------------
/**
*/
void
Table::MigrateInstances(
    Table& src,
    Util::Array<RowId> const& srcRows,
    Table& dst,
    Util::FixedArray<RowId>& dstRows,
    bool defragment,
    std::function<void(Partition*, IndexT, IndexT)> const& moveCallback
)
{
    n_assert(src.tid != dst.tid);
    Table::DuplicateInstances(src, srcRows, dst, dstRows);
    SizeT const num = srcRows.Size();
    if (defragment)
    {
        for (IndexT i = 0; i < num; i++)
        {
            RowId srcRow = srcRows[i];
            Partition* srcPart = src.partitions[srcRow.partition];
            if (moveCallback != nullptr)
            {
                IndexT lastIndex = srcPart->numRows - 1;
                moveCallback(srcPart, lastIndex, srcRow.index);
            }
            srcPart->EraseSwapIndex(srcRow.index);
        }
    }
    else
    {
        for (IndexT i = 0; i < num; i++)
            src.RemoveRow(srcRows[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Table::DuplicateInstances(Table& src, Util::Array<RowId> const& srcRows, Table& dst, Util::FixedArray<RowId>& dstRows)
{
    for (int i = 0; i < srcRows.Size(); i++)
    {
        dstRows[i] = dst.AddRow();
    }

    auto const& dstAttrs = dst.attributes;
    const SizeT numDstAttrs = dst.attributes.Size();
    for (IndexT i = 0; i < numDstAttrs; ++i)
    {
        AttributeId attribute = dstAttrs[i];
        Attribute const* const desc = AttributeRegistry::GetAttribute(attribute.id);
        SizeT const byteSize = desc->typeSize;
        ColumnIndex const srcColId = src.GetAttributeIndex(attribute);

        for (IndexT i = 0; i < srcRows.Size(); i++)
        {
            RowId srcRow = srcRows[i];
            RowId dstRow = dstRows[i];

            Partition* srcPart = src.partitions[srcRow.partition];
            Partition* dstPart = dst.partitions[dstRow.partition];

            auto& buffers = srcPart->columns;
            auto& dstBuffers = dstPart->columns;

            void*& dstBuf = dstBuffers[i];

            if (srcColId != ColumnIndex::Invalid())
            {
                // Copy value from src
                void*& srcBuf = buffers[srcColId.id];
                Memory::Copy(
                    (char*)srcBuf + ((size_t)byteSize * srcRow.index),
                    (char*)dstBuf + ((size_t)byteSize * dstRow.index),
                    byteSize
                );
            }
            else
            {
                // Set default value
                void* val = (char*)dstBuf + ((size_t)dstRow.index * desc->typeSize);
                Memory::Copy(desc->defVal, val, desc->typeSize);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Table::Partition::~Partition()
{
    for (auto& col : this->columns)
    {
        if (col != nullptr)
        {
            Memory::Free(Table::HEAP_MEMORY_TYPE, col);
            col = nullptr;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
uint16_t
Table::Partition::AllocateRowIndex()
{
    n_assert(this->numRows < this->CAPACITY);

    IndexT index;
    if (this->freeIds.Size() > 0)
    {
        index = this->freeIds.Back();
        this->freeIds.EraseBack();
    }
    else
    {
        index = this->numRows;
        this->numRows++;
    }

#if _DEBUG
    n_assert(index >= 0 && index <= 0xFFFF);
#endif

    this->validRows.SetBit(index);
    return (uint16_t)index;
}

//------------------------------------------------------------------------------
/**
*/
void
Table::Partition::FreeIndex(uint16_t instance)
{
    n_assert(instance < this->numRows);
    this->validRows.ClearBit(instance);
    this->freeIds.Append(instance);
}

//------------------------------------------------------------------------------
/**
*/
void
Table::Partition::EraseSwapIndex(uint16_t instance)
{
    Util::Array<AttributeId> const& attributes = this->table->attributes;
    // Swap the element with the last element, and decrement size of array.
    uint32_t const end = this->numRows - 1;

    if (end != instance)
    {
        // erase swap index in column buffers
        const SizeT numColumns = this->columns.Size();
        for (IndexT i = 0; i < numColumns; ++i)
        {
            AttributeId descriptor = attributes[i];
            Attribute* desc = AttributeRegistry::GetAttribute(descriptor.id);
            void*& buf = this->columns[i];
            const SizeT byteSize = desc->typeSize;
            Memory::Copy((char*)buf + ((size_t)byteSize * end), (char*)buf + ((size_t)byteSize * instance), byteSize);
        }
    }

    this->validRows.SetBitIf(instance, (uint64_t)this->validRows.IsSet(end));

    n_assert(this->numRows > 0);
    this->numRows--;
}

} // namespace MemDb
