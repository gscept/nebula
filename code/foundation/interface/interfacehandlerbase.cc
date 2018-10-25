//------------------------------------------------------------------------------
//  interfacehandlerbase.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "interface/interfacehandlerbase.h"

namespace Interface
{
__ImplementClass(Interface::InterfaceHandlerBase, 'IFHB', Messaging::Handler);

//------------------------------------------------------------------------------
/**
*/
InterfaceHandlerBase::InterfaceHandlerBase()
{
    // empty
}     

//------------------------------------------------------------------------------
/**
*/
void
InterfaceHandlerBase::DoWork()
{
    // empty
}

} // namespace Interface