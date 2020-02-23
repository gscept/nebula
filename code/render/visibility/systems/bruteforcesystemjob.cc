//------------------------------------------------------------------------------
//  bruteforcesystem.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "bruteforcesystem.h"
#include "math/clipstatus.h"
#include "profiling/profiling.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void 
BruteforceSystemJobFunc(const Jobs::JobFuncContext& ctx)
{
	N_SCOPE(BruteforceViewFrustumCulling, Visibility);
	const Math::matrix44* camera = (const Math::matrix44*)ctx.uniforms[0];
	for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
	{
		const Math::matrix44* transforms = (const Math::matrix44*)N_JOB_INPUT(ctx, sliceIdx, 0);
		Math::ClipStatus::Type* flag = (Math::ClipStatus::Type*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

		const Math::bbox& box = *transforms;
		*flag = box.clipstatus_soa(*camera);
	}
}

} // namespace Visibility
