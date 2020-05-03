#ifndef TEST_ZIPFSTEST_H
#define TEST_ZIPFSTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::ZipFSTest
    
    Test lowlevel zip filesystem functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ZipFSTest : public TestCase
{
    __DeclareClass(ZipFSTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
#endif

    