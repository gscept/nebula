#pragma once
//------------------------------------------------------------------------------
/**
    @class Animation::AnimEventServer

    This is the server, which is triggered if a animation event is emitted.

    Attach here some handlers to handle special animevents. Handler can be
    registered via RegisterAnimEventHandler and unregistered through
    UnregisterAnimEventHandler messages from the GraphicsProtocol!
    
    Animeventhandler are specified by the category name they handle!

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "messaging/handler.h"
#include "core/singleton.h"
#include "animation/animeventhandlerbase.h"

//------------------------------------------------------------------------------
namespace Animation
{
class AnimEventServer : public Messaging::Handler
{
    __DeclareClass(AnimEventServer);
    __DeclareSingleton(AnimEventServer);
public:

    /// constructor
    AnimEventServer();
    /// destructor
    virtual ~AnimEventServer();    

    /// open the server
    void Open();
    /// close the server
    void Close();
    /// return true if open
    bool IsOpen() const;

    /// delegate to attached handler
    void OnFrame(Timing::Time time);

    /// attach an animeventhandler
    void RegisterAnimEventHandler(const Ptr<AnimEventHandlerBase>& newHandler);
    /// detach an animeventhandler
    void UnregisterAnimEventHandler(const Util::StringAtom& categoryName);
    /// detach an animeventhandler
    void UnregisterAnimEventHandler(const Ptr<AnimEventHandlerBase>& handler);

    /// delegate this event to a attached handler
    bool HandleAnimEvents(const Util::Array<Animation::AnimEventInfo>& eventz);

    /// handle a message, return true if handled
    virtual bool HandleMessage(const Ptr<Messaging::Message>& msg);

private:
    bool isOpen;
    Util::Dictionary<Util::StringAtom, Ptr<AnimEventHandlerBase> > animEventHandler;

};

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimEventServer::IsOpen() const
{
    return this->isOpen;
}
} // namespace Audio
//------------------------------------------------------------------------------
    