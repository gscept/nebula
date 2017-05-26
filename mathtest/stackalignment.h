#pragma once

//------------------------------------------------------------------------------
/**
*/

#include "mathtestutil.h"

namespace Test
{

class TestCase;

//------------------------------------------------------------------------------
/**
*/
template<class T>
void
testStackAlignment16(TestCase *testCase)
{
    class AlignmentTester
    {
    public:
        int testInt;
        char testArray[5];
        T aligned;
    };
    NEBULA3_ALIGN16 char t;
    AlignmentTester t0;
    char array0[5];
    AlignmentTester t1;
    char array1[5];
    AlignmentTester t2;
    // no "unused variable" warnings
    t = '\0';
    array0[0] = '\0';
    array1[0] = '\0';
    // test the alignment of the embedded member
    testCase->Verify( !((size_t)&t0.aligned & 0xF) );
    testCase->Verify( !((size_t)&t1.aligned & 0xF) );
    testCase->Verify( !((size_t)&t2.aligned & 0xF) );
}

} // namespace Test
//------------------------------------------------------------------------------

