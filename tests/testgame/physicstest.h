#pragma once
//------------------------------------------------------------------------------
/**
    Tests the rebuild queue of the physics manager: the actors and shapes which
    are built from the body and collider components, and what happens to them
    when entities are pinged, reparented, lose a component or are destroyed.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "testbase/testcase.h"

namespace Test
{
class PhysicsManagerTest : public TestCase
{
    __DeclareClass(PhysicsManagerTest)
public:
    void Run() override;
};

} // namespace Test
