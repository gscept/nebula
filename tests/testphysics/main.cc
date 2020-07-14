//------------------------------------------------------------------------------
//  testphysics/main.cc
//  (C) 2019 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "physicsapp.h"

using namespace Core;
using namespace Tests;


void
__cdecl main()
{
	// create Nebula runtime
	Ptr<CoreServer> coreServer = CoreServer::Create();
	coreServer->SetAppName(Util::StringAtom("Nebula Physics App"));
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
