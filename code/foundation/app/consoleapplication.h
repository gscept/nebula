#pragma once
//------------------------------------------------------------------------------
/**
    @class App::ConsoleApplication
  
    Base class for Nebula console applications. Will provide a typicel
    execution environment for console apps.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "app/application.h"
#include "core/coreserver.h"
#include "io/ioserver.h"

//------------------------------------------------------------------------------
namespace App
{
class ConsoleApplication : public Application
{
public:
    /// constructor
    ConsoleApplication();
    /// destructor
    virtual ~ConsoleApplication();
    /// open the application
    virtual bool Open();
    /// close the application
    virtual void Close();
    
protected:
    Ptr<Core::CoreServer> coreServer;
    Ptr<IO::IoServer> ioServer;
};

} // namespace App
//------------------------------------------------------------------------------
