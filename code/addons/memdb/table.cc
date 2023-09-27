//------------------------------------------------------------------------------
//  @file table.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "table.h"
#include "componentdescription.h"
#include "typeregistry.h"

namespace MemDb
{

//------------------------------------------------------------------------------
/**
*/
void*
AllocateBuffer(ComponentDescription* desc, SizeT capacity, SizeT numRows)
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
bool
_Table::HasAttribute(AttributeId attribute) const
{
    return this->signature.IsSet(attribute);
}

//------------------------------------------------------------------------------
/**
*/
AttributeId
_Table::GetAttributeId(ColumnIndex columnIndex) const
{
    return this->attributes[columnIndex.id];
}

//------------------------------------------------------------------------------
/**
*/
ColumnIndex
_Table::GetAttributeIndex(AttributeId attribute) const
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
_Table::GetAttributes() const
{
    return this->attributes;
}

//------------------------------------------------------------------------------
/**
*/
TableSignature const&
_Table::GetSignature() const
{
    return this->signature;
}

//------------------------------------------------------------------------------
/**
*/
ColumnIndex
_Table::AddAttribute(AttributeId attribute, bool updateSignature)
{
    n_assert(attribute != AttributeId::Invalid());

    IndexT found = this->attributes.FindIndex(attribute);
    if (found != InvalidIndex)
    {
        n_printf("Warning: Adding multiple columns of same type to a table is not supported and will result in only "
                 "one column!\n");
        return found;
    }

    this->attributes.Append(attribute);

    if (updateSignature)
    {
        TableSignature& signature = this->signature;
#if NEBULA_DEBUG
        // Bit should not be set if the component has not already been registered
        n_assert(!signature.IsSet(attribute));
#endif
        signature.FlipBit(attribute);
    }

    ComponentDescription const* const desc = TypeRegistry::GetDescription(attribute);
    if (desc->typeSize > 0)
    {
        uint32_t col = -1;
        for (Partition* part : this->partitions)
        {
            col = part->columns.Alloc();

            this->columnRegistry.Add(attribute, col);

            Table::ColumnBuffer& buffer = part->columns.Get<1>(col);
            part->columns.Get<0>(col) = attribute;
            buffer = AllocateBuffer(TypeRegistry::GetDescription(attribute), part->capacity, part->numRows);
        }
        return col;
    }

    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
RowId
_Table::AddRow()
{
    if (this->currentPartition->numRows == this->currentPartition->capacity)
    {
        if (this->vacantPartitions.IsEmpty())
        {
            this->currentPartition = new Partition();
            this->currentPartition->partitionIndex = this->partitions.Size();
            this->partitions.Append(currentPartition);
        }
        else
        {
            uint16_t index = this->vacantPartitions.Dequeue();
            this->currentPartition = this->partitions[index];
        }
    }

    uint16_t index = this->currentPartition->AllocateRowIndex();

    const SizeT numColumns = this->attributes.Size();
    for (IndexT i = 0; i < numColumns; ++i)
    {
        ComponentId descriptor = this->currentPartition->columns.Get<0>(i);
        ComponentDescription* desc = TypeRegistry::GetDescription(descriptor.id);

        void*& buf = this->currentPartition->columns.Get<1>(i);
        void* val = (char*)buf + ((size_t)index * desc->typeSize);
        Memory::Copy(desc->defVal, val, desc->typeSize);
    }

    return {this->currentPartition->partitionIndex, index};
}

//------------------------------------------------------------------------------
/**
*/
uint16_t
_Table::Partition::AllocateRowIndex()
{
    n_assert(this->numRows < this->capacity);

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

    return (uint16_t)index;
}

} // namespace MemDb
