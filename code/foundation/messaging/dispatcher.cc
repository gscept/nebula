//------------------------------------------------------------------------------
//  message/dispatcher.cc
//  (C) 2005 RadonLabs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "messaging/dispatcher.h"
#include "messaging/port.h"

namespace Messaging
{
__ImplementClass(Messaging::Dispatcher, 'MDIS', Messaging::Port);

//------------------------------------------------------------------------------
/**
*/
Dispatcher::Dispatcher() :
    idPorts(64, 64)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Attach a new message port.

    @param  port    pointer to a message port object
*/
void
Dispatcher::AttachPort(const Ptr<Port>& port)
{
    n_assert(port);
    n_assert(!this->HasPort(port));

    // add to port array
    this->portArray.Append(port);

    // get the array of accepted messages from the port, and add each message
    // to our own accepted messages, and create a mapping of message ids to ports
    const Util::Array<const Id*>& idArray = port->GetAcceptedMessages();
    IndexT i;
    for (i = 0; i < idArray.Size(); i++)
    {
        const Id* msgIdPtr = idArray[i];
        this->RegisterMessage(*msgIdPtr);
        if (!this->idPortMap.Contains(msgIdPtr))
        {
            // need to add a new empty entry
            Util::Array<Ptr<Port> > emptyArray;
            this->idPorts.Append(emptyArray);
            this->idPortMap.Add(msgIdPtr, this->idPorts.Size() - 1);
        }
        this->idPorts[this->idPortMap[msgIdPtr]].Append(port);
    }
}

//------------------------------------------------------------------------------
/**
    Remove a message port object.

    @param  handler     pointer to message port object to be removed
*/
void
Dispatcher::RemovePort(const Ptr<Port>& port)
{
    n_assert(0 != port);
    n_assert(this->HasPort(port));

    // remove the port from the id/port map
    const Util::Array<const Id*>& idArray = port->GetAcceptedMessages();
    IndexT i;
    for (i = 0; i < idArray.Size(); i++)
    {
        const Id* msgIdPtr = idArray[i];
        if (this->idPortMap.Contains(msgIdPtr))
        {            
            Util::Array<Ptr<Port> >& ports = this->idPorts[this->idPortMap[msgIdPtr]];
            IndexT portIndex = ports.FindIndex(port);
            n_assert(InvalidIndex != portIndex);
            ports.EraseIndex(portIndex);
        }        
    }

    // NOTE: there's no way to remove the message from our accepted messages,
    // so that's not a bug!

    // finally remove the port from the ports array
    IndexT index = this->portArray.FindIndex(port);
    n_assert(InvalidIndex != index);
    this->portArray.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
    Return true if a port is already attached.
*/
bool
Dispatcher::HasPort(const Ptr<Port>& port) const
{
    n_assert(0 != port);
    return (InvalidIndex != this->portArray.FindIndex(port));
}

//------------------------------------------------------------------------------
/**
    Handle a message. The message will only be distributed to ports
    which accept the message.
*/
void
Dispatcher::HandleMessage(const Ptr<Message>& msg)
{
    const Id* msgIdPtr = &(msg->GetId());
    IndexT mapIndex = this->idPortMap.FindIndex(msgIdPtr);
    if (InvalidIndex != mapIndex)
    {
        const Util::Array<Ptr<Port> >& portArray = this->idPorts[this->idPortMap.ValueAtIndex(mapIndex)];
        IndexT portIndex;
        for (portIndex = 0; portIndex < portArray.Size(); portIndex++)
        {
            portArray[portIndex]->HandleMessage(msg);
        }
    }
}

} // namespace Messaging