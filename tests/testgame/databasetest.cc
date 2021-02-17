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
using namespace MemDb;

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
    Ptr<Database> db;
    TableId table0;
    TableId table1;
    TableId table2;
    TableId table3;

    PropertyId TestIntId = TypeRegistry::Register<int>("TestIntId", int(1));
    PropertyId TestFloatId = TypeRegistry::Register<float>("TestFloatId", float(20.0f));
    PropertyId TestStructId = TypeRegistry::Register<StructTest>("TestStructId", StructTest());
    PropertyId TestNonTypedProperty = TypeRegistry::Register("TestNonTypedProperty", 0, nullptr);

    for (int i = 0; i < 2; i++)
    {
        db = Database::Create();

        {
            TableCreateInfo info;
            info.name = "Table0";
            PropertyId pids[] = {
                    TestIntId,
                    TestFloatId,
                    TestStructId
            };
            info.properties = pids;
            info.numProperties = 3;
            table0 = db->CreateTable(info);
        }

        {
            TableCreateInfo info;
            info.name = "Table1";
            PropertyId pids[] = {
                TestIntId,
                TestFloatId
            };
            info.properties = pids;
            info.numProperties = 2;
            table1 = db->CreateTable(info);
        };

        {
            TableCreateInfo info;
            info.name = "Table2";
            PropertyId pids[] = {
                TestStructId,
                TestIntId,
                TestNonTypedProperty
            };
            info.properties = pids;
            info.numProperties = 3;
            table2 = db->CreateTable(info);
        };

        {
            TableCreateInfo info;
            info.name = "Table3";
            PropertyId pids[] = {
                TestNonTypedProperty
            };
            info.properties = pids;
            info.numProperties = 1;
            table3 = db->CreateTable(info);
        };

        Util::Array<IndexT> instances;

        for (size_t i = 0; i < 10; i++)
        {
            instances.Append(db->AllocateRow(table0));
        }

        for (auto i : instances)
        {
            db->DeallocateRow(table0, i);
        }
        instances.Clear();

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
                passed |= (intData[i] == *(int*)TypeRegistry::GetDescription(TestIntId)->defVal);
                passed |= (floatData[i] == *(float*)TypeRegistry::GetDescription(TestFloatId)->defVal);
            }
            VERIFY(passed);
        }

        for (auto i : instances)
        {
            db->DeallocateRow(table0, i);
        }

        instances.Clear();

        VERIFY(db->GetNumRows(table0) == 10000);

        instances.Append(db->AllocateRow(table0));
        instances.Append(db->AllocateRow(table0));
        instances.Append(db->AllocateRow(table0));

        VERIFY(db->GetNumRows(table0) == 10000);

        db->Defragment(table0, [](IndexT, IndexT) {});

        VERIFY(db->GetNumRows(table0) == 3);

        for (auto i : instances)
            VERIFY(i < 10010);

        db->Clean(table0);

        VERIFY(db->GetNumRows(table0) == 0);

        instances.Clear();

        for (size_t i = 0; i < 10; i++)
        {
            instances.Append(db->AllocateRow(table1));
        }

        const auto tbl1cid = db->GetColumnId(table1, TestFloatId);
        for (auto i : instances)
        {
            VERIFY(*((float*)db->GetValuePointer(table1, tbl1cid, i)) == *(float*)TypeRegistry::GetDescription(TestFloatId)->defVal);
        }

        db->Clean(table1);
        instances.Clear();

        for (size_t i = 0; i < 10; i++)
        {
            instances.Append(db->AllocateRow(table2));
        }

        VERIFY(db->GetNumRows(table2) == 10);

        const auto tbl2cid = db->GetColumnId(table2, TestNonTypedProperty);
        VERIFY(tbl2cid == InvalidIndex);
        VERIFY(db->HasProperty(table2, TestNonTypedProperty));

        db->Clean(table2);
        instances.Clear();

        for (size_t i = 0; i < 10; i++)
        {
            instances.Append(db->AllocateRow(table3));
        }

        VERIFY(db->GetNumRows(table3) == 10);

        const auto tbl3cid = db->GetColumnId(table3, TestNonTypedProperty);
        VERIFY(tbl3cid == InvalidIndex);
        VERIFY(db->HasProperty(table3, TestNonTypedProperty));

        for (auto i : instances)
        {
            db->DeallocateRow(table3, i);
        }

        db->Defragment(table3, [](IndexT, IndexT) {});

        db->Clean(table3);
        instances.Clear();

        for (size_t i = 0; i < 10; i++)
        {
            db->AllocateRow(table0);
            db->AllocateRow(table1);
            db->AllocateRow(table2);
            db->AllocateRow(table3);
        }

        {
            FilterSet filter = FilterSet({ TestIntId,
                                           TestNonTypedProperty
                                         });
            Dataset data = db->Query(filter);
            VERIFY(data.tables.Size() == 1);
        }

        {
            FilterSet filter = FilterSet({ TestNonTypedProperty });
            Dataset data = db->Query(filter);
            VERIFY(data.tables.Size() == 2);
        }

        {
            FilterSet filter = FilterSet(
                { // inclusive
                    TestNonTypedProperty
                },
            { // exclusive
                TestIntId
            }
            );

            Dataset data = db->Query(filter);
            VERIFY(data.tables.Size() == 1);
        }

        // Test copying a database
        Ptr<Database> dbCopy = Database::Create();
        db->Copy(dbCopy);

        VERIFY(dbCopy->GetNumTables() == db->GetNumTables());
        // note that the table id is the same for both databases in this case, but doesn't have to be if the original table has deleted tables
        VERIFY(dbCopy->GetNumRows(table0) == db->GetNumRows(table0));
        VERIFY(dbCopy->GetTable(table0).columns.Size() == db->GetTable(table0).columns.Size());

    }

    // Test table signatures

    TableSignature mask = TableSignature({ TestIntId, 129 });
    TableSignature mask0 = TableSignature({ TestIntId, 129 });

    VERIFY(mask == mask0);
    VERIFY(TableSignature::CheckBits(mask, mask0));

    TableSignature mask2 = TableSignature({ TestIntId, TestFloatId, });
    TableSignature mask3 = TableSignature({ TestFloatId, TestIntId });
    
    VERIFY(mask2 == mask3);
    VERIFY(TableSignature::CheckBits(mask2, mask3));

    TableSignature mask4 = TableSignature({ TestIntId, TestFloatId, TestStructId });
    TableSignature mask5 = TableSignature({ TestFloatId, TestStructId, TestIntId});
    
    VERIFY(mask4 == mask5);
    VERIFY(TableSignature::CheckBits(mask4, mask5));
    VERIFY(!(mask2 == mask5));
    
    VERIFY(!TableSignature::CheckBits(mask2, mask5));

    VERIFY(TableSignature::CheckBits(mask5, mask2));

    TableSignature mask6 = TableSignature({ TestIntId, TestFloatId, TestStructId });
    TableSignature mask7 = TableSignature({ TestFloatId });
    TableSignature mask8 = TableSignature({ TestIntId });

    VERIFY(TableSignature::HasAny(mask6, mask7) == true);
    VERIFY(TableSignature::HasAny(mask8, mask7) == false);

    VERIFY(!TableSignature::CheckBits({ 1 }, { 0, 1, 2 }));
    VERIFY(!TableSignature::CheckBits({ 0 }, { 0, 1, 2 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2 }, { 0, 1, 2 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2 }, { 0, 2 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2 }, { 0, 1 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2 }, { 0 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2 }, { 1 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2 }, { 2 }));
    VERIFY(!TableSignature::CheckBits({ 0, 1, 2 }, { 3 }));
    VERIFY(!TableSignature::CheckBits({ 0, 1, 2 }, { 4 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 0 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 1 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 2 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 3 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 4 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 5 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 6 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 7 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 8 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 9 }));
    VERIFY(TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 10 }));
    VERIFY(!TableSignature::CheckBits({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 11 }));

    {
        const ushort num = 8096;
        Util::FixedArray<PropertyId> da(num);
        for (ushort i = 0; i < num; i++)
        {
            da[i] = i;
        }

        TableSignature s = da;
        VERIFY(TableSignature::CheckBits(s, { 0 }));
        VERIFY(TableSignature::CheckBits(s, { 1 }));
        VERIFY(TableSignature::CheckBits(s, { 2 }));
        VERIFY(TableSignature::CheckBits(s, { 3 }));
        VERIFY(TableSignature::CheckBits(s, { 4 }));
        VERIFY(TableSignature::CheckBits(s, { 0,1,2,3,4 }));
        VERIFY(TableSignature::CheckBits(s, { 0, 1023 }));
        VERIFY(TableSignature::CheckBits(s, { 0, 8000 }));
        VERIFY(TableSignature::CheckBits(s, s));
    }

    {
        for (ushort i = 2; i < 10000; i += 7)
        {
            TableSignature s = { (PropertyId)i };
            VERIFY(!TableSignature::CheckBits(s, { 1 }));
        }
    }

    TableSignature signature = { 1, 6, 250, 1010 };

    VERIFY(signature.IsSet(1));
    VERIFY(signature.IsSet(6));
    VERIFY(signature.IsSet(250));
    VERIFY(signature.IsSet(1010));
    VERIFY(!signature.IsSet(0));
    VERIFY(!signature.IsSet(2));
    VERIFY(!signature.IsSet(8));
    VERIFY(!signature.IsSet(9));
    VERIFY(!signature.IsSet(255));
    VERIFY(!signature.IsSet(256));
    VERIFY(!signature.IsSet(2000));
}

}