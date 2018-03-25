//------------------------------------------------------------------------------
#include "stdneb.h"
#include "gamebatcherapp.h"
#ifdef WIN32
#include "io/win32/win32consolehandler.h"
#endif
#include "toolkitconsolehandler.h"


//------------------------------------------------------------------------------
/**
*/
int
main(int argc, const char** argv)
{
	Util::CommandLineArgs args(argc, argv);
	Toolkit::GameBatcherApp app;
	app.SetCompanyName("gscept");
	app.SetAppTitle("Nebula game batcher");
	app.SetCmdLineArgs(args);
	if (app.Open())
	{
		app.Run();
		app.Close();
	}
	app.Exit();
}

extern "C"
{
#ifdef WIN32
	__declspec(dllexport) const char* _cdecl
#else
	const char*
#endif	
	BatchGameData(void)
	{
		Toolkit::GameBatcherApp app;
		app.SetCompanyName("gscept");
		app.SetAppTitle("Nebula game batcher");
		//app.SetCmdLineArgs(args);
		if (app.Open())
		{
			Ptr<IO::Console> console = IO::Console::Instance();
#ifdef WIN32
			const Util::Array<Ptr<IO::ConsoleHandler>> & handlers = console->GetHandlers();
			for (int i = 0; i < handlers.Size(); i++)
			{
				if (handlers[i]->IsA(Win32::Win32ConsoleHandler::RTTI))
				{
					console->RemoveHandler(handlers[i]);
				}
			}
#endif
			app.Run();
			Util::String xmlLogs = app.GetXMLLogs();			
                        char * data;
#ifdef WIN32
		
			data = (char*) CoTaskMemAlloc(xmlLogs.Length() + 1);
#else
			data = Memory::Alloc(Memory::ScratchHeap, xmlLogs.Length() + 1);
#endif
			Memory::Copy(xmlLogs.AsCharPtr(), data, xmlLogs.Length() + 1);			
			app.Close();
			return data;
		}
		return 0;
	}
}
