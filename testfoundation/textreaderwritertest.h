#ifndef TEST_TEXTREADERWRITERTEST_H
#define TEST_TEXTREADERWRITERTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::TextReaderWriterTest
    
    Test TextReader and TextWriter functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class TextReaderWriterTest : public TestCase
{
    __DeclareClass(TextReaderWriterTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
    