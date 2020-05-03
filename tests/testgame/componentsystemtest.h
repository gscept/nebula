#pragma once
//------------------------------------------------------------------------------
/**
	ComponentSystemTest

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "testbase/testcase.h"

namespace App
{
	class GameApplication;
}

namespace Test
{

class ComponentSystemTest : public TestCase
{
	__DeclareClass(ComponentSystemTest)
public:
	virtual void Run();
	App::GameApplication* gameApp;
};

} // namespace Test
