//------------------------------------------------------------------------------
//  databasetest.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "databasetest.h"
#include "threading/safequeue.h"
#include "memory/arenaallocator.h"
#include <functional>
#include "jobs/jobs.h"
#include "timing/timer.h"
#include "memdb/database.h"
#include "util/bitfield.h"

using namespace Math;
using namespace Util;

namespace Test
{
__ImplementClass(Test::DatabaseTest, 'MDBT', Test::TestCase);

struct StructTest
{
    int foo = 10;
    bool boo = true;
    Math::float4 f4 = {1,2,3,4};
};

//------------------------------------------------------------------------------
/**
*/
void
DatabaseTest::Run()
{
    Ptr<MemDb::Database> db;
    MemDb::TableId table0;
    MemDb::TableId table1;
    MemDb::TableId table2;
    
    db = MemDb::Database::Create();

    MemDb::PropertyId TestIntId = MemDb::TypeRegistry::Register<int>("TestIntId", int(1));
    MemDb::PropertyId TestFloatId = MemDb::TypeRegistry::Register<float>("TestFloatId", float(20.0f));


    MemDb::PropertyId TestStructId = MemDb::TypeRegistry::Register<StructTest>("TestStructId", StructTest());

    MemDb::TableCreateInfo info = {
        "Table0",
        {
            TestIntId,
            TestFloatId,
            TestStructId
        }
    };

    table0 = db->CreateTable(info);
    
    info = {
        "Table1",
        {
            TestIntId,
            TestFloatId
        }
    };

    table1 = db->CreateTable(info);

    info = {
        "Table2",
        {
            TestStructId,
            TestIntId
        }
    };

    table2 = db->CreateTable(info);

    Util::Array<IndexT> instances;
    
    for (size_t i = 0; i < 10; i++)
    {
        instances.Append(db->AllocateRow(table0));
    }

    // make sure we allocate enough so that we need to grow/reallocate the buffers
    for (size_t i = 0; i < 10000; i++)
    {
        instances.Append(db->AllocateRow(table0));
    }
    
    {
        int* intData = (int*)db->GetBuffer(table0, db->GetColumnId(table0, TestIntId));
        float* floatData = (float*)db->GetBuffer(table0, db->GetColumnId(table0, TestFloatId));
        StructTest* structData = (StructTest*)db->GetBuffer(table0, db->GetColumnId(table0, TestStructId));

        SizeT numRows = db->GetNumRows(table0);
        bool passed = false;
        for (size_t i = 0; i < numRows; i++)
        {
            passed |= (intData[i] == *(int*)MemDb::TypeRegistry::GetDescription(TestIntId)->defVal);
            passed |= (floatData[i] == *(float*)MemDb::TypeRegistry::GetDescription(TestFloatId)->defVal);
        }
    }

    MemDb::TableMask mask = MemDb::TableMask({ TestIntId, 129 });
    MemDb::TableMask mask0 = MemDb::TableMask({ TestIntId, 129 });

    VERIFY(mask == mask0);
    VERIFY(MemDb::TableMask::CheckBits(mask, mask0));

    MemDb::TableMask mask2 = MemDb::TableMask({ TestIntId, TestFloatId, });
    MemDb::TableMask mask3 = MemDb::TableMask({ TestFloatId, TestIntId });
    
    VERIFY(mask2 == mask3);
    VERIFY(MemDb::TableMask::CheckBits(mask2, mask3));

    MemDb::TableMask mask4 = MemDb::TableMask({ TestIntId, TestFloatId, TestStructId });
    MemDb::TableMask mask5 = MemDb::TableMask({ TestFloatId, TestStructId, TestIntId});
    
    VERIFY(mask4 == mask5);
    VERIFY(MemDb::TableMask::CheckBits(mask4, mask5));
    VERIFY(!(mask2 == mask5));
    
    VERIFY(!MemDb::TableMask::CheckBits(mask2, mask5));

    VERIFY(MemDb::TableMask::CheckBits(mask5, mask2));

    MemDb::TableMask mask6 = MemDb::TableMask({ TestStructId });


    // TODO: More robust testing
}

}