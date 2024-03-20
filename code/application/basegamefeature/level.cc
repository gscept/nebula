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

        SizeT numNewPartitions = ((dataTable.numRows - 1) / MemDb::Table::Partition::CAPACITY) + 1;

        SizeT numRowsLeft = dataTable.numRows;

        // Create new partitions and fill them with data from table
        SizeT byteOffset = 0;
        for (IndexT p = 0; p < numNewPartitions; p++)
        {
            MemDb::Table::Partition* partition = table.NewPartition();

            SizeT numRows = Math::min(numRowsLeft, (SizeT)MemDb::Table::Partition::CAPACITY);
            numRowsLeft -= (SizeT)MemDb::Table::Partition::CAPACITY;

            partition->numRows = numRows;

            SizeT const numColumns = table.GetAttributes().Size();
            for (IndexT columnIndex = 0; columnIndex < numColumns; columnIndex++)
            {
                // TODO: maybe store this in the EntityGroup upon preloading.
                SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(table.GetAttributes()[columnIndex]);
                SizeT const numBytes = numRows * typeSize;
                Memory::Copy(dataTable.columns + byteOffset, partition->columns[columnIndex], numBytes);
                byteOffset += numBytes;
            }

            for (uint16_t rowIndex = 0; rowIndex < numRows; rowIndex++)
            {
                Game::Entity entity = this->world->AllocateEntity();

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
        }
    }

    return entities;
}

} // namespace Game
