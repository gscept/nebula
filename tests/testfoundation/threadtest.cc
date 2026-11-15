//------------------------------------------------------------------------------
//  threadtest.cc
//  (C) 2007 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "threadtest.h"
#include "io/console.h"
#include "timing/timer.h"
#if __linux__
#include <time.h>
#endif

namespace Test
{
__ImplementClass(Test::ThreadTest, 'THRT', Test::TestCase);

__ImplementClass(Test::TestSingle, 'TSTS', Core::RefCounted);
__ImplementSingleton(Test::TestSingle);

__ImplementClass(Test::MyThread, 'MYTD', Threading::Thread);

bool ThreadTest::InstanceCheck = true;
Threading::Event ThreadTest::threadEvent;

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
MyThread::DoWork()
{
    Ptr<IO::Console> con;
    ThreadTest::InstanceCheck = TestSingle::HasInstance();

    if (!IO::Console::HasInstance())
    {
        con = IO::Console::Create();
        con->Open();
    }
    
    if (!ThreadTest::InstanceCheck)
    {
        n_printf("no test single available \n");
        Ptr<Test::TestSingle> ptr = TestSingle::Create();
        n_printf("thread single address 0x%zx \n", (size_t) TestSingle::Instance());
    }
    else
    {
        n_printf("already available\n");
    }

    ThreadTest::threadEvent.Signal();    
}


//------------------------------------------------------------------------------
/**
*/
TestSingle::TestSingle()
{
    __ConstructSingleton;     
}

//------------------------------------------------------------------------------
/**
*/
TestSingle::~TestSingle()
{
}

//------------------------------------------------------------------------------
/**
*/
void
ThreadTest::Run()
{
    this->mythread = MyThread::Create();    ;
    this->mythread->SetThreadAffinity(System::Cpu::Core1);
    this->single = TestSingle::Create();

    this->single = TestSingle::Instance();
    n_printf("main address 0x%zx \n", (size_t) TestSingle::Instance());
    this->mythread->Start();
    
    threadEvent.Wait();
    VERIFY(!InstanceCheck);

    // WaitTimeout() with a sub-second remainder used to build an invalid deadline,
    // busy-spinning until the wall clock allowed a valid one and then waiting the
    // full timeout on top of that
    {
#if __linux__
        // start half way into a second, the old code would spin for ~0.5s here
        timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        timespec delay = { 0, (1500000000L - now.tv_nsec) % 1000000000L };
        nanosleep(&delay, nullptr);
#endif
        Threading::Event unsignaled;
        Timing::Timer timer;
        timer.Start();
        bool signaled = unsignaled.WaitTimeout(999);
        timer.Stop();
        VERIFY(!signaled);
        VERIFY(timer.GetTime() >= 0.9 && timer.GetTime() < 1.25);

        Threading::Event signaledEvent;
        signaledEvent.Signal();
        VERIFY(signaledEvent.WaitTimeout(999));

        // auto reset, the previous wait consumed the signal
        VERIFY(!signaledEvent.WaitTimeout(1));
    }
}

} // namespace Test