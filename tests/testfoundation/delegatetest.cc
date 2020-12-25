//------------------------------------------------------------------------------
//  delegatetest.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "delegatetest.h"
#include "util/delegate.h"

namespace Test
{
__ImplementClass(Test::DelegateTest, 'dlgt', Test::TestCase);

using namespace Util;

class TestClass
{
public:
    /// a test method
    void TestMethodInt(int arg) { n_printf("TestClass::TestMethodInt(%d) called!\n", arg); };
    /// another test method
    void TestMethodFloat(float arg) { n_printf("TestClass::TestMethodFloat(%f) called!\n", arg); };
    /// a virtual test method
    virtual void TestMethodVirtual(int arg) { n_printf("TestClass::TestMethodVirtual(%d) called!\n", arg); };
    /// a static test method
    static void TestMethodStatic(int arg) { n_printf("TestClass::TestMethodStatic(%d) called!\n", arg); };
};

class TestSubClass : public TestClass
{
public:
    /// derived virtual method
    virtual void TestMethodVirtual(int arg) { n_printf("TestSubClass::TestMethodVirtual(%d) called!\n", arg); };
};

void
GlobalTestFunction(float arg)
{
    n_printf("GlobalTestFunction(%f)\n", arg);
}

//------------------------------------------------------------------------------
/**
*/
void
DelegateTest::Run()
{
    // create a few delegate objects
    TestClass testObj;
    TestSubClass testSubObj;
    Delegate<void(int)> del0 = Delegate<void(int)>::FromMethod<TestClass,&TestClass::TestMethodInt>(&testObj);
    Delegate<void(float)> del1 = Delegate<void(float)>::FromMethod<TestClass,&TestClass::TestMethodFloat>(&testObj);
    Delegate<void(int)> del2 = Delegate<void(int)>::FromMethod<TestClass,&TestClass::TestMethodVirtual>(&testObj);    
    Delegate<void(int)> del3 = Delegate<void(int)>::FromMethod<TestClass,&TestClass::TestMethodVirtual>(&testSubObj);
    Delegate<void(int)> del4 = Delegate<void(int)>::FromMethod<TestSubClass,&TestSubClass::TestMethodVirtual>(&testSubObj);
    Delegate<void(int)> del5 = Delegate<void(int)>::FromFunction<&TestClass::TestMethodStatic>();
    Delegate<void(float)> del6 = Delegate<void(float)>::FromFunction<&GlobalTestFunction>();

    // invoke delegates
    del0(10);
    del1(5.0);
    del2(20);
    del3(30);
    del4(40);
    del5(50);
    del6(60.0f);
}

} // namespace Test
