//------------------------------------------------------------------------------
//  animeventhandlerbase.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "animation/animeventhandlerbase.h"

namespace Animation
{
__ImplementClass(Animation::AnimEventHandlerBase, 'AEHB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
AnimEventHandlerBase::AnimEventHandlerBase():
    isOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AnimEventHandlerBase::~AnimEventHandlerBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimEventHandlerBase::HandleEvent(const Animation::AnimEventInfo& event)
{
    n_assert(this->isOpen);

    // fallthrough: message was handled
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimEventHandlerBase::OnFrame(Timing::Time time)
{
    // overwrite in subclass
}
} // namespace Animation
