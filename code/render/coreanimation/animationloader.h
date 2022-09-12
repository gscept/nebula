#pragma once
//------------------------------------------------------------------------------
/** 
    @class CoreAnimation::StreamAnimationLoader
    
    Initialize a CoreAnimation::AnimResource from the content of a stream.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "animresource.h"
#include "resources/resourceloader.h"
#include "coreanimation/animclip.h"
#include "coreanimation/animkeybuffer.h"
#include "coregraphics/config.h"
#include "ids/idallocator.h"

namespace CoreAnimation
{

class AnimationLoader : public Resources::ResourceLoader
{
    __DeclareClass(AnimationLoader);

private:
    friend class AnimSampleBuffer;

    /// perform actual load, override in subclass
    Resources::ResourceUnknownId LoadFromStream(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// unload resource
    void Unload(const Resources::ResourceId id);
};

} // namespace CoreAnimation
//------------------------------------------------------------------------------
