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

//------------------------------------------------------------------------------
/**
    Bookkeeping for the component decay callback, which the test application registers on
    DecayTestComponent. The callback is supposed to run for every removed component, with the
    component still readable from its table, and with the entity that owned it.
*/
struct ComponentDecayRecord
{
    int calls = 0;
    int liveReads = 0;
    Game::Entity lastEntity;
};

extern ComponentDecayRecord componentDecay;

class EntitySystemTest : public TestCase
{
    __DeclareClass(EntitySystemTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
