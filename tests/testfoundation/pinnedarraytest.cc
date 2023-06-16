//------------------------------------------------------------------------------
//  pinnedarraytest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "util/array.h"
#include "pinnedarraytest.h"

namespace Test
{
__ImplementClass(Test::PinnedArrayTest, 'PART', Test::TestCase);

using namespace Util;


//------------------------------------------------------------------------------
/**
*/
void
PinnedArrayTest::Run()
{
    PinnedArray<int> array0(10e6), array1(10e6), array2(10e6);

    array0.Append(1);
    array0.Append(2);
    array0.Append(3);
    array1.Append(4);
    array1.Append(5);
    array1.Append(6);
    array1.Append(7);

    // size and [] operator
    VERIFY(array0.Size() == 3);
    VERIFY(1 == array0[0]);
    VERIFY(2 == array0[1]);
    VERIFY(3 == array0[2]);

    VERIFY(array1.Size() == 4);
    VERIFY(4 == array1[0]);
    VERIFY(5 == array1[1]);
    VERIFY(6 == array1[2]);
    VERIFY(7 == array1[3]);

    // =, ==, != operator
    VERIFY(array0 != array1);
    array2 = array0;
    VERIFY(array2.Size() == 3);
    VERIFY(1 == array2[0]);
    VERIFY(2 == array2[1]);
    VERIFY(3 == array2[2]);
    VERIFY(array0 == array2);
    array2[2] = 10;
    VERIFY(10 == array2[2]);
    VERIFY(array0 != array2);

    // AppendArray
    array2 = array0;
    array2.AppendArray(array1);
    VERIFY(array2.Size() == 7);
    VERIFY(array2[0] == 1);
    VERIFY(array2[3] == 4);
    VERIFY(array2[6] == 7);

    // Front, Back, IsEmpty, Clear, Erase, FindIndex, Fill
    VERIFY(array2.Front() == 1);
    VERIFY(array2.Back() == 7);
    VERIFY(!array2.IsEmpty());
    array2.EraseIndex(0);
    VERIFY(array2.Size() == 6);
    VERIFY(array2[0] == 2);
    VERIFY(array2[1] == 3);
    VERIFY(array2[5] == 7);
    array2.EraseIndex(5);
    VERIFY(array2.Size() == 5);
    VERIFY(array2[4] == 6);
    VERIFY(array2.FindIndex(4) == 2);
    VERIFY(array2.FindIndex(8) == -1);
    array2.Fill(1, 2, 0);
    VERIFY(array2[0] == 2);
    VERIFY(array2[1] == 0);
    VERIFY(array2[2] == 0);
    VERIFY(array2[3] == 5);
    array2.Clear();
    VERIFY(array2.Size() == 0);
    VERIFY(array2.IsEmpty());

    // Difference, FIXME: this is and always was broken!
    /*
    array0.Clear();
    array1.Clear();
    array0.Append(1); array0.Append(2); array0.Append(3);
    array1.Append(2); array1.Append(3); array1.Append(4);
    array2 = array0.Difference(array1);
    VERIFY(array2.Size() == 2);
    VERIFY(array2[0] == 1);
    VERIFY(array2[1] == 4);
    */

    // test InsertSorted()
    array0.Clear();
    VERIFY(InvalidIndex != array0.InsertSorted(3));
    VERIFY(InvalidIndex != array0.InsertSorted(1));
    VERIFY(InvalidIndex != array0.InsertSorted(4));
    VERIFY(InvalidIndex != array0.InsertSorted(0));
    VERIFY(InvalidIndex != array0.InsertSorted(2));
    VERIFY(InvalidIndex != array0.InsertSorted(10));
    VERIFY(InvalidIndex != array0.InsertSorted(100));
    VERIFY(InvalidIndex != array0.InsertSorted(-2));
    VERIFY(InvalidIndex != array0.InsertSorted(6));
    VERIFY(InvalidIndex != array0.InsertSorted(12));
    VERIFY(InvalidIndex != array0.InsertSorted(12));
    VERIFY(array0.IsSorted());

    // test InsertSorted randomly
    array0.Clear();
    IndexT i;
    for (i = 0; i < 20000; i++)
    {
        array0.InsertSorted(rand());
    }
    VERIFY(array0.IsSorted());

    // test Sort()
    array0.Clear();
    array0.Append(4); array0.Append(2); array0.Append(1); array0.Append(3);
    array0.Sort();
    VERIFY(array0[0] == 1);
    VERIFY(array0[1] == 2);
    VERIFY(array0[2] == 3);
    VERIFY(array0[3] == 4);

    // test BinarySearch
    VERIFY(array0.BinarySearchIndex(1) == 0);
    VERIFY(array0.BinarySearchIndex(2) == 1);
    VERIFY(array0.BinarySearchIndex(3) == 2);
    VERIFY(array0.BinarySearchIndex(4) == 3);
    VERIFY(array0.BinarySearchIndex(5) == -1);
}

}; // namespace Test