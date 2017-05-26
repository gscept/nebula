#ifndef TEST_XMLREADERWRITERTEST_H
#define TEST_XMLREADERWRITERTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::XmlReaderWriterTest
    
    Test XmlReader/XmlWriter functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class XmlReaderWriterTest : public TestCase
{
    __DeclareClass(XmlReaderWriterTest);
public:
    /// run the test
    virtual void Run();
};

}
//------------------------------------------------------------------------------
#endif

