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
    this->Verify(posixArgs.GetCmdName() == "mycommand");
    this->Verify(posixArgs.HasArg("-help"));
    this->Verify(posixArgs.HasArg("-file"));
    this->Verify(posixArgs.HasArg("-retries"));
    this->Verify(posixArgs.HasArg("-weight"));
    this->Verify(posixArgs.HasArg("-pos"));
    this->Verify(posixArgs.HasArg("-color"));
    this->Verify(!posixArgs.HasArg("-bla"));

    this->Verify(winArgs.GetCmdName() == "mycommand");
    this->Verify(winArgs.HasArg("-help"));
    this->Verify(winArgs.HasArg("-file"));
    this->Verify(winArgs.HasArg("-retries"));
    this->Verify(winArgs.HasArg("-weight"));
    this->Verify(winArgs.HasArg("-pos"));
    this->Verify(winArgs.HasArg("-color"));
    this->Verify(!winArgs.HasArg("-bla"));

    this->Verify(posixArgs.GetBoolFlag("-help") == true);
    this->Verify(posixArgs.GetString("-file") == "filename.txt");
    this->Verify(posixArgs.GetInt("-retries") == 10);
    this->Verify(posixArgs.GetFloat("-weight") == 1.34f);
    this->Verify(posixArgs.GetFloat4("-color") == float4(1.0f, 2.0f, 3.0f, 1.0f));

    this->Verify(winArgs.GetBoolFlag("-help") == true);
    this->Verify(winArgs.GetString("-file") == "filename.txt");
    this->Verify(winArgs.GetInt("-retries") == 10);
    this->Verify(winArgs.GetFloat("-weight") == 1.34f);
    this->Verify(winArgs.GetFloat4("-pos") == float4(1.03f, 4.53f, 10.2f, 1.0));
    this->Verify(winArgs.GetFloat4("-color") == float4(1.0f, 2.0f, 3.0f, 1.0f));
}

} // namespace Test
