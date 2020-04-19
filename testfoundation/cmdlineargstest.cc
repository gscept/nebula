//------------------------------------------------------------------------------
//  cmdlineargstest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "cmdlineargstest.h"
#include "util/commandlineargs.h"

namespace Test
{
__ImplementClass(Test::CmdLineArgsTest, 'CLAT', Test::TestCase);

using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
CmdLineArgsTest::Run()
{
    // create a POSIX style cmd line args object
    const char* argv[] = {
        "mycommand",
        "-help",
        "-file", "filename.txt",
        "-retries", "10",
        "-weight", "1.34",
        "-pos", "1.03,4.53,10.2",
        "-color", "1.0,2.0,3.0,1.0"
    };
    const int argc = 12;
    CommandLineArgs posixArgs(argc, argv);

    // create Win32 style cmd line args object
    CommandLineArgs winArgs("mycommand -help -file filename.txt -retries 10 -weight 1.34 -pos \"1.03, 4.53, 10.2 1.0\" -color \"1.0,2.0,3.0,1.0\"");

    // check args
    VERIFY(posixArgs.GetCmdName() == "mycommand");
    VERIFY(posixArgs.HasArg("-help"));
    VERIFY(posixArgs.HasArg("-file"));
    VERIFY(posixArgs.HasArg("-retries"));
    VERIFY(posixArgs.HasArg("-weight"));
    VERIFY(posixArgs.HasArg("-pos"));
    VERIFY(posixArgs.HasArg("-color"));
    VERIFY(!posixArgs.HasArg("-bla"));

    VERIFY(winArgs.GetCmdName() == "mycommand");
    VERIFY(winArgs.HasArg("-help"));
    VERIFY(winArgs.HasArg("-file"));
    VERIFY(winArgs.HasArg("-retries"));
    VERIFY(winArgs.HasArg("-weight"));
    VERIFY(winArgs.HasArg("-pos"));
    VERIFY(winArgs.HasArg("-color"));
    VERIFY(!winArgs.HasArg("-bla"));

    VERIFY(posixArgs.GetBoolFlag("-help") == true);
    VERIFY(posixArgs.GetString("-file") == "filename.txt");
    VERIFY(posixArgs.GetInt("-retries") == 10);
    VERIFY(posixArgs.GetFloat("-weight") == 1.34f);
    VERIFY(posixArgs.GetVec4("-color") == vec4(1.0f, 2.0f, 3.0f, 1.0f));

    VERIFY(winArgs.GetBoolFlag("-help") == true);
    VERIFY(winArgs.GetString("-file") == "filename.txt");
    VERIFY(winArgs.GetInt("-retries") == 10);
    VERIFY(winArgs.GetFloat("-weight") == 1.34f);
    VERIFY(winArgs.GetVec4("-pos") == vec4(1.03f, 4.53f, 10.2f, 1.0));
    VERIFY(winArgs.GetVec4("-color") == vec4(1.0f, 2.0f, 3.0f, 1.0f));
}

} // namespace Test
