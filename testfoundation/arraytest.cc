//------------------------------------------------------------------------------
//  arraytest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "arraytest.h"

namespace Test
{
__ImplementClass(Test::ArrayTest, 'ARRT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
ArrayTest::Run()
{
    // create some arrays
    Array<int> array0, array1, array2;
    
    array0.Append(1);
    array0.Append(2);
    array0.Append(3);
    array1.Append(4);
    array1.Append(5);
    array1.Append(6);
    array1.Append(7);

    // size and [] operator
    this->Verify(array0.Size() == 3);
    this->Verify(1 == array0[0]);
    this->Verify(2 == array0[1]);
    this->Verify(3 == array0[2]);

    this->Verify(array1.Size() == 4);
    this->Verify(4 == array1[0]);
    this->Verify(5 == array1[1]);
    this->Verify(6 == array1[2]);
    this->Verify(7 == array1[3]);

    // =, ==, != operator
    this->Verify(array0 != array1);
    array2 = array0;
    this->Verify(array2.Size() == 3);
    this->Verify(1 == array2[0]);
    this->Verify(2 == array2[1]);
    this->Verify(3 == array2[2]);
    this->Verify(array0 == array2);
    array2[2] = 10;
    this->Verify(10 == array2[2]);
    this->Verify(array0 != array2);

    // AppendArray
    array2 = array0;
    array2.AppendArray(array1);
    this->Verify(array2.Size() == 7);
    this->Verify(array2[0] == 1);
    this->Verify(array2[3] == 4);
    this->Verify(array2[6] == 7);

    // Front, Back, IsEmpty, Clear, Erase, FindIndex, Fill
    this->Verify(array2.Front() == 1);
    this->Verify(array2.Back() == 7);
    this->Verify(!array2.IsEmpty());
    array2.EraseIndex(0);
    this->Verify(array2.Size() == 6);
    this->Verify(array2[0] == 2);
    this->Verify(array2[1] == 3);
    this->Verify(array2[5] == 7);
    array2.EraseIndex(5);
    this->Verify(array2.Size() == 5);
    this->Verify(array2[4] == 6);
    this->Verify(array2.FindIndex(4) == 2);
    this->Verify(array2.FindIndex(8) == -1);
    array2.Fill(1, 2, 0);
    this->Verify(array2[0] == 2);
    this->Verify(array2[1] == 0);
    this->Verify(array2[2] == 0);
    this->Verify(array2[3] == 5);
    array2.Clear();
    this->Verify(array2.Size() == 0);
    this->Verify(array2.IsEmpty());

    // Difference, FIXME: this is and always was broken!
    /*
    array0.Clear();
    array1.Clear();
    array0.Append(1); array0.Append(2); array0.Append(3);
    array1.Append(2); array1.Append(3); array1.Append(4);
    array2 = array0.Difference(array1);
    this->Verify(array2.Size() == 2);
    this->Verify(array2[0] == 1);
    this->Verify(array2[1] == 4);
    */

    // test InsertSorted()
    array0.Clear();
    this->Verify(InvalidIndex != array0.InsertSorted(3));
    this->Verify(InvalidIndex != array0.InsertSorted(1));
    this->Verify(InvalidIndex != array0.InsertSorted(4));
    this->Verify(InvalidIndex != array0.InsertSorted(0));
    this->Verify(InvalidIndex != array0.InsertSorted(2));
    this->Verify(InvalidIndex != array0.InsertSorted(10));
    this->Verify(InvalidIndex != array0.InsertSorted(100));
    this->Verify(InvalidIndex != array0.InsertSorted(-2));
    this->Verify(InvalidIndex != array0.InsertSorted(6));
    this->Verify(InvalidIndex != array0.InsertSorted(12));
    this->Verify(InvalidIndex != array0.InsertSorted(12));
    this->Verify(array0.IsSorted());

    // test InsertSorted randomly
    array0.Clear();
    IndexT i;
    for (i = 0; i < 20000; i++)
    {
        array0.InsertSorted(rand());
    }
    this->Verify(array0.IsSorted());

    // test Sort()
    array0.Clear();
    array0.Append(4); array0.Append(2); array0.Append(1); array0.Append(3);
    array0.Sort();
    this->Verify(array0[0] == 1);
    this->Verify(array0[1] == 2);
    this->Verify(array0[2] == 3);
    this->Verify(array0[3] == 4);

    // test BinarySearch
    this->Verify(array0.BinarySearchIndex(1) == 0);
    this->Verify(array0.BinarySearchIndex(2) == 1);
    this->Verify(array0.BinarySearchIndex(3) == 2);
    this->Verify(array0.BinarySearchIndex(4) == 3);
    this->Verify(array0.BinarySearchIndex(5) == -1);
}

}; // namespace Test