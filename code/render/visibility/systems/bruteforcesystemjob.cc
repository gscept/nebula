//------------------------------------------------------------------------------
//  bruteforcesystem.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
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
	const Graphics::GraphicsEntityId* ids = (const Graphics::GraphicsEntityId*)ctx.inputs[2];
	bool* flags = (bool*)ctx.outputs[0];

	const Math::matrix44 cam = *camera;

	const Math::bbox& box = transforms[0];
	const Math::ClipStatus::Type status = box.clipstatus_soa(cam);

	// if clip status is outside, unset visibility
	if (status == Math::ClipStatus::Inside || Math::ClipStatus::Clipped)
		flags[0] = true;
}

} // namespace Visibility
