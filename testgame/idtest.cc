//------------------------------------------------------------------------------
//  idtest.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "idtest.h"
#include "ids/id.h"
#include "ids/idallocator.h"
#include "ids/idpool.h"
#include "ids/idgenerationpool.h"

using namespace Ids;

namespace Test
{
__ImplementClass(Test::IdTest, 'IDTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
IdTest::Run()
{
	{
		Util::Array<uint32_t> firstGenIds;
		Util::Array<uint32_t> secondGenIds;

		IdGenerationPool pool;
		
		Id32 tmp;
		pool.Allocate(tmp);
		this->Verify(tmp == 0);
		this->Verify(pool.IsValid(tmp));

		pool.Deallocate(tmp);
		this->Verify(!pool.IsValid(tmp));
		Id32 tmp2;
		pool.Allocate(tmp2);
		this->Verify(tmp != tmp2);
		pool.Deallocate(tmp2);
		this->Verify(!pool.IsValid(tmp2));

		uint32_t generation = Generation(tmp);
		this->Verify(generation == 0);

		uint32_t lastidx = tmp2;
		bool increase = true;
		for (int i = 0; i < 1022; i++)
		{
			Id32 tmp;
			pool.Allocate(tmp);
			increase &= tmp > lastidx;
			lastidx = tmp;
			pool.Deallocate(tmp);
		}
		this->Verify(increase);
		for (int i = 0; i < 1022; i++)
		{
			Id32 tmp;
			pool.Allocate(tmp);
			uint32_t index = Index(tmp);
			generation = Generation(tmp);
			increase &= index < 1024;
			pool.Deallocate(tmp);
		}
		this->Verify(increase);

		this->Verify(generation == 1);

		pool = IdGenerationPool();

		const int numIds = 1024;

		Id32 temp;
		for (SizeT i = 0; i < numIds; i++)
		{
			pool.Allocate(temp);
			firstGenIds.Append(temp);
		}

		for (SizeT i = 0; i < numIds; i++)
		{
			pool.Deallocate(firstGenIds[i]);
		}

		for (SizeT i = 0; i < numIds; i++)
		{
			pool.Allocate(temp);
			secondGenIds.Append(temp);
		}

		for (SizeT i = 0; i < numIds; i++)
		{
			this->Verify(firstGenIds[i] != secondGenIds[i]);
		}

		firstGenIds.Clear();

		for (SizeT i = 0; i < numIds; i++)
		{
			pool.Allocate(temp);
			firstGenIds.Append(temp);
		}

		for (SizeT i = 0; i < numIds; i++)
		{
			this->Verify(firstGenIds[i] != secondGenIds[i]);
		}
	}

	{
		Util::Array<Id32> ids;

		IdAllocator<int, float, Util::String> allocator;

		const int numAllocs = 1024;
		for (SizeT i = 0; i < numAllocs; i++)
		{
			Id32 id = allocator.AllocObject();
			ids.Append(id);
			allocator.Get<0>(id) = i;
			allocator.Get<1>(id) = 2.0f;
			allocator.Get<2>(id) = "Test";
		}
		this->Verify(allocator.GetNumUsed() == numAllocs);

		this->Verify(allocator.Get<1>(Id32(0)) == 2.0f);
		this->Verify(allocator.Get<2>(Id32(0)) == "Test");

		for (SizeT i = 0; i < numAllocs; i++)
		{
			this->Verify(allocator.Get<0>(ids[i]) == i);
		}

		for (SizeT i = 0; i < numAllocs; i++)
		{
			allocator.DeallocObject(ids[i]);
		}

		this->Verify(allocator.GetNumUsed() == 0);
	}
}

}