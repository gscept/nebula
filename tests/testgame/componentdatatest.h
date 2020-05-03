#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ComponentTest
    
    Testscomponentdata functionality.
    
    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"
#include "game/component/attribute.h"
#include "game/component/component.h"
#include "game/entity.h"

//------------------------------------------------------------------------------
namespace Attr
{
    __DeclareAttribute(GuidTest, Util::Guid, 'gTst', Attr::ReadWrite, Util::Guid());
    __DeclareAttribute(StringTest, Util::String, 'sTst', Attr::ReadWrite, "Default string");
    __DeclareAttribute(IntTest, int, 'iTst', Attr::ReadWrite, int(1337));
    __DeclareAttribute(FloatTest, float, 'fTst', Attr::ReadWrite, float(10.0f));
} // namespace Attr

namespace Test
{

class TestComponentAllocator : public Game::Component<
    Attr::GuidTest,
    Attr::StringTest,
    Attr::IntTest,
    Attr::FloatTest
>
{

};


class TestComponent
{
    __DeclareComponent(TestComponent)
};

class CompDataTest : public TestCase
{
    __DeclareClass(CompDataTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
