#ifndef TEST_FILESERVERTEST_H
#define TEST_FILESERVERTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::FileServerTest
    
    Test IO::FileServer functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class FileServerTest : public TestCase
{
    __DeclareClass(FileServerTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
    