#pragma once
//------------------------------------------------------------------------------
/** 
	@class CoreAnimation::StreamAnimationLoader
	
	Initialize a CoreAnimation::AnimResource from the content of a stream.
	
	(C) 2008 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
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
	const Util::FixedArray<AnimClip>& GetClips(const AnimResourceId id);
	/// get clip by index
	const AnimClip& GetClip(const AnimResourceId id, const IndexT index);
	/// get clip index based on name
	const IndexT GetClipIndex(const AnimResourceId id, const Util::StringAtom& name);
	/// get anim key buffer
	const Ptr<AnimKeyBuffer>& GetKeyBuffer(const AnimResourceId id);
private:
	friend class AnimSampleBuffer;

	/// perform actual load, override in subclass
	LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
	/// unload resource
	void Unload(const Resources::ResourceId id);

	Ids::IdAllocator<
		Util::FixedArray<AnimClip>,
		Util::HashTable<Util::StringAtom, IndexT, 32>,
		Ptr<AnimKeyBuffer>
	> animAllocator;

	__ImplementResourceAllocatorTyped(animAllocator, AnimResourceIdType);
};

} // namespace CoreAnimation
//------------------------------------------------------------------------------
