#ifndef TEST_THREADTEST_H
#define TEST_THREADTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::ThreadTest
    
    Test functionality of thread singletons/thread local storage.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"
#include "core/refcounted.h"
#include "core/singleton.h"
#include "threading/thread.h"
#include "threading/event.h"

//------------------------------------------------------------------------------
namespace Test
{  

class TestSingle : public Core::RefCounted
{
    __DeclareClass(TestSingle);
    __DeclareSingleton(TestSingle);
public:
    /// constructor
    TestSingle();
    /// destructor
    virtual ~TestSingle();
};


class MyThread : public Threading::Thread
{
    __DeclareClass(MyThread);
public:
    virtual void DoWork();
    /// attach a message handler (called by OnCreateHandlers())
};



class ThreadTest : public TestCase
{
    __DeclareClass(ThreadTest);
public:
    /// run the test
    virtual void Run();

    Ptr<Test::TestSingle> single;
    Ptr<Test::MyThread> mythread;
    
    static bool InstanceCheck;
    static Threading::Event threadEvent;
};

} // namespace Test
//------------------------------------------------------------------------------
#endif    