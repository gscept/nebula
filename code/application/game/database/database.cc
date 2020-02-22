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
bool
Database::HasColumn(TableId table, Column col)
{
	return this->tables.Get<0>(Ids::Index(table.id)).columns.GetArray<0>().FindIndex(col) != InvalidIndex;
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
	n_assert(column.IsValid());
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

	IndexT found = table.columns.GetArray<0>().FindIndex(column);
	if (found != InvalidIndex)
		return found;

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

		switch (column.GetType())
		{
		case Game::AttributeType::Int8Type:			*((int8_t*)buffer + row)	= std::get<int8_t>(column.GetDefaultValue()); break;
		case Game::AttributeType::UInt8Type:		*((uint8_t*)buffer + row)	= std::get<uint8_t>(column.GetDefaultValue()); break;
		case Game::AttributeType::Int16Type:		*((int16_t*)buffer + row)	= std::get<int16_t>(column.GetDefaultValue()); break;
		case Game::AttributeType::UInt16Type:		*((uint16_t*)buffer + row)	= std::get<uint16_t>(column.GetDefaultValue()); break;
		case Game::AttributeType::Int32Type:		*((int32_t*)buffer + row)	= std::get<int32_t>(column.GetDefaultValue()); break;
		case Game::AttributeType::UInt32Type:		*((uint32_t*)buffer + row)	= std::get<uint32_t>(column.GetDefaultValue()); break;
		case Game::AttributeType::Int64Type:		*((int64_t*)buffer + row)	= std::get<int64_t>(column.GetDefaultValue()); break;
		case Game::AttributeType::UInt64Type:		*((uint64_t*)buffer + row)	= std::get<uint64_t>(column.GetDefaultValue()); break;
		case Game::AttributeType::FloatType:		*((float*)buffer + row)		= std::get<float>(column.GetDefaultValue()); break;
		case Game::AttributeType::DoubleType:		*((double*)buffer + row)	= std::get<double>(column.GetDefaultValue()); break;
		case Game::AttributeType::BoolType:			*((bool*)buffer + row)		= std::get<bool>(column.GetDefaultValue()); break;
		case Game::AttributeType::Float2Type:		*((Math::float2*)buffer + row)		= std::get<Math::float2>(column.GetDefaultValue()); break;
		case Game::AttributeType::Float4Type:		*((Math::float4*)buffer + row)		= std::get<Math::float4>(column.GetDefaultValue()); break;
		case Game::AttributeType::QuaternionType:	*((Math::quaternion*)buffer + row)	= std::get<Math::quaternion>(column.GetDefaultValue()); break;
		case Game::AttributeType::Matrix44Type:		*((Math::matrix44*)buffer + row)	= std::get<Math::matrix44>(column.GetDefaultValue()); break;
		case Game::AttributeType::GuidType:			*((Util::Guid*)buffer + row)		= std::get<Util::Guid>(column.GetDefaultValue()); break;
		case Game::AttributeType::EntityType:		*((Game::Entity*)buffer + row)		= std::get<Game::Entity>(column.GetDefaultValue()); break;
		case Game::AttributeType::StringType:		*((Util::String*)buffer + row)		= std::get<Util::String>(column.GetDefaultValue()); break;

		default:
			n_error("Type not yet supported!");
		}
	}

	for (int i = 0; i < table.states.Size(); ++i)
	{
		auto const& desc = table.states.Get<0>(i);
		void*& buf = table.states.Get<1>(i);
		void* val = (char*)buf + (row * desc.typeSize);
		Memory::Copy(desc.defVal, val, desc.typeSize);
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
Util::Array<Column> const&
Database::GetColumns(TableId tid)
{
	Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
	auto& colTypes = table.columns.GetArray<0>();
	return colTypes;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void
GrowBuffer(Column column, void*& buffer, const SizeT capacity, const SizeT size, const SizeT newCapacity)
{
	if constexpr (std::is_trivial<TYPE>::value)
	{
		const SizeT byteSize = Game::GetAttributeSize(column.GetType());

		int oldNumBytes = byteSize * capacity;
		int newNumBytes = byteSize * newCapacity;
		void* newData = Memory::Alloc(ALLOCATIONHEAP, newNumBytes);

		Memory::Move(buffer, newData, size * byteSize);
		Memory::Free(ALLOCATIONHEAP, buffer);
		buffer = newData;
	}
	else
	{
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
Database::Table&
Database::GetTable(TableId tid)
{
	return this->tables.Get<0>(Ids::Index(tid.id));
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
		switch (colTypes[i].GetType())
		{
		case Game::AttributeType::Int8Type:			GrowBuffer<int8_t>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::UInt8Type:		GrowBuffer<uint8_t>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::Int16Type:		GrowBuffer<int16_t>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::UInt16Type:		GrowBuffer<uint16_t>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::Int32Type:		GrowBuffer<int32_t>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::UInt32Type:		GrowBuffer<uint32_t>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::Int64Type:		GrowBuffer<int64_t>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::UInt64Type:		GrowBuffer<uint64_t>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::FloatType:		GrowBuffer<float>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::DoubleType:		GrowBuffer<double>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::BoolType:			GrowBuffer<bool>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::Float2Type:		GrowBuffer<Math::float2>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::Float4Type:		GrowBuffer<Math::float4>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::QuaternionType:	GrowBuffer<Math::quaternion>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::Matrix44Type:		GrowBuffer<Math::matrix44>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::GuidType:			GrowBuffer<Util::Guid>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::EntityType:		GrowBuffer<Game::Entity>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		case Game::AttributeType::StringType:		GrowBuffer<Util::String>(colTypes[i], buffers[i], oldCapacity, table.numRows, table.capacity); break;
		default:
			n_error("Type not yet supported!");
		}
	}

	// Grow state buffers
	for (int i = 0; i < table.states.Size(); ++i)
	{
		auto const& desc = table.states.Get<0>(i);
		void*& buf = table.states.Get<1>(i);

		const SizeT byteSize = desc.typeSize;

		int oldNumBytes = byteSize * oldCapacity;
		int newNumBytes = byteSize * table.capacity;
		void* newData = Memory::Alloc(ALLOCATIONHEAP, newNumBytes);

		Memory::Move(buf, newData, table.numRows * byteSize);
		Memory::Free(ALLOCATIONHEAP, buf);
		buf = newData;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void*
AllocateBuffer(Column column, const SizeT capacity, const SizeT size)
{
	if constexpr (std::is_trivial<TYPE>::value)
	{
		const SizeT byteSize = Game::GetAttributeSize(column.GetType());
		void* buffer = Memory::Alloc(ALLOCATIONHEAP, capacity * byteSize);

		for (IndexT i = 0; i < size; ++i)
		{
			*((TYPE*)buffer + i) = std::get<TYPE>(column.GetDefaultValue());
		}

		return buffer;
	}
	else
	{
		TYPE* buffer = n_new_array(TYPE, capacity);
		for (int i = 0; i < size; i++)
		{
			buffer[i] = std::get<TYPE>(column.GetDefaultValue());
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

	const Game::AttributeType type = column.GetType();

	void* buffer = nullptr;

	switch (column.GetType())
	{
	case Game::AttributeType::Int8Type:			buffer = AllocateBuffer<int8_t>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::UInt8Type:		buffer = AllocateBuffer<uint8_t>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::Int16Type:		buffer = AllocateBuffer<int16_t>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::UInt16Type:		buffer = AllocateBuffer<uint16_t>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::Int32Type:		buffer = AllocateBuffer<int32_t>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::UInt32Type:		buffer = AllocateBuffer<uint32_t>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::Int64Type:		buffer = AllocateBuffer<int64_t>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::UInt64Type:		buffer = AllocateBuffer<uint64_t>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::FloatType:		buffer = AllocateBuffer<float>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::DoubleType:		buffer = AllocateBuffer<double>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::BoolType:			buffer = AllocateBuffer<bool>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::Float2Type:		buffer = AllocateBuffer<Math::float2>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::Float4Type:		buffer = AllocateBuffer<Math::float4>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::QuaternionType:	buffer = AllocateBuffer<Math::quaternion>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::Matrix44Type:		buffer = AllocateBuffer<Math::matrix44>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::GuidType:			buffer = AllocateBuffer<Util::Guid>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::EntityType:		buffer = AllocateBuffer<Game::Entity>(column, table.capacity, table.numRows); break;
	case Game::AttributeType::StringType:		buffer = AllocateBuffer<Util::String>(column, table.capacity, table.numRows); break;
	default:
		n_error("Type not yet supported!\n");
	}

	return buffer;
}

//------------------------------------------------------------------------------
/**
*/
void*
Database::AllocateState(TableId tid, Table::StateDescription const& desc)
{
	n_assert(desc.defVal != nullptr);
	n_assert(desc.typeSize != 0);

	Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));

	void* buffer = Memory::Alloc(ALLOCATIONHEAP, desc.typeSize * table.capacity);

	for (IndexT i = 0; i < table.numRows; ++i)
	{
		void* val = (char*)buffer + (i * desc.typeSize);
		Memory::Copy(desc.defVal, val, desc.typeSize);
	}

	return buffer;
}


} // namespace Game
