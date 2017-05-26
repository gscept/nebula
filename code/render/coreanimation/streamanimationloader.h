#pragma once
//------------------------------------------------------------------------------
/** 
    @class CoreAnimation::StreamAnimationLoader
    
    Initialize a CoreAnimation::AnimResource from the content of a stream.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/streamresourceloader.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class StreamAnimationLoader : public Resources::StreamResourceLoader
{
    __DeclareClass(StreamAnimationLoader);
protected:
    /// setup the AnimResource object from a stream
    virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);

    /// setup the AnimResource from an NAX3 stream
    bool SetupFromNax3(const Ptr<IO::Stream>& stream);
};

} // namespace CoreAnimation
//------------------------------------------------------------------------------
