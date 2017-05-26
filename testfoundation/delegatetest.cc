//------------------------------------------------------------------------------
//  delegatetest.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#if !__PS3__
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
    Delegate<int> del0 = Delegate<int>::FromMethod<TestClass,&TestClass::TestMethodInt>(&testObj);
    Delegate<float> del1 = Delegate<float>::FromMethod<TestClass,&TestClass::TestMethodFloat>(&testObj);
    Delegate<int> del2 = Delegate<int>::FromMethod<TestClass,&TestClass::TestMethodVirtual>(&testObj);    
    Delegate<int> del3 = Delegate<int>::FromMethod<TestClass,&TestClass::TestMethodVirtual>(&testSubObj);
    Delegate<int> del4 = Delegate<int>::FromMethod<TestSubClass,&TestSubClass::TestMethodVirtual>(&testSubObj);
    Delegate<int> del5 = Delegate<int>::FromFunction<&TestClass::TestMethodStatic>();
    Delegate<float> del6 = Delegate<float>::FromFunction<&GlobalTestFunction>();

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
#endif
    
    