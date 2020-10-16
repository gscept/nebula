#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::EntitySystemTest

    Tests Game::Database functionality.

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"
#include "game/entity.h"
#include "game/messaging/message.h"

//------------------------------------------------------------------------------
namespace Test
{

__DeclareMsg(TestMsg, 'tsMs', int, float);

class EntitySystemTest : public TestCase
{
    __DeclareClass(EntitySystemTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
