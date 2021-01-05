//------------------------------------------------------------------------------
//  testmem/main.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "viewerapp.h"



using namespace Core;
using namespace Tests;


int main(int argc, char**argv)
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Test Viewer"));
#ifdef USE_GITHUB_DEMO
    coreServer->SetRootDirectory("https://github.com/gscept/nebula-test-content/raw/master/");
#endif
    coreServer->Open();

    Tests::SimpleViewerApplication app;
    if (app.Open())
    {
        app.Run();
        app.Close();
    }

    coreServer->Close();
    coreServer = nullptr;   

    Core::SysFunc::Exit(0);
}
