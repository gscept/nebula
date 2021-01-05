#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::Dispatcher

    A message Dispatcher is a specialization of a message Port. A message
    Dispatcher distributes all messages it receives to the attached Ports
    which are interested in this message id.

    @verbatim
                                    +------+
                                +-->| Port |
                               /    +------+
                +------------+/     +------+
    --- Msg --->| Dispatcher |----->| Port |
                +------------+\     +------+
                               \    +------+
                                +-->| Port |
                                    +------+
    @endverbatim

    Dispatcher objects usually serve as front end message ports which hide
    a more complex message processing infrastructure underneath.

    (C) 2007 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/ptr.h"
#include "messaging/port.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class Dispatcher : public Port
{
    __DeclareClass(Dispatcher);
public:
    /// constructor
    Dispatcher();
    /// handle a single message (distribute to ports which accept the message)
    virtual void HandleMessage(const Ptr<Messaging::Message>& msg);
    /// attach a message port
    void AttachPort(const Ptr<Port>& port);
    /// remove a message port
    void RemovePort(const Ptr<Port>& port);
    /// return true if a port exists
    bool HasPort(const Ptr<Port>& port) const;

private:
    Util::Array<Ptr<Port> > portArray;
    Util::Array<Util::Array<Ptr<Port> > > idPorts;             // one entry per message, contains ports which accepts the message
    Util::Dictionary<const Id*,IndexT> idPortMap;    // maps message id's to indices in the msgIdPorts array
};

} // namespace Message
//------------------------------------------------------------------------------
