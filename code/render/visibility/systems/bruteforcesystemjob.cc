//------------------------------------------------------------------------------
//  bruteforcesystem.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "bruteforcesystem.h"
#include "math/clipstatus.h"
#include "math/mat4.h"
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
	const Math::mat4* camera = (const Math::mat4*)ctx.uniforms[0];
	for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
	{
		const Math::mat4* transforms = (const Math::mat4*)N_JOB_INPUT(ctx, sliceIdx, 0);
		Math::ClipStatus::Type* flag = (Math::ClipStatus::Type*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

		const Math::bbox& box = *transforms;
		*flag = box.clipstatus(*camera);
	}
}

} // namespace Visibility
