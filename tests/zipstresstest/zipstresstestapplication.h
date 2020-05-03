#pragma once
//------------------------------------------------------------------------------
/**
    @class ZipStressTestApplication
    
    Multithreading stress test for zip file access.
    
    (C) 2009 Radon Labs GmbH
*/
#include "app/consoleapplication.h"
#include "core/coreserver.h"

//------------------------------------------------------------------------------
namespace App
{
class ZipStressTestApplication : public ConsoleApplication
{
public:
    /// run the application, return when user wants to exit
    virtual void Run();
}; 

} // namespace App
//------------------------------------------------------------------------------

    