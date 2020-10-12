//------------------------------------------------------------------------------
//  database.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "database.h"

namespace MemDb
{

__ImplementClass(MemDb::Database, 'MmDb', Core::RefCounted);

static constexpr Memory::HeapType ALLOCATIONHEAP = Memory::HeapType::DefaultHeap;

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
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TableId
Database::CreateTable(TableCreateInfo const& info)
{
	TableId id;
	this->tableIdPool.Allocate(id.id);
	n_assert2(Ids::Index(id.id) < MAX_NUM_TABLES, "Tried to allocate more tables than currently allowed! Increase MAX_NUM_TABLES if this keeps occuring!\n");

	Table& table = this->tables[Ids::Index(id.id)];
	// Make sure we don't use any old data
	table = Table();

	table.name = info.name;
	
	for (IndexT i = 0; i < info.columns.Size(); i++)
	{
		this->AddColumn(id, info.columns[i]);
	}

	this->numTables = (Ids::Index(id.id) + 1 > this->numTables ? Ids::Index(id.id) + 1 : this->numTables);

	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
Database::DeleteTable(TableId table)
{
	n_error("implement me!");
	// FIXME: Note that the query method (and possibly others) currently assume that all tables that are present in the array are valid... We should change this

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
Database::HasColumn(TableId table, ColumnDescriptor col)
{
	n_assert(this->IsValid(table));
	return this->tables[Ids::Index(table.id)].columns.GetArray<0>().FindIndex(col) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
ColumnDescriptor
Database::GetColumn(TableId table, ColumnId columnId)
{
	n_assert(this->IsValid(table));
	return this->tables[Ids::Index(table.id)].columns.Get<0>(columnId.id);
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Database::GetColumnId(TableId table, ColumnDescriptor column)
{
	n_assert(this->IsValid(table));
	n_assert(column != ColumnDescriptor::Invalid());
	ColumnId cid = this->tables[Ids::Index(table.id)].columns.GetArray<0>().FindIndex(column);
	n_assert(cid != ColumnId::Invalid());
	return cid;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
Database::AllocateRow(TableId tid)
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

	this->SetToDefault(tid, index);
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

	for (int i = 0; i < table.columns.Size(); ++i)
	{
		ColumnDescriptor descriptor = table.columns.Get<0>(i);
		ColumnDescription* desc = this->columnDescriptions[descriptor.id];
		void*& buf = table.columns.Get<1>(i);
		void* val = (char*)buf + (row * desc->typeSize);
		if (desc->trivialType)
		{
			Memory::Copy(desc->defVal, val, desc->typeSize);
		}
		else
		{
			desc->fTable.Assign(val, desc->defVal, 1);
		}
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
Util::Array<ColumnDescriptor> const&
Database::GetColumns(TableId tid)
{
	n_assert(this->IsValid(tid));
	Table& table = this->tables[Ids::Index(tid.id)];
	auto& colTypes = table.columns.GetArray<0>();
	return colTypes;
}

//------------------------------------------------------------------------------
/**
*/
Table&
Database::GetTable(TableId tid)
{
	return this->tables[Ids::Index(tid.id)];
}

//------------------------------------------------------------------------------
/**
	Defragments a table and call the move callback BEFORE moving elements.
	Returns number of erased instances.
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
*/
void
Database::EraseSwapIndex(Table& table, IndexT instance)
{
	// Swap the element with the last element, and decrement size of array.
	auto const& cols = table.columns.GetArray<0>();
	auto& buffers = table.columns.GetArray<1>();

	uint32_t end = table.numRows - 1;

	// erase swap index in column buffers
	for (int i = 0; i < table.columns.Size(); ++i)
	{
		ColumnDescriptor descriptor = table.columns.Get<0>(i);
		ColumnDescription* desc = this->columnDescriptions[descriptor.id];
		void*& buf = table.columns.Get<1>(i);
		const SizeT byteSize = desc->typeSize;
		if (desc->trivialType)
		{
			Memory::Copy((char*)buf + (byteSize * end), (char*)buf + (byteSize * instance), byteSize);
		}
		else
		{
			desc->fTable.Assign((char*)buf + (byteSize * instance), (char*)buf + (byteSize * end), 1);
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
	auto& colTypes = table.columns.GetArray<0>();
	auto& buffers = table.columns.GetArray<1>();

	int oldCapacity = table.capacity;
	table.capacity += table.grow;
	table.grow *= 2;

	// Grow column buffers
	for (int i = 0; i < table.columns.Size(); ++i)
	{
		ColumnDescriptor descriptor = table.columns.Get<0>(i);
		ColumnDescription* desc = this->columnDescriptions[descriptor.id];
		void*& buf = table.columns.Get<1>(i);

		const SizeT byteSize = desc->typeSize;

		int oldNumBytes = byteSize * oldCapacity;
		int newNumBytes = byteSize * table.capacity;
		void* newData = Memory::Alloc(ALLOCATIONHEAP, newNumBytes);

		if (desc->trivialType)
		{
			Memory::Move(buf, newData, table.numRows * byteSize);
			Memory::Free(ALLOCATIONHEAP, buf);
		}
		else
		{
			desc->fTable.Copy(buf, newData, table.numRows);
		}
		buf = newData;
	}
}

//------------------------------------------------------------------------------
/**
*/
void*
Database::AllocateBuffer(TableId tid, ColumnDescription* desc)
{
	n_assert(this->IsValid(tid));
	n_assert(desc->defVal != nullptr);
	n_assert(desc->typeSize != 0);

	Table& table = this->tables[Ids::Index(tid.id)];

	void* buffer = Memory::Alloc(ALLOCATIONHEAP, desc->typeSize * table.capacity);

	if (!desc->trivialType)
	{
		desc->fTable.Assign(buffer, desc->defVal, table.numRows);
	}
	else
	{
		for (IndexT i = 0; i < table.numRows; ++i)
		{
			void* val = (char*)buffer + (i * desc->typeSize);
			Memory::Copy(desc->defVal, val, desc->typeSize);
		}
	}
	
	return buffer;
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Database::AddColumn(TableId tid, ColumnDescriptor column)
{
	n_assert(this->IsValid(tid));
	Table& table = this->tables[Ids::Index(tid.id)];

	IndexT found = table.columns.GetArray<0>().FindIndex(column);
	if (found != InvalidIndex)
		return found;

	uint32_t col = table.columns.Alloc();

	Table::ColumnBuffer& buffer = table.columns.Get<1>(col);
	table.columns.Get<0>(col) = column;

	buffer = this->AllocateBuffer(tid, this->columnDescriptions[column.id]);

	// add the tid to the columndescriptions table registry so that we can access the buffer with less lookups.
	this->columnDescriptions[column.id]->tableRegistry.Add(tid, this->GetPersistantBuffer(tid, col));

	return col;
}

//------------------------------------------------------------------------------
/**
*/
Dataset
Database::Query(FilterSet const& filterset)
{
	Dataset set;
	set.filter = filterset;

	for (IndexT tid = 0; tid < this->numTables; tid++)
	{
		bool valid = true;
		for (auto column : filterset.inclusive)
		{
			if (!this->columnDescriptions[column.id]->tableRegistry.Contains(tid))
			{
				valid = false;
				break;
			}
		}

		if (valid)
		{
			for (auto column : filterset.exclusive)
			{
				if (this->columnDescriptions[column.id]->tableRegistry.Contains(tid))
				{
					valid = false;
					break;
				}
			}
		}

		if (valid)
		{
			Util::ArrayStack<void*, 16> buffers;
			buffers.Reserve(filterset.inclusive.Size());

			Table const& tbl = this->GetTable(tid);

			IndexT i = 0;
			for (auto attrid : filterset.inclusive)
			{
				ColumnId colId = this->GetColumnId(tid, attrid);
				buffers.Append(tbl.columns.Get<1>(colId.id));
			}

			Dataset::View view = {
				tid,
				this->GetNumRows(tid),
				std::move(buffers)
			};

			set.tables.Append(std::move(view));
		}
	}

	return set;
}

} // namespace MemDb
