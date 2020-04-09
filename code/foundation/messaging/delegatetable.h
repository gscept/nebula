#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::DelegateTable
    
    Associates message ids with handler delegates. One message id may
    be associated with any number of handler functions. The order
    of handler functions for one message id will be preserved.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/delegate.h"
#include "messaging/id.h"
#include "messaging/message.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class DelegateTable
{
public:
    /// bind a message id to a method call
    template<class CLASS, void (CLASS::*METHOD)(const Ptr<Message>&)> void Bind(const Id& msgId, CLASS* obj);
    /// invoke delegates bound to msg, returns false if no delegates bound to msg id
    bool Invoke(const Ptr<Message>& msg);
    
private:
    /// add a delegate
    void AddDelegate(const Id& msgId, const Util::Delegate<void(const Ptr<Message>&)>& del);

    Util::Array<Util::Array<Util::Delegate<void(const Ptr<Message>&)> > > delegateArray;
    Util::Dictionary<const Id*, IndexT> idIndexMap;
};

//------------------------------------------------------------------------------
/**
*/
template<class CLASS, void (CLASS::*METHOD)(const Ptr<Message>&)>
void
DelegateTable::Bind(const Id& msgId, CLASS* obj)
{
    // setup a new delegate
    Util::Delegate<void(const Ptr<Message>&)> del = Util::Delegate<void(const Ptr<Message>&)>::FromMethod<CLASS,METHOD>(obj);
    this->AddDelegate(msgId, del);
}

} // namespace Messaging
//------------------------------------------------------------------------------
