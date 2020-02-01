//------------------------------------------------------------------------------
//  profilingtest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "profilingtest.h"
#include "core/ptr.h"
#include "profiling/profiling.h"

namespace Test
{
__ImplementClass(Test::ProfilingTest, 'PROT', Test::TestCase);

using namespace Util;
using namespace Profiling;

//------------------------------------------------------------------------------
/**
*/
void RecursivePrintScopes(const ProfilingScope& scope, int level)
{
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
	{
		N_MARKER(OneSecond, test);
		Core::SysFunc::Sleep(1);
	}

	{
		N_MARKER(ThreeSecondsAccum, test);
		{
			N_MARKER(TwoSeconds, test);
			Core::SysFunc::Sleep(2);
		}
		{
			N_MARKER(OneSecond, test);
			Core::SysFunc::Sleep(1);
		}
	}

	const Util::Array<ProfilingScope>& scopes = ProfilingGetScopes();

	VERIFY(Math::n_nearequal(scopes[0].duration, 1.0f, 0.1f));
	VERIFY(Math::n_nearequal(scopes[1].duration, 3.0f, 0.1f));
	VERIFY(Math::n_nearequal(scopes[1].children[0].duration, 2.0f, 0.1f));
	VERIFY(Math::n_nearequal(scopes[1].children[1].duration, 1.0f, 0.1f));

	for (IndexT i = 0; i < scopes.Size(); i++)
	{
		RecursivePrintScopes(scopes[i], 0);
	}
}

}; // namespace Test
