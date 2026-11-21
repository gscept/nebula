#pragma once
/**
    Tests the NavMesh component: the navmesh it loads, how it is found by entity and by agent
    kind, and that it is released when the component goes away, is replaced or its entity is
    deleted.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

namespace Test
{
class NavigationManagerTest : public TestCase
{
    __DeclareClass(NavigationManagerTest)
public:
    void Run() override;
};

} // namespace Test
