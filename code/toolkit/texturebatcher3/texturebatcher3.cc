//------------------------------------------------------------------------------
//  texturebatcher3.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "texturebatcherapp.h"

//------------------------------------------------------------------------------
/**
*/
int __cdecl
main(int argc, const char** argv)
{    
    Util::CommandLineArgs args(argc, argv);
    Toolkit::TextureBatcherApp app;
    app.SetCompanyName("gscept");
    app.SetAppTitle("NebulaT Texture Batch Converter");
    app.SetCmdLineArgs(args);
    if (app.Open())
    {
        app.Run();
        app.Close();
    }
    app.Exit();
}
