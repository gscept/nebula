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

//------------------------------------------------------------------------------
/**
    Bookkeeping for the component initialization callback, which the test application registers on
    TestResource. The callback runs when the component is staged rather than in a table, so the entity is
    read through the chain of staged commands. That is what lets an Init see a component added in the same
    batch, including one added before it
*/
struct ComponentInitRecord
{
    int calls = 0;
    bool sawBatchComponent = false;
};

extern ComponentInitRecord componentInit;

class EntitySystemTest : public TestCase
{
    __DeclareClass(EntitySystemTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
