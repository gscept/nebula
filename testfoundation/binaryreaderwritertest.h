#ifndef TEST_BINARYREADERWRITERTEST_H
#define TEST_BINARYREADERWRITERTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::BinaryReaderWriterTest
    
    Test BinaryReader/BinaryWriter functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class BinaryReaderWriterTest : public TestCase
{
    __DeclareClass(BinaryReaderWriterTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
#endif
