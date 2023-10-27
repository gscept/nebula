//------------------------------------------------------------------------------
//  querybenchmark.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "querybenchmark.h"
#include "memdb/table.h"
#include "memdb/attributeregistry.h"
#include "memdb/database.h"
#include "game/category.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::QueryBenchmark, 'QMPB', Benchmarking::Benchmark);

using namespace Timing;
using namespace MemDb;

uint PsuedoRand()
{
    // XORshift128
    static uint x = 123456789;
    static uint y = 362436069;
    static uint z = 521288629;
    static uint w = 88675123;
    uint t;
    t = x ^ (x << 11);
    x = y;
    y = z;
    z = w;
    return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}

template<size_t I>
struct IntTest
{
    int value = I;
};

template <typename T, T... ints>
void
RegisterAttributes(Util::Array<AttributeId>& attributes, std::integer_sequence<T, ints...> int_seq)
{
    (
        attributes.Append(
            AttributeRegistry::Register<IntTest<ints>>(
                Util::String("BenchType_") + Util::String::FromInt(ints),
                IntTest<ints>()
            )
        )
    , ...);
}

//------------------------------------------------------------------------------
/**
*/
void
QueryBenchmark::Run(Timer& timer)
{   
    // Create a database
    Ptr<Database> db = Database::Create();
    
    // Setup a multitude of descriptors
    Util::Array<AttributeId> d;
    constexpr SizeT numDescs = 1024;
    RegisterAttributes(d, std::make_index_sequence<numDescs> {});

    using DA = Util::FixedArray<AttributeId>;

    // Create a couple of tables
    for (int i = 0; i < Database::MAX_NUM_TABLES; i++)
    {
        TableCreateInfo info;
        info.name = "Table_" + Util::String::FromInt(i);

        const SizeT numColumns = (numDescs / 24);
        DA da = DA(numColumns);
        for (int a = 0; a < numColumns; a++)
        {
            IndexT descriptorIndex;
            do
            {
                descriptorIndex = PsuedoRand() % numDescs;
            } while (da.FindIndex(d[descriptorIndex]) != InvalidIndex);
            
            da[a] = d[descriptorIndex];
        }

        info.attributeIds = da.Begin();
        info.numAttributes = da.Size();
        db->CreateTable(info);
    }

    Util::Array<FilterSet> filters;
    const SizeT numQueries = 100000;
    filters.Reserve(numQueries);
    Timer t;
    for (int i = 0; i < numQueries; i++)
    {
        const SizeT numBitsSet = (numDescs / 8);
        DA da = DA(numBitsSet);
        for (int a = 0; a < numBitsSet; a++)
        {
            IndexT descriptorIndex = PsuedoRand() % numDescs;
            da[a] = d[descriptorIndex];
        }
        t.Start();
        FilterSet filter(da);
        t.Stop();
        filters.Append(filter);
    }

    n_printf("Number of queries: %i\n", numQueries);


    n_printf("Signature generation time: %f\n", t.GetTime());

    timer.Start();
    for (int i = 0; i < numQueries; i++)
    {
        volatile Dataset data = db->Query(filters[i]);
    }
    timer.Stop();
}

} // namespace Benchmarking