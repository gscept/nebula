//------------------------------------------------------------------------------
//  database.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "database.h"

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
	for (IndexT tid = 0; tid < this->numTables; ++tid)
	{
		this->DeleteTable(tid);
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
Database::DeleteTable(TableId tid)
{
	Table& table = this->tables[tid.id];
	for (IndexT i = 0; i < table.columns.Size(); ++i)
	{
		ColumnDescriptor descriptor = table.columns.Get<0>(i);
		ColumnDescription* desc = TypeRegistry::GetDescription(descriptor);
		void*& buf = table.columns.Get<1>(i);
		Memory::Free(Table::HEAP_MEMORY_TYPE, buf);
		buf = nullptr;
	}
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

	IndexT index = this->AllocateRowIndex(tid);

	for (int i = 0; i < table.columns.Size(); ++i)
	{
		ColumnDescriptor descriptor = table.columns.Get<0>(i);
		ColumnDescription* desc = TypeRegistry::GetDescription(descriptor.id);

		void*& buf = table.columns.Get<1>(i);
		void* val = (char*)buf + (index * desc->typeSize);
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

	for (int i = 0; i < table.columns.Size(); ++i)
	{
		ColumnDescriptor descriptor = table.columns.Get<0>(i);
		ColumnDescription* desc = TypeRegistry::GetDescription(descriptor.id);
		void*& buf = table.columns.Get<1>(i);
		void* val = (char*)buf + (row * desc->typeSize);
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
	@returns	New index/row in destination table
	@note		This might be destructive if the destination table is missing some of the source tables columns!
	@note		This is an instant erase swap on src table, which means any external references to rows (instance ids) will be invalidated!
*/
IndexT
Database::MigrateInstance(TableId srcTid, IndexT srcRow, TableId dstTid)
{
	n_assert(srcTid != dstTid);
	IndexT dstRow = this->DuplicateInstance(srcTid, srcRow, dstTid);
	this->EraseSwapIndex(GetTable(srcTid), srcRow);
	return dstRow;
}

//------------------------------------------------------------------------------
/**
	@returns	New index/row in destination table
	@note		This might be destructive if the destination table is missing some of the source tables columns!
	@todo		GetColumnId is quite expensive, should be possible to improve
*/
IndexT
Database::DuplicateInstance(TableId srcTid, IndexT srcRow, TableId dstTid)
{
	Table& src = this->tables[Ids::Index(srcTid.id)];
	Table& dst = this->tables[Ids::Index(dstTid.id)];

	IndexT dstRow = this->AllocateRowIndex(dstTid);

	auto const& cols = src.columns.GetArray<0>();
	auto& buffers = src.columns.GetArray<1>();
	auto const& dstCols = dst.columns.GetArray<0>();
	auto& dstBuffers = dst.columns.GetArray<1>();

	const int numDstCols = dst.columns.Size();

	for (int i = 0; i < numDstCols; ++i)
	{
		ColumnDescriptor descriptor = dstCols[i];
		ColumnDescription const* const desc = TypeRegistry::GetDescription(descriptor.id);
		void*& dstBuf = dstBuffers[i];
		SizeT const byteSize = desc->typeSize;

		ColumnId const srcColId = this->GetColumnId(srcTid, descriptor);
		if (srcColId != ColumnId::Invalid())
		{
			// Copy value from src
			void*& srcBuf = buffers[srcColId.id];
			Memory::Copy((char*)srcBuf + (byteSize * srcRow), (char*)dstBuf + (byteSize * dstRow), byteSize);
		}
		else
		{
			// Set default value
			void* val = (char*)dstBuf + (dstRow * desc->typeSize);
			Memory::Copy(desc->defVal, val, desc->typeSize);
		}
	}

	return dstRow;
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
	@returns	number of erased instances.
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
		for (int i = 0; i < table.columns.Size(); ++i)
		{
			ColumnDescriptor descriptor = cols[i];
			ColumnDescription* desc = TypeRegistry::GetDescription(descriptor.id);
			void*& buf = buffers[i];
			const SizeT byteSize = desc->typeSize;
			Memory::Copy((char*)buf + (byteSize * end), (char*)buf + (byteSize * instance), byteSize);
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
		ColumnDescription* desc = TypeRegistry::GetDescription(descriptor);
		void*& buf = table.columns.Get<1>(i);

		const SizeT byteSize = desc->typeSize;

		int oldNumBytes = byteSize * oldCapacity;
		int newNumBytes = byteSize * table.capacity;
		void* newData = Memory::Alloc(Table::HEAP_MEMORY_TYPE, newNumBytes);
		
		Memory::Copy(buf, newData, table.numRows * byteSize);
		Memory::Free(Table::HEAP_MEMORY_TYPE, buf);
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
*/
ColumnId
Database::AddColumn(TableId tid, ColumnDescriptor column)
{
	n_assert(this->IsValid(tid));
	Table& table = this->tables[Ids::Index(tid.id)];

	IndexT found = table.columns.GetArray<0>().FindIndex(column);
	if (found != InvalidIndex)
	{
		n_printf("Warning: Adding multiple columns of same type to a table is not supported and will result in only one column!\n");
		return found;
	}

	uint32_t col = table.columns.Alloc();

	Table::ColumnBuffer& buffer = table.columns.Get<1>(col);
	table.columns.Get<0>(col) = column;

	buffer = this->AllocateBuffer(tid, TypeRegistry::GetDescription(column.id));

	// add the tid to the columndescriptions table registry so that we can access the buffer with less lookups.
	TypeRegistry::GetDescription(column.id)->tableRegistry.Add(tid, this->GetPersistantBuffer(tid, col));

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
			if (!TypeRegistry::GetDescription(column.id)->tableRegistry.Contains(tid))
			{
				valid = false;
				break;
			}
		}

		if (valid)
		{
			for (auto column : filterset.exclusive)
			{
				if (TypeRegistry::GetDescription(column.id)->tableRegistry.Contains(tid))
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

			Dataset::View view;
			view.tid = tid;
			view.numInstances = this->GetNumRows(tid);
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

} // namespace MemDb
