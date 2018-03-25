#pragma once
//------------------------------------------------------------------------------
/** 
    @class CoreAnimation::StreamAnimationLoader
    
    Initialize a CoreAnimation::AnimResource from the content of a stream.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "animresource.h"
#include "resources/resourcestreampool.h"
#include "coreanimation/animclip.h"
#include "coreanimation/animkeybuffer.h"
#include "coregraphics/config.h"
#include "ids/idallocator.h"

namespace CoreAnimation
{

class StreamAnimationPool : public Resources::ResourceStreamPool
{
    __DeclareClass(StreamAnimationPool);

public:
	/// get clips
	const Util::FixedArray<AnimClip>& GetClips(const AnimResourceId& id);
	/// name-to-clip map
	const Util::Dictionary<Util::StringAtom, IndexT>& GetClipIndexMap(const AnimResourceId& id);
	/// get anim key buffer
	const Ptr<AnimKeyBuffer>& GetKeyBuffer(const AnimResourceId& id);
protected:
	friend class AnimSampleBuffer;

	/// perform actual load, override in subclass
	LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Resources::ResourceId id);

	Ids::IdAllocator<
		Util::FixedArray<AnimClip>,
		Util::Dictionary<Util::StringAtom, IndexT>,
		Ptr<AnimKeyBuffer>
	> animAllocator;

	__ImplementResourceAllocatorTyped(animAllocator, AnimResourceIdType);
};

} // namespace CoreAnimation
//------------------------------------------------------------------------------
