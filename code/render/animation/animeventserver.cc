//------------------------------------------------------------------------------
//  audioserver.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "animation/animeventserver.h"

namespace Animation
{
__ImplementClass(Animation::AnimEventServer, 'ANES', Core::RefCounted);
__ImplementSingleton(Animation::AnimEventServer);


//------------------------------------------------------------------------------
/**
*/
AnimEventServer::AnimEventServer() : isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
AnimEventServer::~AnimEventServer()
{
    this->animEventHandler.Clear();

    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimEventServer::Open()
{
    n_assert(!this->isOpen);

    // TODO implement

    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimEventServer::Close()
{
    n_assert(this->isOpen);

    // TODO implement

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimEventServer::RegisterAnimEventHandler(const Ptr<AnimEventHandlerBase>& newHandler)
{
    n_assert(newHandler.isvalid());
    
    // check if already a handler for this category
    if(this->animEventHandler.Contains(newHandler->GetCategoryName()))
    {
        n_error("AnimEventServer::RegisterAnimEventHandler -> Handler for category '%s' already registered!", newHandler->GetCategoryName().Value());
    }

    // now open and attach this handler
    newHandler->Open();
    this->animEventHandler.Add(newHandler->GetCategoryName(), newHandler);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimEventServer::UnregisterAnimEventHandler(const Util::StringAtom& categoryName)
{
    n_assert(categoryName.IsValid());
    
    // check if exists
    if(!this->animEventHandler.Contains(categoryName))
    {
        n_error("AnimEventServer::UnregisterAnimEventHandler -> No such handler registered '%s'!", categoryName.Value());
    }

    // close and detach
    IndexT index = this->animEventHandler.FindIndex(categoryName);
    this->animEventHandler.ValueAtIndex(index)->Close();
    this->animEventHandler.EraseAtIndex(index);
}


//------------------------------------------------------------------------------
/**
*/
void
AnimEventServer::UnregisterAnimEventHandler(const Ptr<AnimEventHandlerBase>& handler)
{        
    for(IndexT i = 0 ; i < this->animEventHandler.Size() ; i++)
    {
        if(this->animEventHandler.ValueAtIndex(i) == handler)
        {
            handler->Close();
            this->animEventHandler.EraseAtIndex(i);
            return;
        }
    }
    n_error("tried to remove unknown animeventhandler");
}
//------------------------------------------------------------------------------
/**
*/
bool
AnimEventServer::HandleAnimEvents(const Util::Array<Animation::AnimEventInfo>& eventz)
{
    n_assert(eventz.Size() > 0);

    bool allHandled = true;
    IndexT index;
    for (index = 0; index < eventz.Size(); index++)
    {
        const AnimEventInfo& event = eventz[index];
        Util::StringAtom category;
        if(event.GetAnimEvent().HasCategory())
        {
            category = event.GetAnimEvent().GetCategory();
        }

        IndexT indeX = this->animEventHandler.FindIndex(category);
        if (InvalidIndex == indeX)
        {
            allHandled = false;
        }
        else
        {
            allHandled &= this->animEventHandler.ValueAtIndex(indeX)->HandleEvent(event);
        }
    }
    return allHandled;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimEventServer::OnFrame(Timing::Time time)
{
    IndexT index;
    for (index = 0; index < this->animEventHandler.Size(); index++)
    {
        this->animEventHandler.ValueAtIndex(index)->OnFrame(time);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool 
AnimEventServer::HandleMessage(const Ptr<Messaging::Message>& msg)
{
    bool handled = false;
    IndexT index;
    for (index = 0; index < this->animEventHandler.Size(); index++)
    {
        handled |= this->animEventHandler.ValueAtIndex(index)->HandleMessage(msg);
    }    
    return handled;
}
} // namespace Animation

