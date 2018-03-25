//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "raytraceapp.h"

//------------------------------------------------------------------------------
/**
*/
int __cdecl
main(int argc, const char** argv)
{
	Util::CommandLineArgs args(argc, argv);
	Toolkit::RaytraceApp app;
	app.SetCompanyName("gscept");
	app.SetAppTitle("NebulaT Raytracer");
	app.SetCmdLineArgs(args);
	if (app.Open())
	{
		app.Run();
		app.Close();
	}
	app.Exit();
}
