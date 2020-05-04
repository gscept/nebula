//------------------------------------------------------------------------------
//  zipstresstest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "zipstresstestapplication.h"

//------------------------------------------------------------------------------
/**
*/
int
__cdecl main()
{
    App::ZipStressTestApplication app;
    app.SetCompanyName("Radon Labs GmbH");
    app.SetAppID("ZipStressTest");
    if (app.Open())
    {
        app.Run();
        app.Close();
    }
    app.Exit();
}
