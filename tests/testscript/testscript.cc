//------------------------------------------------------------------------------
//  testscript.cc
//  (C) 2010 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scripttestapplication.h"
#include "system/appentry.h"

//ImplementNebulaApplication();

using namespace Test;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
int main(int argc,char**argv)
{
    ScriptTestApplication app(argc, argv);
    
    if (app.Open())
    {
        app.Run();
        app.Close();
    }
 
}