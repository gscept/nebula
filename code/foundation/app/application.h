#pragma once
//------------------------------------------------------------------------------
/**
    @class App::Application
  
    Provides a simple application model for Nebula apps.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/singleton.h"
#include "util/commandlineargs.h"

//------------------------------------------------------------------------------
namespace App
{
class Application
{
    __DeclareSingleton(Application);
public:
    /// constructor
    Application();
    /// destructor
    virtual ~Application();
    /// set company name
    void SetCompanyName(const Util::String& n);
    /// get company name
    const Util::String& GetCompanyName() const;
    /// set application name
    void SetAppTitle(const Util::String& n);
    /// get application name
    const Util::String& GetAppTitle() const;
    /// set application id
    void SetAppID(const Util::String& n);
    /// get application id
    const Util::String& GetAppID() const;   
    /// set application version
    void SetAppVersion(const Util::String& n);
    /// get application version
    const Util::String& GetAppVersion() const;
    /// set command line args
    void SetCmdLineArgs(const Util::CommandLineArgs& a);
    /// get command line args
    const Util::CommandLineArgs& GetCmdLineArgs() const;
    /// open the application
    virtual bool Open();
    /// close the application
    virtual void Close();
    /// exit the application, call right before leaving main()
    virtual void Exit();
    /// run the application, return when user wants to exit
    virtual void Run();
    /// return true if app is open
    bool IsOpen() const;
    /// get the return code
    int GetReturnCode() const;

protected:
    /// set return code
    void SetReturnCode(int c);

    Util::String companyName;
    Util::String appName;
    Util::String appID;
    Util::String appVersion;
    Util::CommandLineArgs args;
    bool isOpen;
    int returnCode;
};

//------------------------------------------------------------------------------
/**
*/
inline
bool
Application::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
Application::SetCompanyName(const Util::String& n)
{
    this->companyName = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
Application::GetCompanyName() const
{
    return this->companyName;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
Application::SetAppTitle(const Util::String& n)
{
    this->appName = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
Application::GetAppTitle() const
{
    return this->appName;
}     

//------------------------------------------------------------------------------
/**
*/
inline
void
Application::SetAppID(const Util::String& n)
{
    this->appID = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
Application::GetAppID() const
{
    return this->appID;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
Application::SetAppVersion(const Util::String& n)
{
    this->appVersion = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
Application::GetAppVersion() const
{
    return this->appVersion;
}

//------------------------------------------------------------------------------
/**
*/
inline
void    
Application::SetCmdLineArgs(const Util::CommandLineArgs& a)
{
    this->args = a;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::CommandLineArgs&
Application::GetCmdLineArgs() const
{
    return this->args;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Application::SetReturnCode(int c)
{
    this->returnCode = c;
}

//------------------------------------------------------------------------------
/**
*/
inline int
Application::GetReturnCode() const
{
    return this->returnCode;
}

} // namespace App
//------------------------------------------------------------------------------
