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
		VERIFY(tmp == 0);
		VERIFY(pool.IsValid(tmp));

		pool.Deallocate(tmp);
		VERIFY(!pool.IsValid(tmp));
		Id32 tmp2;
		pool.Allocate(tmp2);
		VERIFY(tmp != tmp2);
		pool.Deallocate(tmp2);
		VERIFY(!pool.IsValid(tmp2));

		uint32_t generation = Generation(tmp);
		VERIFY(generation == 0);

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
		VERIFY(increase);
		for (int i = 0; i < 1022; i++)
		{
			Id32 tmp;
			pool.Allocate(tmp);
			uint32_t index = Index(tmp);
			generation = Generation(tmp);
			increase &= index < 1024;
			pool.Deallocate(tmp);
		}
		VERIFY(increase);

		VERIFY(generation == 1);

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

		bool success = true;

		for (SizeT i = 0; i < numIds; i++)
		{
			success &= (firstGenIds[i] != secondGenIds[i]);
		}
        VERIFY(success);
		firstGenIds.Clear();

		for (SizeT i = 0; i < numIds; i++)
		{
			pool.Allocate(temp);
			firstGenIds.Append(temp);
		}
        success = true;
		for (SizeT i = 0; i < numIds; i++)
		{
			success &= (firstGenIds[i] != secondGenIds[i]);
		}
        VERIFY(success);
	}

	{
		Util::Array<Id32> ids;

		IdAllocator<int, float, Util::String> allocator;

		const int numAllocs = 1024;
		for (SizeT i = 0; i < numAllocs; i++)
		{
			Id32 id = allocator.Alloc();
			ids.Append(id);
			allocator.Get<0>(id) = i;
			allocator.Get<1>(id) = 2.0f;
			allocator.Get<2>(id) = "Test";
		}
		VERIFY(allocator.Size() == numAllocs);

		VERIFY(allocator.Get<1>(Id32(0)) == 2.0f);
		VERIFY(allocator.Get<2>(Id32(0)) == "Test");

        bool success = true;
		for (SizeT i = 0; i < numAllocs; i++)
		{
			success &= (allocator.Get<0>(ids[i]) == i);
		}
        VERIFY(success);

		for (SizeT i = 0; i < numAllocs; i++)
		{
			allocator.Dealloc(ids[i]);
		}

		VERIFY(allocator.Size() == 0);
	}
}

}