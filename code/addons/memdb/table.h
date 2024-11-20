#pragma once
//------------------------------------------------------------------------------
/**
    @file   memdb/table.h

    Contains declarations for tables and table partitions.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/arrayallocator.h"
#include "util/fixedarray.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "util/hashtable.h"
#include "attributeid.h"
#include "tablesignature.h"
#include "util/bitfield.h"
#include "util/queue.h"
#include "ids/idgenerationpool.h"
#include "tableid.h"
#include <functional>

namespace MemDb
{

/// information for creating a table
struct TableCreateInfo
{
    /// name to be given to the table
    Util::String name;
    /// array of attributes the table should initially have
    AttributeId const* attributeIds;
    /// number of columns
    SizeT numAttributes;
};

//------------------------------------------------------------------------------
/**
    @class MemDb::Table

    @brief A table of columns (attributes) and rows. 
    
    @details A Table in MemDb is a collection of rows with attributes (columns)
    Tables are organized into Partitions, which help manage memory and improve performance.
    Each Partition has a fixed capacity and contains column buffers for storing attribute data.
    The Table maintains a list of all partitions, including free partitions for recycling,
    and keeps track of active partitions that contain valid rows.

    Attributes in a Table are defined by an array of AttributeIds. Each attribute is associated
    with a specific index in the columns array of each Partition, which is managed by a hash table
    mapping AttributeId to column index.

    The TableSignature of a Table contains a bitfield where each bit represents the presence or absence
    of an attribute. This allows for quick checks if a table contains a specific set of attributes.

    Partitions are linked together in a chain, starting from the first active partition, which helps in
    efficiently iterating over all partitions that contain entities. Each Partition maintains its own list
    of free indices for row reuse, as well as bitfields for tracking modified and valid rows.
*/
class Table
{
    using ColumnBuffer = void*;

private:
    friend class Database;
    Table() = default;
    ~Table();

public:
    class Partition;

    /// Check if a column exists in the table
    bool HasAttribute(AttributeId attribute) const;

    /// Returns the attribute located in given column
    AttributeId GetAttributeId(ColumnIndex columnIndex) const;
    /// Returns the index of the attribute or invalid if attribute is missing from table
    ColumnIndex GetAttributeIndex(AttributeId attribute) const;
    /// Get the all descriptors for a table
    Util::Array<AttributeId> const& GetAttributes() const;
    /// Get the table signature
    TableSignature const& GetSignature() const;

    /// Add an attribute to the table
    ColumnIndex AddAttribute(AttributeId attribute, bool updateSignature = true);
    /// Add/Get a free row from the table
    RowId AddRow();
    /// Deallocate a row from a table. This only frees the row for recycling. See ::Defragment
    void RemoveRow(RowId row);
    /// Get total number of rows in a table
    SizeT GetNumRows() const;
    /// set total number of rows in a table. This does not allocate any memory or create instaces, thus should pretty much never be used.
    void SetNumRows(SizeT value);
    /// Set all row values to default
    void SetToDefault(RowId row);

    /// Defragment table
    SizeT Defragment(std::function<void(Partition*, RowId, RowId)> const& moveCallback);
    /// Clean table. Does not deallocate anything; just sets the size of the table to zero.
    void Clean();
    /// Reset table. Deallocate all data
    void Reset();

    /// Get first active partition with entities
    Partition* GetFirstActivePartition();
    /// Get number of partitions that contain entities
    uint16_t GetNumActivePartitions() const;
    /// Get number of partitions in table
    uint16_t GetNumPartitions() const;
    /// Get current partition
    Partition* GetCurrentPartition();
    ///
    Partition* GetPartition(uint16_t partitionId);
    /// get a buffer. Might be invalidated if rows are allocated or deallocated
    void* GetValuePointer(ColumnIndex cid, RowId row);
    /// get a buffer. Might be invalidated if rows are allocated or deallocated
    void* GetBuffer(uint16_t partition, ColumnIndex cid);

    /// Serialize a row into a blob.
    Util::Blob SerializeInstance(RowId row) const;
    /// deserialize a blob into a row
    void DeserializeInstance(Util::Blob const& data, RowId row);

    /// move instance from one table to another.
    static RowId MigrateInstance(
        Table& src,
        RowId srcRow,
        Table& dst,
        bool defragment = true,
        std::function<void(Partition*, RowId, RowId)> const& moveCallback = nullptr
    );
    /// duplicate instance from one row into destination table.
    static RowId DuplicateInstance(Table const& src, RowId srcRow, Table& dst);

    /// move n instances from one table to another.
    static void MigrateInstances(
        Table& src,
        Util::Array<RowId> const& srcRows,
        Table& dst,
        Util::FixedArray<RowId>& dstRows,
        bool defragment = true,
        std::function<void(Partition*, IndexT, IndexT)> const& moveCallback = nullptr
    );
    /// duplicate instance from one row into destination table.
    static void DuplicateInstances(Table& src, Util::Array<RowId> const& srcRows, Table& dst, Util::FixedArray<RowId>& dstRows);

    /// allocation heap used for the column buffers
    static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::DefaultHeap;

    /// name of the table
    Util::StringAtom name;

    /// Create a new partition for this table. Adds it to the list of partitions and the vacancy list
    Partition* NewPartition();

private:

    /// the signature of this table. Contains one bit set to true for every attribute that exists in the table.
    TableSignature signature;
    /// table identifier
    TableId tid = TableId::Invalid();
    /// sum of all rows in all partitions of this table, 
    uint32_t totalNumRows = 0;
    /// Current partition that we'll be using when allocating data.
    Partition* currentPartition = nullptr;
    /// All partitions, even null partitions
    Util::Array<Partition*> partitions;
    /// free partitions for recycling allocated partitions
    Util::Array<Partition*> freePartitions;
    /// indices to null partitions
    Util::Array<uint16_t> nullPartitions;
    /// First partition that has entities. You can use this to iterate over all active partitions with entities by following the chain of partition->next.
    Partition* firstActivePartition = nullptr;
    /// number of active partitions
    uint16_t numActivePartitions = 0;
    /// all attributes that this table has
    Util::Array<AttributeId> attributes;
    /// maps attr id -> index in columns array
    Util::HashTable<AttributeId, IndexT, 32, 1> columnRegistry;
};

//------------------------------------------------------------------------------
/**
    @class MemDb::Table::Partition

    @brief Represents a partition within a Table in MemDb.
    
    @details A Partition is a segment of a Table that contains a fixed number of rows and manages
    memory for attribute data through column buffers.
    Each Partition has a unique partition ID and maintains metadata about its contents, such as the
    number of rows it currently holds.

    Partitions are linked together in a chain within a Table, starting from the first active partition,
    which allows efficient iteration over all partitions that contain valid rows. Each Partition keeps track
    of free indices using an array, enabling reuse of memory for deleted rows.

    Column buffers in a Partition store data for each attribute defined in the Table. The columns array
    contains these buffers, and a bitfield tracks whether each row is valid (i.e., not deleted) or modified.
    This information is used for operations like defragmentation and garbage collection within the partition.
*/
class Table::Partition
{
private:
    Partition() = default;
    ~Partition();

public:
    // total capacity (in elements) that the partition can contain
    static constexpr uint CAPACITY = 256;
    // The table that this partition is part of
    Table* table = nullptr;
    // next active partition that has entities, or null if end of chain
    Partition* next = nullptr;
    // previous active partition that has entities, or null if first in chain
    Partition* previous = nullptr;
    // The id of the partition
    uint16_t partitionId = 0xFFFF;
    /// number of rows
    uint32_t numRows = 0;
    // bump the version if you change anything about the partition
    uint64_t version = 0;
    // holds freed indices/rows to be reused in the partition.
    Util::Array<uint16_t> freeIds;
    /// holds all the column buffers. This excludes non-typed attributes
    Util::Array<ColumnBuffer> columns;
    /// check a bit if the row has been modified, and you need to track it.
    /// bits are reset when partition is defragged
    Util::BitField<CAPACITY> modifiedRows;
    /// bits are set if the row is occupied. If the row is removed, the bit is set to zero.
    /// this is kept up to date if defragging the partition.
    Util::BitField<CAPACITY> validRows;

private:
    friend Table;
    /// recycle free row or allocate new row
    uint16_t AllocateRowIndex();
    /// Free an index.
    void FreeIndex(uint16_t instance);
    /// erase row by swapping with last row and reducing number of rows in table
    void EraseSwapIndex(uint16_t instance);
};

} // namespace MemDb
