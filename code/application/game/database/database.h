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
    ColumnData() : data(nullptr), numRows(nullptr)
    {
        // empty
    }
    ColumnData(ColumnId const columnId, void** ptrptr, uint32_t const* const numRows, bool state = false) :
        data((void**)ptrptr),
        cid(columnId),
        isState(state),
        numRows(numRows)
    {
        // empty
    }
    
    ~ColumnData() = default;
    TYPE& operator[](IndexT index)
    {
        n_assert(this->data != nullptr);
        n_assert(*this->data != nullptr);
#ifdef NEBULA_BOUNDSCHECKS
        n_assert(index >= 0 && index < *this->numRows);
#endif
        void* dataptr = *this->data;
        TYPE* ptr = reinterpret_cast<TYPE*>(dataptr);
        return (ptr[index]);
    }
private:
    void** data;
    ColumnId cid;
    uint32_t const* numRows;
    bool isState = false;
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
    
    bool HasColumn(TableId table, Column col);

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
        static_assert(std::is_standard_layout<TYPE>(), "Type is not standard layout!");
        static_assert(std::is_trivially_copyable<TYPE>(), "Type is not trivially copyable!");
        Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
        
        uint32_t col = table.states.Alloc();

        Table::ColumnBuffer& buffer = table.states.Get<1>(col);
        
        // setup a state description with the default values from the type
        Table::StateDescription desc = Table::StateDescription(TYPE());
        buffer = this->AllocateState(tid, desc);

        table.states.Get<0>(col) = std::move(desc);

        return Game::ColumnData<TYPE>(col, &table.states.Get<1>(col), &table.numRows, true);
    }

    template<typename ATTR>
    Game::ColumnData<typename ATTR::TYPE> GetColumnData(TableId table)
    {
        Game::Database::Table& tbl = this->tables.Get<0>(Ids::Index(table.id));
        ColumnId cid = this->GetColumnId(table, ATTR::Id());
        return Game::ColumnData<ATTR::TYPE>(cid, &tbl.columns.Get<1>(cid.id), &tbl.numRows);
    }

    /// Get a persistant buffer. Only use this if you know what you're doing!
    void** GetPersistantBuffer(TableId table, ColumnId cid)
    {
        Game::Database::Table& tbl = this->tables.Get<0>(Ids::Index(table.id));
        return &tbl.columns.Get<1>(cid.id);
    }

    struct Table
    {
        using ColumnBuffer = void*;

        Util::StringAtom name;

        Util::ArrayAllocator<Column, ColumnBuffer> columns;
        uint32_t numRows = 0;
        uint32_t capacity = 128;
        uint32_t grow = 128;
        // Holds freed indices to be reused in the attribute table.
        Util::Array<IndexT> freeIds;

        class StateDescription
        {
        public:
            template<typename T>
            explicit StateDescription(T const& defaultValue)
            {
                static_assert(std::is_standard_layout<T>(), "State needs to be a standard layout type!");
                static_assert(std::is_trivially_copyable<T>(), "State needs to be trivially copyable type!");
                this->defVal = Memory::Alloc(Memory::HeapType::ObjectHeap, sizeof(T));
                Memory::Copy(&defaultValue, this->defVal, sizeof(T));
                this->typeSize = sizeof(T);
            }
            StateDescription() : defVal(nullptr), typeSize(0)
            {
                // empty
            }
            StateDescription(StateDescription&& desc) : defVal(desc.defVal), typeSize(desc.typeSize)
            {
                desc.defVal = nullptr;
                desc.typeSize = 0;
            }
            ~StateDescription()
            {
                if (this->defVal != nullptr)
                {
                    Memory::Free(Memory::HeapType::ObjectHeap, this->defVal);
                }
            }
            void operator=(StateDescription&& rhs)
            {
                this->typeSize = rhs.typeSize;
                this->defVal = rhs.defVal;

                rhs.typeSize = 0;
                rhs.defVal = nullptr;
            }
            void operator=(StateDescription& rhs)
            {
                this->typeSize = rhs.typeSize;
                this->defVal = rhs.defVal;

                rhs.typeSize = 0;
                rhs.defVal = nullptr;
            }
            
            SizeT typeSize;
            void* defVal;
        };
        
        Util::ArrayAllocator<StateDescription, ColumnBuffer> states;
        
        static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::ObjectArrayHeap;
    };


    /// retrieve a table.
    Table& GetTable(TableId tid);

    SizeT Defragment(TableId tid, std::function<void(InstanceId, InstanceId)> const& moveCallback);


private:
    void EraseSwapIndex(Table& table, InstanceId instance);

    void GrowTable(TableId tid);

    void* AllocateColumn(TableId tid, Column column);
    void* AllocateState(TableId tid, Table::StateDescription const& desc);

    Ids::IdGenerationPool tableIdPool;
    Util::ArrayAllocator<Table> tables;
};

} // namespace Game
