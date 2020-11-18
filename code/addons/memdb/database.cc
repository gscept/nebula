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
	for (IndexT tableIndex = 0; tableIndex < this->numTables; ++tableIndex)
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
	n_assert2(Ids::Index(id.id) < MAX_NUM_TABLES, "Tried to allocate more tables than currently allowed! Increase MAX_NUM_TABLES if this keeps occuring!\n");

	Table& table = this->tables[Ids::Index(id.id)];
	// Make sure we don't use any old data
	table = Table();
	table.tid = id;
	table.name = info.name;

	const SizeT numColumns = info.columns.Size();
	for (IndexT i = 0; i < numColumns; i++)
	{
		this->AddColumn(id, info.columns[i]);
	}

	TableSignature& signature = this->tableSignatures[Ids::Index(id.id)];
	signature = TableSignature(info.columns);

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
		PropertyDescription* desc = TypeRegistry::GetDescription(descriptor);
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
Database::HasProperty(TableId table, PropertyId col)
{
	n_assert(this->IsValid(table));
	return this->tableSignatures[Ids::Index(table.id)].IsSet(col);
	return this->tables[Ids::Index(table.id)].columns.GetArray<0>().FindIndex(col) != InvalidIndex;
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
Database::GetColumnId(TableId table, PropertyId column)
{
	n_assert(this->IsValid(table));
	n_assert(column != PropertyId::Invalid());
	ColumnIndex cid = this->tables[Ids::Index(table.id)].columns.GetArray<0>().FindIndex(column);
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
	@note		This is an instant erase swap on src table, which means any external references to rows (instance ids) will be invalidated!
*/
IndexT
Database::MigrateInstance(TableId srcTid, IndexT srcRow, Ptr<Database> const& dstDb, TableId dstTid)
{
	IndexT dstRow = this->DuplicateInstance(srcTid, srcRow, dstDb, dstTid);
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
	n_assert(this->IsValid(srcTid));
	n_assert(this->IsValid(dstTid));

	Table& src = this->tables[Ids::Index(srcTid.id)];
	Table& dst = this->tables[Ids::Index(dstTid.id)];

	IndexT dstRow = this->AllocateRowIndex(dstTid);

	auto const& cols = src.columns.GetArray<0>();
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
	@returns	New index/row in destination table
	@note		This might be destructive if the destination table is missing some of the source tables columns!
	@todo		GetColumnId is quite expensive, should be possible to improve
*/
IndexT
Database::DuplicateInstance(TableId srcTid, IndexT srcRow, Ptr<Database> const& dstDb, TableId dstTid)
{
	n_assert(this->IsValid(srcTid));
	n_assert(dstDb->IsValid(dstTid));

	Table& src = this->tables[Ids::Index(srcTid.id)];
	Table& dst = dstDb->tables[Ids::Index(dstTid.id)];

	IndexT dstRow = dstDb->AllocateRowIndex(dstTid);

	auto const& cols = src.columns.GetArray<0>();
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
*/
Table&
Database::GetTable(TableId tid)
{
	n_assert(this->IsValid(tid));
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
	auto& colTypes = table.columns.GetArray<0>();
	auto& buffers = table.columns.GetArray<1>();

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

		int oldNumBytes = byteSize * oldCapacity;
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
*/
ColumnIndex
Database::AddColumn(TableId tid, PropertyId column)
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

	return col;
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
		IndexT tid = potentialTables[index];
		Table const& tbl = this->GetTable(tid);
		if (this->IsValid(tbl.tid))
		{
			Util::ArrayStack<void*, 16> buffers;
			buffers.Reserve(filterset.PropertyIds().Size());


			IndexT i = 0;
			for (auto attrid : filterset.PropertyIds())
			{
				ColumnIndex colId = this->GetColumnId(tid, attrid);
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
