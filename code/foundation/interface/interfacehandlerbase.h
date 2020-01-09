#pragma once
//------------------------------------------------------------------------------
/**
    @class Interface::InterfaceHandlerBase
    
    Base class for message handlers attached to Interface objects.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "messaging/handler.h"
#include "util/stringatom.h"

//------------------------------------------------------------------------------
namespace Interface
{
class InterfaceHandlerBase : public Messaging::Handler
{
    __DeclareClass(InterfaceHandlerBase);
public:
    /// constructor
    InterfaceHandlerBase();
    /// set the company name
    void SetCompanyName(const Util::StringAtom& companyName);
    /// get the company name
    const Util::StringAtom& GetCompanyName() const;
    /// set the application name
    void SetAppName(const Util::StringAtom& appName);
    /// get the application name
    const Util::StringAtom& GetAppName() const;
    /// optional "per-frame" DoWork method for continuous handlers
    virtual void DoWork();

protected:
    Util::StringAtom companyName;
    Util::StringAtom appName;
};

//------------------------------------------------------------------------------
/**
*/
inline void
InterfaceHandlerBase::SetCompanyName(const Util::StringAtom& n)
{
    this->companyName = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
InterfaceHandlerBase::GetCompanyName() const
{
    return this->companyName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InterfaceHandlerBase::SetAppName(const Util::StringAtom& n)
{
    this->appName = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
InterfaceHandlerBase::GetAppName() const
{
    return this->appName;
}

} // namespace Interface
//------------------------------------------------------------------------------
