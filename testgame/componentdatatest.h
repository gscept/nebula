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

enum AttributeNames
{
	OWNER,
	GUID,
	STRING,
	INT,
	FLOAT,
	NumAttributes
};

class TestComponent : public Game::Component<
	decltype(Attr::GuidTest),
	decltype(Attr::StringTest),
	decltype(Attr::IntTest),
	decltype(Attr::FloatTest)
>
{
	__DeclareClass(TestComponent)
public:
	TestComponent();
	~TestComponent();

	uint32_t RegisterEntity(const Game::Entity& e);
	void DeregisterEntity(const Game::Entity& e);
	SizeT Optimize();

	/// get single item from resource, template expansion might give you cancer
	template <int MEMBER>
	void Set(const uint32_t instance, Util::Variant value);

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
