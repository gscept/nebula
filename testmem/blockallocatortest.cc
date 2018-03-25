//------------------------------------------------------------------------------
// blockallocatortest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "blockallocatortest.h"
#include "memory/sliceallocatorpool.h"
#include "memory/chunkallocator.h"
#include "timing/timer.h"
#include "io/console.h"

struct ChunkTypeOne
{
	int a;
};

struct ChunkTypeTwo
{
	char a;
};

struct ChunkTypeThree
{
	double a;
};

struct TestType
{
	int a;
	float b;
	char c;
};

class Foobar : public Core::RefCounted
{
	__DeclareClass(Foobar);
public:
	Foobar() {};
	~Foobar() {};

	int a;
	float b;
	char c;
};

__ImplementClass(Foobar, 'FOOB', Core::RefCounted);

using namespace Timing;
using namespace Memory;
namespace Test
{

__ImplementClass(Test::BlockAllocatorTest, 'BLKT', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
BlockAllocatorTest::Run()
{
	ChunkAllocator chunkAllocator(0x8);
	ChunkTypeOne* a = chunkAllocator.Alloc<ChunkTypeOne>();
	ChunkTypeTwo* b = chunkAllocator.Alloc<ChunkTypeTwo>();
	ChunkTypeThree* c = chunkAllocator.Alloc<ChunkTypeThree>();

	double* d = chunkAllocator.Alloc<double>();
	int* f = chunkAllocator.Alloc<int>();
}

} // namespace Test