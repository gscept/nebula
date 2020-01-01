//------------------------------------------------------------------------------
//  interfacebase.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "interface/interfacebase.h"
#include "interface/interfacehandlerbase.h"
#include "core/coreserver.h"

namespace Interface
{
__ImplementClass(Interface::InterfaceBase, 'INBS', Messaging::AsyncPort);

using namespace Core;

//------------------------------------------------------------------------------
/**
*/
InterfaceBase::InterfaceBase() :
    rootDirectory("home:")
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
InterfaceBase::~InterfaceBase()        
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
InterfaceBase::Open()
{
    n_assert(!this->IsOpen());
    
    // set core server attributes
    this->SetCompanyName(CoreServer::Instance()->GetCompanyName());
    this->SetAppName(CoreServer::Instance()->GetAppName());
    this->SetRootDirectory(CoreServer::Instance()->GetRootDirectory());

    Messaging::AsyncPort::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
InterfaceBase::AttachHandler(const Ptr<Messaging::Handler>& handler)
{
    // set application attributes on handler
    const Ptr<InterfaceHandlerBase>& interfaceHandler = handler.downcast<InterfaceHandlerBase>();
    interfaceHandler->SetCompanyName(this->companyName);
    interfaceHandler->SetAppName(this->appName);

    // call parent class
    Messaging::AsyncPort::AttachHandler(handler);
}

} // namespace Interface