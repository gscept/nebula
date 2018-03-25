//------------------------------------------------------------------------------
//  idlc.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idlcompiler.h"

using namespace Tools;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
int __cdecl
main(int argc, const char** argv)
{
    CommandLineArgs args(argc, argv);
    IDLCompiler app;
    app.SetCompanyName("gscept");
    app.SetAppTitle("NebulaT IDL Compiler");
    app.SetCmdLineArgs(args);
    if (app.Open())
    {
        app.Run();
        app.Close();
    }
    app.Exit();
    return 0;
}
