#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::Id
    
    A message identifier. This is automatically implemented in message classes
    using the __DeclareMsgId and __ImplementMsgId macros.
   
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class Id
{
public:
    /// constructor
    Id();
    /// equality operator
    bool operator==(const Id& rhs) const;
};

//------------------------------------------------------------------------------
/**
*/
inline
Id::Id()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
Id::operator==(const Id& rhs) const
{
    return (this == &rhs);
}

} // namespace Messaging
//------------------------------------------------------------------------------
