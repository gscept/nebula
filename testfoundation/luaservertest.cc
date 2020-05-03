//------------------------------------------------------------------------------
//  luaservertest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "luaservertest.h"
//#include "scripting/lua/luaserver.h"

namespace Test
{
__ImplementClass(Test::LuaServerTest, 'luat', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
LuaServerTest::Run()
{
/*
    Ptr<LuaServer> luaServer = LuaServer::Create();
    
    luaServer->Open();
    VERIFY(luaServer->IsOpen());

    luaServer->Eval("i = 3");

    luaServer->Close();
    VERIFY(!luaServer->IsOpen());
*/
}

} // namespace Test