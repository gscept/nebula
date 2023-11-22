//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "assetbatcherapp.h"

#define NEBULA_TEXT_IMPLEMENT
#include "toolkit-common/text.h"

//------------------------------------------------------------------------------
/**
*/
int __cdecl
main(int argc, const char** argv)
{
    Util::CommandLineArgs args(argc, argv);
    Toolkit::AssetBatcherApp app;
    app.SetCompanyName("gscept");
    app.SetAppTitle("NebulaT asset batcher");
    app.SetCmdLineArgs(args);
    if (app.Open())
    {
        app.Run();
        app.Close();
    }
    app.Exit();
}
