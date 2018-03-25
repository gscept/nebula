//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbxbatcher3app.h"

//------------------------------------------------------------------------------
/**
*/
int __cdecl
main(int argc, const char** argv)
{
	Util::CommandLineArgs args(argc, argv);
	Toolkit::FBXBatcher3App app;
	app.SetCompanyName("gscept");
	app.SetAppTitle("Nebula FBX batcher");
	app.SetCmdLineArgs(args);
	if (app.Open())
	{
		app.Run();
		app.Close();
	}
	app.Exit();
}
