#ifndef TEST_LUASERVERTEST_H
#define TEST_LUASERVERTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::LuaServerTest
  
    Test Lua script server functionality.
    
    (C) 2006 Radon Labs GmbH
*/    
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class LuaServerTest : public TestCase
{
    __DeclareClass(LuaServerTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
