//------------------------------------------------------------------------------
//  databasetest.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "databasetest.h"
#include "game/database/database.h"
#include "threading/safequeue.h"
#include "memory/arenaallocator.h"
#include <functional>
#include "jobs/jobs.h"
#include "timing/timer.h"
#include "game/database/attribute.h"

using namespace Game;
using namespace Math;
using namespace Util;

namespace Attr
{
__DefineAttribute(TestInt);
__DefineAttribute(TestString);
__DefineAttribute(TestVec4);
__DefineAttribute(TestFloat);
}

namespace Test
{
__ImplementClass(Test::DatabaseTest, 'GDBT', Test::TestCase);

struct t
{
    int foo = 10;
    bool boo = true;
    Math::float4 f4 = {1,2,3,4};
    Util::String gryrf = "alfons_0123456789123456789123456789";
};

//------------------------------------------------------------------------------
/**
*/
void
DatabaseTest::Run()
{
    Ptr<Db::Database> db;
    Db::TableId table0;
    Db::TableId table1;
    Db::TableId table2;
    
    db = Db::Database::Create();

    //Db::StateDescriptor transformDescriptor = db->GetStateDescriptor("Transform"_atm);
    //Db::TableCreateInfo info = {
    //    "StateTable",
    //    {
    //        transformDescriptor
    //    }
    //};

    Db::TableCreateInfo info = {
        "Table0",
        {
            Attr::Runtime::TestIntId,
            Attr::Runtime::TestStringId,
            Attr::Runtime::TestVec4Id,
            Attr::Runtime::TestFloatId
        }
    };

    table0 = db->CreateTable(info);
    
    info = {
        "Table1",
        {
            Attr::Runtime::TestIntId,
            Attr::Runtime::TestStringId
        }
    };

    table1 = db->CreateTable(info);

    info = {
        "Table2",
        {
            Attr::Runtime::TestVec4Id,
            Attr::Runtime::TestFloatId
        }
    };

    table2 = db->CreateTable(info);

    Util::Array<IndexT> instances;
    
    for (size_t i = 0; i < 10; i++)
    {
        instances.Append(db->AllocateRow(table0));
    }

    Db::ColumnData<int> intData = db->GetColumnData<Attr::TestInt>(table0);
    Db::ColumnData<Util::String> stringData = db->GetColumnData<Attr::TestString>(table0);
    Db::ColumnData<Math::vec4> float4Data = db->GetColumnData<Attr::TestVec4>(table0);
    Db::ColumnData<float> floatData = db->GetColumnData<Attr::TestFloat>(table0);
    
    // make sure we allocate enough so that we need to grow/reallocate the buffers
    for (size_t i = 0; i < 10000; i++)
    {
        instances.Append(db->AllocateRow(table0));
    }
    
    SizeT numRows = db->GetNumRows(table0);
    bool passed = false;
    for (size_t i = 0; i < numRows; i++)
    {
        passed |= (intData[i] == Attr::TestInt::DefaultValue());
        passed |= (stringData[i] == Attr::TestString::DefaultValue());
        passed |= (float4Data[i] == Attr::TestVec4::DefaultValue());
        passed |= (floatData[i] == Attr::TestFloat::DefaultValue());
    }

    VERIFY(passed);

    Db::ColumnData<t> intState = db->AddStateColumn<t>(table0, "intState"_atm);

    VERIFY(true);
  
    Db::StateDescriptor transformDescriptor = db->CreateStateDescriptor<Math::mat4>("Transform"_atm);
    db->AddState(table0, transformDescriptor);
    Db::ColumnData<Math::mat4> transformStates = db->GetStateColumn<Math::mat4>(table0, transformDescriptor);

}

}