#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::DatabaseTest

    Tests Game::Database functionality.

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"
#include "game/database/attribute.h"

namespace States
{
    struct TransformState
    {
        Math::mat4 localTransform;
        Math::mat4 worldTransform;
    };
}

namespace Attr
{
__DeclareAttribute(TestInt, AccessMode::ReadWrite, int, 'tsti', int(10));
__DeclareAttribute(TestString, AccessMode::ReadWrite, Util::String, 'tsts', Util::String("Hello world"));
__DeclareAttribute(TestVec4, AccessMode::ReadWrite, Math::vec4, 'tst4', Math::vec4(1, 2, 3, 4));
__DeclareAttribute(TestFloat, AccessMode::ReadWrite, float, 'tstf', 5.0f);
}

//------------------------------------------------------------------------------
namespace Test
{
class DatabaseTest : public TestCase
{
    __DeclareClass(DatabaseTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
