#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::JobsTestApplication
  
    Test Jobs subsystem.
    
    (C) 2009 Radon Labs GmbH
*/    
#include "app/consoleapplication.h"
#include "jobs/jobsystem.h"
#include "jobs/job.h"
#include "jobs/jobport.h"
#include "io/gamecontentserver.h"

//------------------------------------------------------------------------------
namespace Particles
{
class JobsTestApplication : public App::ConsoleApplication
{
public:
    /// constructor
    JobsTestApplication();
    /// destructor
    virtual ~JobsTestApplication();
    /// open the application
    virtual bool Open();
    /// close the application
    virtual void Close();
    /// run the application
    virtual void Run();

private:
    Ptr<Jobs::JobSystem> jobSystem;
    Ptr<Jobs::JobPort> jobPort;
    Ptr<Jobs::Job> job;
    Ptr<IO::GameContentServer> gameContentServer;
};

} // namespace Test
//------------------------------------------------------------------------------
