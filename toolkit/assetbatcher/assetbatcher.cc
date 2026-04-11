//------------------------------------------------------------------------------
//  assetbatcher.cc
//  (C) 2012-2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "assetbatcherapp.h"

//------------------------------------------------------------------------------
/**
*/
int __cdecl
main(int argc, const char** argv)
{
    Util::CommandLineArgs args(argc, argv);
    Toolkit::AssetBatcherApp app;
    app.SetCompanyName("gscept");
    app.SetAppTitle("Nebula asset batcher");
    app.SetCmdLineArgs(args);
    if (app.Open())
    {
        app.Run();
        app.Close();
    }
    app.Exit();
}
