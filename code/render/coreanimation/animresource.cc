//------------------------------------------------------------------------------
//  animresource.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coreanimation/animresource.h"
#include "streamanimationpool.h"

namespace CoreAnimation
{

StreamAnimationPool* animPool = nullptr;

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<AnimClip>&
AnimGetClips(const AnimResourceId& id)
{
	return animPool->GetClips(id);
}

} // namespace CoreAnimation
