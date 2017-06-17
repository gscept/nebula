#pragma once
//------------------------------------------------------------------------------
/**
	A graphics context is the base class which implements a graphics entity rendering component.
	This class is abstract, but any class inheriting it has to be a singleton.

	The idea is that every rendering module should implement its own GraphicsContext. The data
	for each graphics entity is then saved in a BlockAllocator as defined in this class, and
	that data is sliced into whatever is important. This way, we can simply loop over all
	slices in the BlockAllocator once per update, and simply ignore all the crud in between.

	Example:

		OOP:
			Entity:
				Model
				Character
				AnimEvents
				Resource Loading data
				Instancing stuff
				Picking stuff
				LightProbe stuff
				Transform

			For each entity
				Run anim event update on entity
					> requires: animation event list, character
					> stride between each entity = tons of stuff
		DO:
			Entity:
				Transform
			Context Singleton:
				Models[]
				Characters[]
				AnimEvents[]

			For each frame
				Run anim event update on AnimEvents[] array
					> Updates all entities
					> stride between each entity = 0

	This base class implements a block allocator which creates chunks of FixedPool objects.
	Whenever an entity is registered, a slice ID is calculated using the pool index, and the index within the pool as a 64 bit integer.
	The entity id is then paired with that slice id, so that the slice and pool can be obtained and free'd. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/fixedpool.h"
#include "timing/time.h"
namespace Graphics
{
class View;
class GraphicsContext : public Core::RefCounted
{
	__DeclareAbstractClass(GraphicsContext);

protected:
	/// constructor
	GraphicsContext();
	/// destructor
	virtual ~GraphicsContext();

	

	template<typename TYPE>
	struct BlockAllocator
	{
		// default constructor just sets the fill function to nullptr
		BlockAllocator()
		{
			fillFunc = nullptr;
		}

		std::function<void(TYPE&, IndexT idx)> fillFunc;
		Util::Array<Util::FixedPool<TYPE>> pool;
	};

	friend class GraphicsServer;

	/// runs before frame is updated
	void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs when visibility has finished processing 
	void OnVisibilityReady(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs before a specific view
	void OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after view is rendered
	void OnAfterView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after a frame is updated
	void OnAfterFrame(const IndexT frameIndex, const Timing::Time frameTime);

	/// update context
	virtual void Update(const IndexT frameIndex, const Timing::Time frameTime);

	/// helper function for allocating a slice in an array of pools
	/// @param func is the function used to setup the individual elements of the slice
	/// @param pool is the pool to allocate for
	/// @param entity is the ID of the entity being registered
	/// @return the global slice index
	template<class POOL_DATA_TYPE> POOL_DATA_TYPE* AllocateSlice(const int64_t& entity, BlockAllocator<POOL_DATA_TYPE>& pool);

	/// helper function to free a slice
	/// @param entity is the entity ID used to create the slice previously
	/// @param pool is the pool from which the 'slice' ID was generated
	template<class POOL_DATA_TYPE> void FreeSlice(const int64_t& entity, BlockAllocator<POOL_DATA_TYPE>& pool);

	/// get slice by id
	/// @param entity is the entity ID used to allocate the slice previously
	template <class POOL_DATA_TYPE> POOL_DATA_TYPE* GetSlice(const int64_t& entity, BlockAllocator<POOL_DATA_TYPE>& pool);

	/// control the size of the entity bucket pool
	const int PoolSize = 512;

private:
	Util::Dictionary<int64_t, int64_t> entitySliceMap;
};

//------------------------------------------------------------------------------
/**
*/
template<class POOL_DATA_TYPE>
inline POOL_DATA_TYPE*
Graphics::GraphicsContext::AllocateSlice(const int64_t& entity, BlockAllocator<POOL_DATA_TYPE>& pool)
{
	n_assert(pool.fillFunc != nullptr);
	int64_t index = -1;
	POOL_DATA_TYPE* slice = nullptr;
	IndexT i;
	for (i = 0; i < pool.pool.Size(); i++)
	{
		if (!pool.pool[i].IsFull())
		{
			slice = pool.pool[i].Alloc();
			index = (static_cast<int64_t>(i) << 32) + (pool.pool[i].NumUsed() - 1);	// index is first 32 bit pool id, rest 32 bit slice id
			break;
		}
	}

	// if no slice is found, allocate new array
	if (slice == nullptr)
	{
		pool.pool.Append(Util::FixedPool<POOL_DATA_TYPE>(PoolSize, pool.func);

		// now get slice
		slice = pool.pool.Back().Alloc();
		index = (static_cast<int64_t>((pool.pool.Size() - 1)) << 32);	// index is first 32 bit pool id, rest 32 bit slice id (we know the pool is initially empty now)
	}

	// add slice to dictionary
	this->entitySliceMap.Add(entity, index);
	n_assert(slice != nullptr && index != -1);
	return slice;
}

//------------------------------------------------------------------------------
/**
*/
template<class POOL_DATA_TYPE>
inline void
Graphics::GraphicsContext::FreeSlice(const int64_t& entity, BlockAllocator<POOL_DATA_TYPE>& pool)
{
	int64_t index = this->entitySliceMap[entity];

	// leftmost 32 bits is the pool index, the rest is the slice index within that pool
	int id = static_cast<int>(index >> 32);
	int slice = static_cast<int>(index & 0x00000000FFFFFFFF);
	pool.pool[id].Free(slice);
}

//------------------------------------------------------------------------------
/**
*/
template <class POOL_DATA_TYPE>
inline POOL_DATA_TYPE*
Graphics::GraphicsContext::GetSlice(const int64_t& entity, BlockAllocator<POOL_DATA_TYPE>& pool)
{
	int64_t index = this->entitySliceMap[entity];

	// leftmost 32 bits is the pool index, the rest is the slice index within that pool
	int id = static_cast<int>(index >> 32);
	int slice = static_cast<int>(index & 0x00000000FFFFFFFF);
	return pool.pool[id][slice];
}

} // namespace Graphics