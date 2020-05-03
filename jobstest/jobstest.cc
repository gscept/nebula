//------------------------------------------------------------------------------
//  jobstest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobstestapplication.h"

ImplementNebulaApplication();

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
NebulaMain(const CommandLineArgs& args)
{
    Particles::JobsTestApplication app;
    app.SetCompanyName("Radon Labs GmbH");
    app.SetAppTitle("Jobs Test");
    if (app.Open())
    {
        app.Run();
        app.Close();
    }
    app.Exit();
}

