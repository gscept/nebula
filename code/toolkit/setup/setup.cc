//------------------------------------------------------------------------------
#include "stdneb.h"
#include "setupapplication.h"

//------------------------------------------------------------------------------
/**
*/
void __cdecl
main(int argc, const char** argv)
{
	Util::CommandLineArgs args(argc, argv);
	Tools::SetupApplication app;
	app.SetCompanyName("gscept");
	app.SetAppTitle("Nebula Setup application");
	app.SetCmdLineArgs(args);
	if (app.Open())
	{
		app.Run();
		app.Close();
	}
	app.Exit();
}