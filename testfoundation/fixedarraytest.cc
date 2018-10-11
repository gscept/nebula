//------------------------------------------------------------------------------
//  fixedarraytest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fixedarraytest.h"
#include "util/fixedarray.h"

namespace Test
{
__ImplementClass(Test::FixedArrayTest, 'FART' /* huhu, he said 'fart' */, Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
FixedArrayTest::Run()
{
    // create a few arrays of different types
    FixedArray<int> intArray;
    FixedArray<String> stringArray1(5);
    FixedArray<String> stringArray2(3, "Three");

    VERIFY(intArray.Size() == 0);
    VERIFY(stringArray1.Size() == 5);
    VERIFY(stringArray2.Size() == 3);

    VERIFY(stringArray2[0] == "Three");
    VERIFY(stringArray2[1] == "Three");
    VERIFY(stringArray2[2] == "Three");
    // NOTE: the following should generate a runtime error
    //VERIFY(stringArray2[3] == "Three");

    stringArray1[0] = "One";
    stringArray1[1] = "Two";
    stringArray1[2] = "Three";
    stringArray1[3] = "Four";
    stringArray1[4] = "Five";
    VERIFY("One" == stringArray1[0]);
    VERIFY("Two" == stringArray1[1]);
    VERIFY("Three" == stringArray1[2]);
    VERIFY("Four" == stringArray1[3]);
    VERIFY("Five" == stringArray1[4]);
    VERIFY(stringArray1 != stringArray2);

    // finding elements in unsorted arrays
    FixedArray<String>::Iterator iter = stringArray1.Find("Three");
    VERIFY(0 != iter);
    VERIFY("Three" == *iter);
    iter++;
    VERIFY("Four" == *iter);
    iter = stringArray1.Find("Ten");
    VERIFY(0 == iter);

    VERIFY(0 == stringArray1.FindIndex("One"));
    VERIFY(1 == stringArray1.FindIndex("Two"));
    VERIFY(4 == stringArray1.FindIndex("Five"));
    VERIFY(InvalidIndex == stringArray1.FindIndex("Ten"));

    // fill range
    stringArray1.Fill(1, 3, "xxx");
    VERIFY(stringArray1[0] == "One");
    VERIFY(stringArray1[1] == "xxx");
    VERIFY(stringArray1[2] == "xxx");
    VERIFY(stringArray1[3] == "xxx");
    VERIFY(stringArray1[4] == "Five");

    // clear
    stringArray1.Fill("Clear");
    VERIFY(stringArray1[0] == "Clear");
    VERIFY(stringArray1[1] == "Clear");
    VERIFY(stringArray1[2] == "Clear");
    VERIFY(stringArray1[3] == "Clear");
    VERIFY(stringArray1[4] == "Clear");

    // assignment, equality and inequality
    stringArray1 = stringArray2;
    VERIFY(stringArray1 == stringArray2);
    stringArray1[1] = "xxx";
    VERIFY(stringArray1 != stringArray2);

    // sorting and binary search
    intArray.SetSize(5);
    intArray[0] = 100;
    intArray[1] = 50;
    intArray[2] = 80;
    intArray[3] = 101;
    intArray[4] = 1;
    intArray.Sort();
    VERIFY(1 == intArray[0]);
    VERIFY(50 == intArray[1]);
    VERIFY(80 == intArray[2]);
    VERIFY(100 == intArray[3]);
    VERIFY(101 == intArray[4]);
    VERIFY(0 == intArray.BinarySearchIndex(1));
    VERIFY(1 == intArray.BinarySearchIndex(50));
    VERIFY(2 == intArray.BinarySearchIndex(80));
    VERIFY(3 == intArray.BinarySearchIndex(100));
    VERIFY(4 == intArray.BinarySearchIndex(101));
    VERIFY(InvalidIndex == intArray.BinarySearchIndex(4));
}

}; 