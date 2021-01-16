//------------------------------------------------------------------------------
//  profilingtest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "profilingtest.h"
#include "core/ptr.h"
#include "profiling/profiling.h"
#include "threading/thread.h"

namespace Test
{
__ImplementClass(Test::ProfilingTest, 'PROT', Test::TestCase);

using namespace Util;
using namespace Profiling;

class ProfilingThread : public Threading::Thread
{
    __DeclareClass(ProfilingThread);
public:
    void DoWork() override
    {
        ProfilingRegisterThread();
        fn();
    };
    std::function<void()> fn;

};

__ImplementClass(ProfilingThread, 'PRFT', Threading::Thread);

//------------------------------------------------------------------------------
/**
*/
void RecursivePrintScopes(const ProfilingScope& scope, int level)
{
    // variable length printf, just print as many tabs as we have recursive levels
    n_printf("%.*s%s(%d) %s: %f s\n", level, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t", scope.file, scope.line, scope.name, scope.duration);

    // depth-first traversal
    for (IndexT j = 0; j < scope.children.Size(); j++)
        RecursivePrintScopes(scope.children[j], level+1);
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilingTest::Run()
{
    auto fn = []()
    {
        {
            N_SCOPE(OneSecond, test);
            Core::SysFunc::Sleep(1);
        }

        // test scoped markers
        {
            N_SCOPE(ThreeSecondsAccum, test);
            {
                N_SCOPE(TwoSeconds, test);
                Core::SysFunc::Sleep(2);
            }
            {
                N_SCOPE(OneSecond, test);
                Core::SysFunc::Sleep(1);
            }
        }


        // test begin/end markers
        {
            N_MARKER_BEGIN(TwoSecondsInterval, test);
            Core::SysFunc::Sleep(2);
            N_MARKER_END();

            N_MARKER_BEGIN(OneSecondInterval, test);
            Core::SysFunc::Sleep(1);
            N_MARKER_END();

            N_MARKER_BEGIN(OneSecondIntervalAccum, test);
                N_MARKER_BEGIN(HalfSecondInterval, test);
                Core::SysFunc::Sleep(0.5f);
                N_MARKER_END();

                N_MARKER_BEGIN(HalfSecondInterval, test);
                Core::SysFunc::Sleep(0.5f);
                N_MARKER_END();
            N_MARKER_END();
        }
    };

    // start a thread, and generate scopes on that thread
    Ptr<ProfilingThread> thread = ProfilingThread::Create();
    thread->fn = fn;
    thread->SetName("ProfilingThread");
    thread->Start();

    // run on main thread
    ProfilingRegisterThread();
    fn();

    // wait for thread to finish
    thread->Stop();

    const Util::Array<ProfilingContext>& contexts = ProfilingGetContexts();
    IndexT i;
    for (i = 0; i < contexts.Size(); i++)
    {
        const ProfilingContext& ctx = contexts[i];
        VERIFY(Math::nearequal(ctx.topLevelScopes[0].duration, 1.0f, 0.1f));
        VERIFY(Math::nearequal(ctx.topLevelScopes[1].duration, 3.0f, 0.1f));
        VERIFY(Math::nearequal(ctx.topLevelScopes[1].children[0].duration, 2.0f, 0.1f));
        VERIFY(Math::nearequal(ctx.topLevelScopes[1].children[1].duration, 1.0f, 0.1f));
        VERIFY(Math::nearequal(ctx.topLevelScopes[2].duration, 2.0f, 0.1f));
        VERIFY(Math::nearequal(ctx.topLevelScopes[3].duration, 1.0f, 0.1f));
        VERIFY(Math::nearequal(ctx.topLevelScopes[4].duration, 1.0f, 0.1f));
        VERIFY(Math::nearequal(ctx.topLevelScopes[4].children[0].duration, 0.5f, 0.1f));

        n_printf("%s\n", ctx.threadName.Value());
        for (IndexT j = 0; j < ctx.topLevelScopes.Size(); j++)
        {
            RecursivePrintScopes(ctx.topLevelScopes[j], 0);
        }
    }
}

}; // namespace Test
