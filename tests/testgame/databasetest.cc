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
#include "game/category.h"

using namespace Math;
using namespace Util;
using namespace MemDb;

namespace Test
{
__ImplementClass(Test::DatabaseTest, 'MDBT', Test::TestCase);

struct IntTest
{
    int value = 1;
    DECLARE_COMPONENT;
};

struct FloatTest
{
    float value = 20.0f;
    DECLARE_COMPONENT;
};

struct StructTest
{
    int foo = 10;
    bool boo = true;
    Math::float4 f4 = {1, 2, 3, 4};
    DECLARE_COMPONENT;
};

DEFINE_COMPONENT(IntTest);
DEFINE_COMPONENT(FloatTest);
DEFINE_COMPONENT(StructTest);

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

    AttributeId TestIntId = TypeRegistry::Register<IntTest>("TestIntId", IntTest());
    AttributeId TestFloatId = TypeRegistry::Register<FloatTest>("TestFloatId", FloatTest());
    AttributeId TestStructId = TypeRegistry::Register<StructTest>("TestStructId", StructTest());
    AttributeId TestNonTypedComponent = TypeRegistry::Register("TestNonTypedComponent", 0, nullptr);

    for (int i = 0; i < 2; i++)
    {
        db = Database::Create();

        {
            TableCreateInfo info;
            info.name = "Table0";
            AttributeId const cids[] = {TestIntId, TestFloatId, TestStructId};
            info.components = cids;
            info.numComponents = sizeof(cids) / sizeof(AttributeId);
            table0 = db->CreateTable(info);
        }

        {
            TableCreateInfo info;
            info.name = "Table1";
            AttributeId const cids[] = {TestIntId, TestFloatId};
            info.components = cids;
            info.numComponents = sizeof(cids) / sizeof(AttributeId);
            table1 = db->CreateTable(info);
        };

        {
            TableCreateInfo info;
            info.name = "Table2";
            AttributeId const cids[] = {TestStructId, TestIntId, TestNonTypedComponent};
            info.components = cids;
            info.numComponents = sizeof(cids) / sizeof(AttributeId);
            table2 = db->CreateTable(info);
        };

        {
            TableCreateInfo info;
            info.name = "Table3";
            AttributeId const cids[] = {TestNonTypedComponent};
            info.components = cids;
            info.numComponents = sizeof(cids) / sizeof(AttributeId);
            table3 = db->CreateTable(info);
        };

        Util::Array<RowId> instances;

        Table& tbl0 = db->GetTable(table0);
        Table& tbl1 = db->GetTable(table1);
        Table& tbl2 = db->GetTable(table2);
        Table& tbl3 = db->GetTable(table3);

        for (size_t i = 0; i < 10; i++)
        {
            instances.Append(tbl0.AddRow());
        }

        // 10 rows obviously
        VERIFY(tbl0.GetNumRows() == 10);

        for (auto i : instances)
        {
            tbl0.RemoveRow(i);
        }
        instances.Clear();

        // Still 10 rows
        // RemoveRow shouldn't immediately erase the row. It only adds it to free ids. You must defragment to reduce num rows.
        VERIFY(tbl0.GetNumRows() == 10);

        // make sure we allocate enough so that we need to create multiple partitions
        const size_t numInstA = Table::Partition::CAPACITY * 100;
        for (size_t i = 0; i < numInstA; i++)
        {
            instances.Append(tbl0.AddRow());
        }

        // Since we've not executed defragment since previous test, we should have reused the old rows
        VERIFY(tbl0.GetNumRows() == numInstA);

        { // make sure default values are correctly assigned to all rows
            uint16_t const numParts = tbl0.GetNumPartitions();

            bool passed = false;
            for (uint16_t partId = 0; partId < numParts; partId++)
            {
                int* intData = (int*)tbl0.GetBuffer(partId, tbl0.GetAttributeIndex(TestIntId));
                float* floatData = (float*)tbl0.GetBuffer(partId, tbl0.GetAttributeIndex(TestFloatId));
                StructTest* structData = (StructTest*)tbl0.GetBuffer(partId, tbl0.GetAttributeIndex(TestStructId));

                Table::Partition* partition = tbl0.GetPartition(partId);
                SizeT const numRows = partition->numRows;
                for (size_t i = 0; i < numRows; i++)
                {
                    passed |= (intData[i] == *(int*)TypeRegistry::GetDescription(TestIntId)->defVal);
                    passed |= (floatData[i] == *(float*)TypeRegistry::GetDescription(TestFloatId)->defVal);
                }
            }
            VERIFY(passed);
        }

        for (auto i : instances)
        {
            tbl0.RemoveRow(i);
        }

        instances.Clear();

        // RemoveRow shouldn't immediately erase the row. It only adds it to free ids. You must defragment to reduce num rows.
        VERIFY(tbl0.GetNumRows() == numInstA);
        instances.Append(tbl0.AddRow());
        instances.Append(tbl0.AddRow());
        instances.Append(tbl0.AddRow());

        tbl0.Defragment([](MemDb::Table::Partition*, MemDb::RowId, MemDb::RowId) {});

        // Make sure defragment reduces num rows
        VERIFY(tbl0.GetNumRows() == 3);

        tbl0.Clean();

        // Make sure cleaning the table reduces num rows
        VERIFY(tbl0.GetNumRows() == 0);

        instances.Clear();

        for (size_t i = 0; i < 10; i++)
        {
            instances.Append(tbl1.AddRow());
        }

        const auto tbl1cid = tbl1.GetAttributeIndex(TestFloatId);
        for (auto i : instances)
        {
            // Verify default values are set correctly
            VERIFY(
                *((float*)tbl1.GetValuePointer(tbl1cid, i)) ==
                *(float*)TypeRegistry::GetDescription(TestFloatId)->defVal
            );
        }

        tbl1.Clean();
        instances.Clear();

        for (size_t i = 0; i < 10; i++)
        {
            instances.Append(tbl2.AddRow());
        }

        VERIFY(tbl2.GetNumRows() == 10);

        const auto tbl2cid = tbl2.GetAttributeIndex(TestNonTypedComponent);
        VERIFY(tbl2.HasAttribute(TestNonTypedComponent));
        VERIFY(tbl2.GetBuffer(0, tbl2cid) == nullptr);

        tbl2.Clean();
        instances.Clear();

        for (size_t i = 0; i < 10; i++)
        {
            instances.Append(tbl3.AddRow());
        }

        VERIFY(tbl3.GetNumRows() == 10);

        const auto tbl3cid = tbl3.GetAttributeIndex(TestNonTypedComponent);
        VERIFY(tbl3.HasAttribute(TestNonTypedComponent));
        VERIFY(tbl3.GetBuffer(0, tbl3cid) == nullptr);

        for (auto i : instances)
        {
            tbl3.RemoveRow(i);
        }

        tbl3.Defragment([](MemDb::Table::Partition*, MemDb::RowId, MemDb::RowId) {});

        tbl3.Clean();
        instances.Clear();

        for (size_t i = 0; i < 10; i++)
        {
            tbl0.AddRow();
            tbl1.AddRow();
            tbl2.AddRow();
            tbl3.AddRow();
        }

        {
            FilterSet filter = FilterSet({TestIntId, TestNonTypedComponent});
            Dataset data = db->Query(filter);
            VERIFY(data.tables.Size() == 1);
        }

        {
            FilterSet filter = FilterSet({TestNonTypedComponent});
            Dataset data = db->Query(filter);
            VERIFY(data.tables.Size() == 2);
        }

        {
            FilterSet filter = FilterSet(
                {// inclusive
                 TestNonTypedComponent},
                {// exclusive
                 TestIntId}
            );

            Dataset data = db->Query(filter);
            VERIFY(data.tables.Size() == 1);
        }

        // Test copying a database
        Ptr<Database> dbCopy = Database::Create();
        db->Copy(dbCopy);

        VERIFY(dbCopy->GetNumTables() == db->GetNumTables());
        // note that the table id is the same for both databases in this case, but doesn't have to be if the original table has deleted tables
        VERIFY(dbCopy->GetTable(table0).GetNumRows() == db->GetTable(table0).GetNumRows());
        VERIFY(dbCopy->GetTable(table0).GetAttributes().Size() == db->GetTable(table0).GetAttributes().Size());
    }

    // Test table signatures

    TableSignature mask = TableSignature({TestIntId, 129});
    TableSignature mask0 = TableSignature({TestIntId, 129});

    VERIFY(mask == mask0);
    VERIFY(TableSignature::CheckBits(mask, mask0));

    TableSignature mask2 = TableSignature({
        TestIntId,
        TestFloatId,
    });
    TableSignature mask3 = TableSignature({TestFloatId, TestIntId});

    VERIFY(mask2 == mask3);
    VERIFY(TableSignature::CheckBits(mask2, mask3));

    TableSignature mask4 = TableSignature({TestIntId, TestFloatId, TestStructId});
    TableSignature mask5 = TableSignature({TestFloatId, TestStructId, TestIntId});

    VERIFY(mask4 == mask5);
    VERIFY(TableSignature::CheckBits(mask4, mask5));
    VERIFY(!(mask2 == mask5));

    VERIFY(!TableSignature::CheckBits(mask2, mask5));

    VERIFY(TableSignature::CheckBits(mask5, mask2));

    TableSignature mask6 = TableSignature({TestIntId, TestFloatId, TestStructId});
    TableSignature mask7 = TableSignature({TestFloatId});
    TableSignature mask8 = TableSignature({TestIntId});

    VERIFY(TableSignature::HasAny(mask6, mask7) == true);
    VERIFY(TableSignature::HasAny(mask8, mask7) == false);

    VERIFY(!TableSignature::CheckBits({1}, {0, 1, 2}));
    VERIFY(!TableSignature::CheckBits({0}, {0, 1, 2}));
    VERIFY(TableSignature::CheckBits({0, 1, 2}, {0, 1, 2}));
    VERIFY(TableSignature::CheckBits({0, 1, 2}, {0, 2}));
    VERIFY(TableSignature::CheckBits({0, 1, 2}, {0, 1}));
    VERIFY(TableSignature::CheckBits({0, 1, 2}, {0}));
    VERIFY(TableSignature::CheckBits({0, 1, 2}, {1}));
    VERIFY(TableSignature::CheckBits({0, 1, 2}, {2}));
    VERIFY(!TableSignature::CheckBits({0, 1, 2}, {3}));
    VERIFY(!TableSignature::CheckBits({0, 1, 2}, {4}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {0}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {1}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {2}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {3}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {4}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {5}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {6}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {7}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {8}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {9}));
    VERIFY(TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {10}));
    VERIFY(!TableSignature::CheckBits({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {11}));

    {
        const ushort num = 8096;
        Util::FixedArray<AttributeId> da(num);
        for (ushort i = 0; i < num; i++)
        {
            da[i] = i;
        }

        TableSignature s = da;
        VERIFY(TableSignature::CheckBits(s, {0}));
        VERIFY(TableSignature::CheckBits(s, {1}));
        VERIFY(TableSignature::CheckBits(s, {2}));
        VERIFY(TableSignature::CheckBits(s, {3}));
        VERIFY(TableSignature::CheckBits(s, {4}));
        VERIFY(TableSignature::CheckBits(s, {0, 1, 2, 3, 4}));
        VERIFY(TableSignature::CheckBits(s, {0, 1023}));
        VERIFY(TableSignature::CheckBits(s, {0, 8000}));
        VERIFY(TableSignature::CheckBits(s, s));
    }

    {
        for (ushort i = 2; i < 10000; i += 7)
        {
            TableSignature s = {(AttributeId)i};
            VERIFY(!TableSignature::CheckBits(s, {1}));
        }
    }

    TableSignature signature = {1, 6, 250, 1010};

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

} // namespace Test