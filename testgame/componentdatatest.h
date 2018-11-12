#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ComponentTest
    
    Testscomponentdata functionality.
    
    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"
#include "game/attr/attrid.h"
#include "game/component/component.h"
#include "game/entity.h"


//------------------------------------------------------------------------------
namespace Attr
{
	DeclareGuid(GuidTest, 'gTst', Attr::ReadWrite);
	DeclareString(StringTest, 'sTst', Attr::ReadWrite);
	DeclareInt(IntTest, 'iTst', Attr::ReadWrite);
	DeclareFloat(FloatTest, 'fTst', Attr::ReadWrite);
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
namespace Test
{

class TestComponent
{
	__DeclareComponent(TestComponent)
private:
	bool immediate_deletion = false;
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
