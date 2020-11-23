#pragma once
//------------------------------------------------------------------------------
/**
	@file	op.h

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/array.h"
#include "game/entity.h"
#include "game/category.h"
#include "memory/arenaallocator.h"
#include "memdb/typeregistry.h"

namespace Game
{

class EntityManager;

namespace Op
{
	struct AddProperty
	{
		AddProperty() = default;
		template<typename TYPE>
		AddProperty(Entity entity, PropertyId pid, TYPE const& value) :
			entity(entity),
			pid(pid),
			value(allocator.Alloc<TYPE>())
		{
			Memory::Copy(&value, const_cast<void*>(this->value), sizeof(TYPE));
		}

		AddProperty(Entity entity, PropertyId pid) :
			entity(entity),
			pid(pid),
			value(MemDb::TypeRegistry::DefaultValue(pid))
		{
			// empty
		}

		AddProperty(const AddProperty& rhs) :
			entity(rhs.entity),
			pid(rhs.pid),
			value(rhs.value)
		{
			// empty
		}

		AddProperty& operator=(AddProperty const& rhs)
		{
			this->entity = rhs.entity;
			this->pid = rhs.pid;
			this->value = rhs.value;
			return *this;
		}

		~AddProperty()
		{
			// value memory is released by calling the ReleaseAllOps function
		}

		/// this function will invalidate ALL allocated operations, including ones that has not been executed yet.
		static void ReleaseAll()
		{
			allocator.Release();
		}

		Game::Entity entity;
		Game::PropertyId pid;
		void const* value;
	private:
		static Memory::ArenaAllocator<1024> allocator;
	};

	//------------------------------------------------------------------------------
	/**
		Used to batch multiple property adds to multiple entities of the same category
	*/
	struct BatchAddProperty
	{
		BatchAddProperty() = default;
		BatchAddProperty(CategoryId entityCategory, std::initializer_list<PropertyId> pids) :
			fromCategory(entityCategory),
			pids(pids)
		{
			this->values.Resize(pids.size());
		}

		void Add(std::initializer_list<AddProperty> addOps)
		{
			Entity entity = addOps.begin()->entity;

			this->entities.Append(entity);
			IndexT pid = 0;
			for (auto& op : addOps)
			{
#ifdef NEBULA_DEBUG
				n_assert2(entity == op.entity, "All ops must be for the same entity!\n");
				n_assert2(this->pids[pid] == op.pid, "Properties must be in same order as when constructing the batch!\n")
#endif
				this->values[pid].Append(op.value);
			}
		}

		bool const IsEmpty() const { return this->entities.Size() == 0; }

	private:
		friend class Game::EntityManager;
		
		CategoryId fromCategory;
		Util::ArrayStack<Game::PropertyId, 4> pids;
		Util::Array<Util::ArrayStack<void const*, 256>> values;
		Util::Array<Game::Entity> entities;
	};

	struct RemoveProperty
	{
		Game::Entity entity;
		Game::PropertyId pid;
	};
}

class OpQueue
{
public:
	OpQueue();
	~OpQueue();

	void Add(Op::AddProperty&& op)
	{
		this->addPropertyQueue.Enqueue(std::move(op));
	}

	void Add(Op::BatchAddProperty&& op)
	{
		this->batchAdds.Append(std::move(op));
	}

	void Add(Op::RemoveProperty&& op)
	{
		this->removePropertyQueue.Enqueue(std::move(op));
	}

	/// release all allocated operations, for all queues.
	/// @note	this function will invalidate ALL allocated operations, in ALL queues, including ones that has not been executed yet.
	static void ReleaseAllOps()
	{
		Op::AddProperty::ReleaseAll();
	}

private:
	friend class EntityManager;
	
	Util::Array<Op::BatchAddProperty> batchAdds;
	Util::Queue<Op::AddProperty> addPropertyQueue;
	Util::Queue<Op::RemoveProperty> removePropertyQueue;
};

inline OpQueue::OpQueue()
{
}

inline OpQueue::~OpQueue()
{
}

} // namespace Game



