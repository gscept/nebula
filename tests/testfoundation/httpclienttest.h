#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::HttpClientTest
    
    Test Http::HttpClient functionality.
    
    (C) 2009 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class HttpClientTest : public TestCase
{
    __DeclareClass(HttpClientTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
