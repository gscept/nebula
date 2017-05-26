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

    this->Verify(intArray.Size() == 0);
    this->Verify(stringArray1.Size() == 5);
    this->Verify(stringArray2.Size() == 3);

    this->Verify(stringArray2[0] == "Three");
    this->Verify(stringArray2[1] == "Three");
    this->Verify(stringArray2[2] == "Three");
    // NOTE: the following should generate a runtime error
    //this->Verify(stringArray2[3] == "Three");

    stringArray1[0] = "One";
    stringArray1[1] = "Two";
    stringArray1[2] = "Three";
    stringArray1[3] = "Four";
    stringArray1[4] = "Five";
    this->Verify("One" == stringArray1[0]);
    this->Verify("Two" == stringArray1[1]);
    this->Verify("Three" == stringArray1[2]);
    this->Verify("Four" == stringArray1[3]);
    this->Verify("Five" == stringArray1[4]);
    this->Verify(stringArray1 != stringArray2);

    // finding elements in unsorted arrays
    FixedArray<String>::Iterator iter = stringArray1.Find("Three");
    this->Verify(0 != iter);
    this->Verify("Three" == *iter);
    iter++;
    this->Verify("Four" == *iter);
    iter = stringArray1.Find("Ten");
    this->Verify(0 == iter);

    this->Verify(0 == stringArray1.FindIndex("One"));
    this->Verify(1 == stringArray1.FindIndex("Two"));
    this->Verify(4 == stringArray1.FindIndex("Five"));
    this->Verify(InvalidIndex == stringArray1.FindIndex("Ten"));

    // fill range
    stringArray1.Fill(1, 3, "xxx");
    this->Verify(stringArray1[0] == "One");
    this->Verify(stringArray1[1] == "xxx");
    this->Verify(stringArray1[2] == "xxx");
    this->Verify(stringArray1[3] == "xxx");
    this->Verify(stringArray1[4] == "Five");

    // clear
    stringArray1.Fill("Clear");
    this->Verify(stringArray1[0] == "Clear");
    this->Verify(stringArray1[1] == "Clear");
    this->Verify(stringArray1[2] == "Clear");
    this->Verify(stringArray1[3] == "Clear");
    this->Verify(stringArray1[4] == "Clear");

    // assignment, equality and inequality
    stringArray1 = stringArray2;
    this->Verify(stringArray1 == stringArray2);
    stringArray1[1] = "xxx";
    this->Verify(stringArray1 != stringArray2);

    // sorting and binary search
    intArray.SetSize(5);
    intArray[0] = 100;
    intArray[1] = 50;
    intArray[2] = 80;
    intArray[3] = 101;
    intArray[4] = 1;
    intArray.Sort();
    this->Verify(1 == intArray[0]);
    this->Verify(50 == intArray[1]);
    this->Verify(80 == intArray[2]);
    this->Verify(100 == intArray[3]);
    this->Verify(101 == intArray[4]);
    this->Verify(0 == intArray.BinarySearchIndex(1));
    this->Verify(1 == intArray.BinarySearchIndex(50));
    this->Verify(2 == intArray.BinarySearchIndex(80));
    this->Verify(3 == intArray.BinarySearchIndex(100));
    this->Verify(4 == intArray.BinarySearchIndex(101));
    this->Verify(InvalidIndex == intArray.BinarySearchIndex(4));
}

}; 