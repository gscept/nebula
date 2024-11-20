//------------------------------------------------------------------------------
//  @file level.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "basegamefeature/level.h"
#include "io/ioserver.h"
#include "game/world.h"

#include "flat/game/level.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
Util::Array<Game::Entity>
PackedLevel::Instantiate() const
{
    Util::Array<Game::Entity> entities;

    for (IndexT i = 0; i < this->tables.Size(); i++)
    {
        EntityGroup const& dataTable = this->tables[i];
        MemDb::Table& table = this->world->GetDatabase()->GetTable(dataTable.dstTable);

        SizeT numRowsLeft = dataTable.numRows;

        MemDb::Table::Partition* partition = table.GetCurrentPartition();
        if (partition == nullptr || partition->numRows == MemDb::Table::Partition::CAPACITY)
            partition = table.NewPartition();

    
        // Create new partitions and fill them with data from table
        SizeT rowsProcessed = 0;
        while (numRowsLeft > 0)
        {
            SizeT numRows = Math::min(numRowsLeft, (SizeT)MemDb::Table::Partition::CAPACITY - (SizeT)partition->numRows);
            numRowsLeft -= (SizeT)MemDb::Table::Partition::CAPACITY - partition->numRows;

            SizeT const numColumns = table.GetAttributes().Size();
            SizeT byteOffset = 0;
            for (IndexT columnIndex = 0; columnIndex < numColumns; columnIndex++)
            {
                // TODO: maybe store this in the EntityGroup upon preloading.
                SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(table.GetAttributes()[columnIndex]);
                SizeT const numBytes = numRows * typeSize;
                ubyte* src = dataTable.columns + byteOffset + (rowsProcessed * typeSize);
                Memory::Copy(src, (byte*)partition->columns[columnIndex] + (partition->numRows * typeSize), numBytes);
                byteOffset += dataTable.numRows * typeSize;
            }

            rowsProcessed += numRows;

            for (uint16_t rowIndex = partition->numRows; rowIndex < partition->numRows + numRows; rowIndex++)
            {
                partition->validRows.SetBit(rowIndex);
                Game::Entity entity = this->world->AllocateEntityId();

                Game::EntityMapping& mapping = this->world->entityMap[entity.index];
                mapping.table = dataTable.dstTable;
                mapping.instance = {.partition = partition->partitionId, .index = rowIndex};

                // Set the owner of this instance.
                MemDb::Table& table = this->world->GetDatabase()->GetTable(mapping.table);
                Game::Entity* owners =
                    (Game::Entity*)table.GetBuffer(mapping.instance.partition, Game::Entity::Traits::fixed_column_index);
                owners[mapping.instance.index] = entity;

                entities.Append(entity);
                // TODO: could initialize all components of a specific type at the same time
                this->world->InitializeAllComponents(entity, mapping.table, mapping.instance);
            }

            partition->numRows += numRows;
            if (partition->numRows == MemDb::Table::Partition::CAPACITY)
                partition = table.NewPartition();
        }

        // update table numRows total
        table.SetNumRows(table.GetNumRows() + dataTable.numRows);
    }

    return entities;
}

} // namespace Game
