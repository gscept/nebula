//------------------------------------------------------------------------------
//  bruteforcesystem.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "bruteforcesystem.h"
#include "math/clipstatus.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void 
BruteforceSystemJobFunc(const Jobs::JobFuncContext& ctx)
{
	const Math::matrix44* camera = (const Math::matrix44*)ctx.uniforms[0];

	const Math::matrix44* transforms = (const Math::matrix44*)ctx.inputs[0];
	bool* flag = (bool*)ctx.outputs[0];

	const Math::bbox& box = transforms[0];
	const Math::ClipStatus::Type status = box.clipstatus_soa(*camera);

	// if clip status is outside, unset visibility
	if (status == Math::ClipStatus::Outside)
		*flag = false;
}

} // namespace Visibility
