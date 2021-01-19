#pragma once
//------------------------------------------------------------------------------
/**
    @class Interface::InterfaceBase
    
    Base class for interfaces. An interface is the frontend of a fat thread,
    visible from all threads in the Nebula application. Other threads can
    send messages to the Interface singleton which will dispatch the
    messages to handlers running in the thread context.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file 
*/
#include "messaging/asyncport.h"
#include "util/stringatom.h"

//------------------------------------------------------------------------------
namespace Interface
{
class InterfaceBase : public Messaging::AsyncPort
{
    __DeclareClass(InterfaceBase);
public:
    /// constructor
    InterfaceBase();
    /// destructor
    virtual ~InterfaceBase();
    
    /// attach a handler to the port (call before open!)
    virtual void AttachHandler(const Ptr<Messaging::Handler>& h);
    /// open the async port
    virtual void Open();
    /// close the async port
    virtual void Close();

    /// get the company name
    const Util::StringAtom& GetCompanyName() const;
    /// get the application name
    const Util::StringAtom& GetAppName() const;
    /// get the root directory
    const Util::StringAtom& GetRootDirectory() const;
    
private:
    /// set the company name
    void SetCompanyName(const Util::StringAtom& companyName);
    /// set the application name
    void SetAppName(const Util::StringAtom& appName);
    /// set the root directory (default is home:)
    void SetRootDirectory(const Util::StringAtom& dir);

    Util::StringAtom rootDirectory;
    Util::StringAtom companyName;
    Util::StringAtom appName;
};

//------------------------------------------------------------------------------
/**
*/
inline void
InterfaceBase::SetCompanyName(const Util::StringAtom& n)
{
    this->companyName = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
InterfaceBase::GetCompanyName() const
{
    return this->companyName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InterfaceBase::SetAppName(const Util::StringAtom& n)
{
    this->appName = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
InterfaceBase::GetAppName() const
{
    return this->appName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
InterfaceBase::SetRootDirectory(const Util::StringAtom& d)
{
    this->rootDirectory = d;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
InterfaceBase::GetRootDirectory() const
{
    return this->rootDirectory;
}

} // namespace Interface
//------------------------------------------------------------------------------
