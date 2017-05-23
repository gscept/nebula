//------------------------------------------------------------------------------
//  delegatetable.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "messaging/delegatetable.h"

#if !(__WII__ || __PS3__)
namespace Messaging
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
DelegateTable::AddDelegate(const Id& msgId, const Delegate<const Ptr<Message>&>& del)
{
    IndexT index = this->idIndexMap.FindIndex(&msgId);
    if (InvalidIndex == index)
    {
        // this is the first delegate for this message id
        Array<Delegate<const Ptr<Message>&> > emptyArray;
        this->delegateArray.Append(emptyArray);
        index = this->delegateArray.Size() - 1;
        this->idIndexMap.Add(&msgId, index);
    }

    // add delegate to lookup table
    this->delegateArray[index].Append(del);
}

//------------------------------------------------------------------------------
/**
*/
bool
DelegateTable::Invoke(const Ptr<Message>& msg)
{
    // check if any delegates have been bound to the message
    IndexT index = this->idIndexMap.FindIndex(&msg->GetId());
    if (InvalidIndex != index)    
    {
        // call delegates for this message
        const Array<Delegate<const Ptr<Message>&> > delegates = this->delegateArray[index];
        IndexT delegateIndex;
        for (delegateIndex = 0; delegateIndex < delegates.Size(); delegateIndex++)
        {
            delegates[delegateIndex](msg);
        }
        return true;
    }
    else
    {
        // no delegates for this message
        return false;
    }
}

} // namespace Messaging
#endif
