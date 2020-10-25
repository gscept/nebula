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

    MemDb::ColumnDescriptor TestIntId = MemDb::TypeRegistry::Register<int>("TestIntId", int(1));
    MemDb::ColumnDescriptor TestFloatId = MemDb::TypeRegistry::Register<float>("TestFloatId", float(20.0f));
    MemDb::ColumnDescriptor TestStructId = MemDb::TypeRegistry::Register<StructTest>("TestStructId", StructTest());

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

    MemDb::ColumnView<int> intData = db->GetColumnView<int>(table0, TestIntId);
    MemDb::ColumnView<float> floatData = db->GetColumnView<float>(table0, TestFloatId);
    MemDb::ColumnView<StructTest> structData = db->GetColumnView<StructTest>(table0, TestStructId);
    
    // make sure we allocate enough so that we need to grow/reallocate the buffers
    for (size_t i = 0; i < 10000; i++)
    {
        instances.Append(db->AllocateRow(table0));
    }
    
    SizeT numRows = db->GetNumRows(table0);
    bool passed = false;
    for (size_t i = 0; i < numRows; i++)
    {
        passed |= (intData[i] == *(int*)MemDb::TypeRegistry::GetDescription(TestIntId)->defVal);
        passed |= (floatData[i] == *(float*)MemDb::TypeRegistry::GetDescription(TestFloatId)->defVal);
    }

    VERIFY(passed);

    // TODO: More robust testing
}

}