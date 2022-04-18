//------------------------------------------------------------------------------
//  main.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "texgenapp.h"

//------------------------------------------------------------------------------
/**
*/
int __cdecl
main(int argc, const char** argv)
{    
    Util::CommandLineArgs args(argc, argv);
    Toolkit::TexGenApp app;
    app.SetCompanyName("gscept");
    app.SetAppTitle("Nebula Texture Generator");
    app.SetAppVersion("0.1.1");
    app.SetCmdLineArgs(args);
    
    if (app.Open())
    {
        if (app.ParseCmdLineArgs())
        {
            app.Run();
        }
        app.Close();
    }
    app.Exit();
    return app.GetReturnCode();
}
