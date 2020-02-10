//------------------------------------------------------------------------------
//  database.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "database.h"

namespace Game
{
__ImplementClass(Game::Database, 'GMDB', Core::RefCounted);

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
	n_assert(Ids::Index(id.id) <= this->tables.Size());

	Table* table;

	if (Ids::Index(id.id) == this->tables.Size())
	{
		uint32_t idx = this->tables.Alloc();
		table = &this->tables.Get<0>(idx);
	}
	else
	{
		table = &this->tables.Get<0>(Ids::Index(id.id));
	}

	table->name = info.name;
	table->columns.Clear();

	for (IndexT i = 0; i < info.columns.Size(); i++)
	{
		this->AddColumn(id, info.columns[i]);
	}

	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
Database::DeleteTable(TableId table)
{
	n_error("implement me!");
	//n_delete(this->tables[Ids::Index(table.id)]);
	//this->tableIdPool.Deallocate(table.id);

}

//------------------------------------------------------------------------------
/**
*/
bool
Database::IsValid(TableId table)
{
	return this->tableIdPool.IsValid(table.id);
}

//------------------------------------------------------------------------------
/**
*/
Column
Database::GetColumn(TableId table, ColumnId columnId)
{
	return this->tables.Get<0>(Ids::Index(table.id)).columns.Get<0>(columnId.id);
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Database::GetColumnId(TableId table, Column column)
{
	ColumnId cid = this->tables.Get<0>(Ids::Index(table.id)).columns.GetArray<0>().FindIndex(column);
	n_assert(cid != ColumnId::Invalid());
	return cid;
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Database::AddColumn(TableId tid, Column column)
{
	Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
	uint32_t col = table.columns.Alloc();

	Table::ColumnBuffer& buffer = table.columns.Get<1>(col);
	table.columns.Get<0>(col) = column;

	buffer = this->AllocateColumn(tid, column);

	return col;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
Database::AllocateRow(TableId tid)
{
	Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));

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
	Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
	n_assert(row < table.numRows);
	table.freeIds.InsertSorted(row);
}

//------------------------------------------------------------------------------
/**
*/
void
Database::SetToDefault(TableId tid, IndexT row)
{
	Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
	n_assert(row < table.numRows);

	for (IndexT col = 0; col < table.columns.Size(); col++)
	{
		Column column = table.columns.Get<0>(col);
		void* buffer = table.columns.Get<1>(col);
		
		switch (column.GetValueType())
		{
		case Attr::ValueType::IntType:
			*((int*)buffer + row) = column.GetIntDefValue();
			break;
		case Attr::ValueType::FloatType:
			*((float*)buffer + row) = column.GetFloatDefValue();
			break;
		case Attr::ValueType::StringType:
		{
			Util::String& to = ((Util::String*)buffer)[row];
			to = column.GetStringDefValue();
			break;
		}
		case Attr::ValueType::Float4Type:
			*((Math::float4*)buffer + row) = column.GetFloat4DefValue();
			break;
		default:
			n_error("Type not yet supported!");
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
SizeT
Database::GetNumRows(TableId table)
{
	return this->tables.Get<0>(Ids::Index(table.id)).numRows;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void
GrowBuffer(Column column, void*& buffer, const SizeT capacity, const SizeT size, const SizeT newCapacity)
{
	if constexpr (!std::is_trivial<TYPE>::value)
	{
		const SizeT byteSize = column.GetSizeOfType();
		
		int oldNumBytes = byteSize * capacity;
		int newNumBytes = byteSize * newCapacity;
		void* newData = Memory::Alloc(ALLOCATIONHEAP, newNumBytes);

		Memory::Move(buffer, newData, size * byteSize);
		Memory::Free(ALLOCATIONHEAP, buffer);
		buffer = newData;
	}
	else
	{
		const SizeT byteSize = column.GetSizeOfType();

		TYPE* newData = n_new_array(TYPE, newCapacity);

		for (int row = 0; row < size; ++row)
		{
			newData[row] = std::move(((TYPE*)buffer)[row]);
		}

		n_delete_array((TYPE*)buffer);

		buffer = (void*)newData;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
Database::GrowTable(TableId tid)
{
	Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
	auto& colTypes = table.columns.GetArray<0>();
	auto& buffers = table.columns.GetArray<1>();

	int oldCapacity = table.capacity;
	table.capacity += table.grow;
	table.grow *= 2;

	for (int i = 0; i < colTypes.Size(); ++i)
	{
		switch (colTypes[i].GetValueType())
		{
		case Attr::ValueType::IntType: GrowBuffer<int>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Attr::ValueType::FloatType: GrowBuffer<float>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Attr::ValueType::Float4Type: GrowBuffer<Math::float4>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Attr::ValueType::StringType: GrowBuffer<Util::String>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		default:
			n_error("Type not yet supported!");
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void*
AllocateBuffer(Column column, const SizeT capacity, const SizeT size)
{
	if constexpr (!std::is_trivial<TYPE>::value)
	{
		const SizeT byteSize = column.GetSizeOfType();
		void* buffer = Memory::Alloc(ALLOCATIONHEAP, capacity * byteSize);

		for (IndexT i = 0; i < size; ++i)
		{
			*((TYPE*)buffer + i) = column.GetDefaultValue().Get<TYPE>();
		}

		return buffer;
	}
	else
	{
		TYPE* buffer = n_new_array(TYPE, capacity);
		for (int i = 0; i < size; i++)
		{
			buffer[i] = column.GetDefaultValue().Get<TYPE>();
		}
		return buffer;
	}
}

//------------------------------------------------------------------------------
/**
*/
void*
Database::AllocateColumn(TableId tid, Column column)
{
	Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));

	const Attr::ValueType type = column.GetValueType();

	void* buffer = nullptr;
	switch (column.GetValueType())
	{
	case Attr::ValueType::IntType:		buffer = AllocateBuffer<int>(column, table.capacity, table.numRows); break;
	case Attr::ValueType::FloatType:	buffer = AllocateBuffer<float>(column, table.capacity, table.numRows); break;
	case Attr::ValueType::Float4Type:	buffer = AllocateBuffer<Math::float4>(column, table.capacity, table.numRows); break;
	case Attr::ValueType::StringType:	buffer = AllocateBuffer<Util::String>(column, table.capacity, table.numRows); break;
	default:
		n_error("Type not yet supported!");
	}

	return buffer;
}


} // namespace Game
