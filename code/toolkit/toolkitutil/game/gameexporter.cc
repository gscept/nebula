//------------------------------------------------------------------------------
//  gameexporter.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/gameexporter.h"
#include "io/ioserver.h"
#include "db/sqlite3/sqlite3factory.h"
#include "db/writer.h"
#include "attr/attributedefinitionbase.h"
#include "scriptfeature/scriptattr/scriptattributes.h"
#include "leveldbwriter.h"
#ifdef WIN32
#include "io/win32/win32consolehandler.h"
#endif

using namespace IO;
using namespace Db;
using namespace Attr;
using namespace Util;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::GameExporter, 'GAEX', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
GameExporter::GameExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
GameExporter::~GameExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
GameExporter::Open()
{
	ExporterBase::Open();

}

//------------------------------------------------------------------------------
/**
*/
void 
GameExporter::Close()
{
	ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**
	Exports both templates and levels with a clean database (removes and creates backup of previous)
*/
void 
GameExporter::ExportAll()
{
	
    String projectFolder = "proj:";
	
	Ptr<ToolkitUtil::ToolkitConsoleHandler> console = ToolkitUtil::ToolkitConsoleHandler::Instance();
	console->Clear();
    IO::AssignRegistry::Instance()->SetAssign(Assign("home","proj:"));

    Ptr<Db::DbFactory> sqlite3Factory;
    if(Db::DbFactory::HasInstance())
    {
        sqlite3Factory = Db::Sqlite3Factory::Instance();
    }
    else
    {
        sqlite3Factory = Db::Sqlite3Factory::Create();
    }
	
	if (!IoServer::Instance()->DirectoryExists("export:data/tables"))
	{
		IoServer::Instance()->CreateDirectory("export:data/tables");
	}

	IO::IoServer * ioServer = IO::IoServer::Instance();
	Util::Array<Util::String> xmlfiles = ioServer->ListFiles("proj:data/tables/", "*.xml", false);

	ToolLog log("Tables");
	for (Util::Array<Util::String>::Iterator iter = xmlfiles.Begin(); iter != xmlfiles.End(); iter++)
	{
		if (*iter != "blueprints.xml")
		{
			Util::String from("proj:data/tables/" + *iter);
			Util::String to("export:data/tables/" + *iter);			
			ioServer->CopyFile(from, to);
			IO::URI fromUri(from);
			IO::URI toUri(to);
			n_printf("Copying table %s to %s\n", fromUri.LocalPath().AsCharPtr(), toUri.LocalPath().AsCharPtr());
		}		
	}
	log.AddEntry(console, "Game Batcher", "data/tables");
	console->Clear();
	this->logs.Append(log);

	ToolLog blog("Blueprints");

    Ptr<Toolkit::EditorBlueprintManager> bm;
    if(Toolkit::EditorBlueprintManager::HasInstance())
    {
        bm = Toolkit::EditorBlueprintManager::Instance();
    }
    else
    {
        bm = Toolkit::EditorBlueprintManager::Create();
        bm->SetLogger(this->logger);

        bm->ParseProjectInfo("proj:projectinfo.xml");
        bm->ParseProjectInfo("toolkit:projectinfo.xml");

        bm->ParseBlueprint("proj:data/tables/blueprints.xml");    
        bm->ParseBlueprint("toolkit:data/tables/blueprints.xml");
        // templates need to be parsed from tookit first to add virtual templates with their attributes
        bm->ParseTemplates("toolkit:data/tables/db");
        bm->ParseTemplates("proj:data/tables/db");


        bm->UpdateAttributeProperties();
        bm->CreateMissingTemplates();
    }
    
   
	if (!bm->CreateDatabases("export:/db/"))
	{
		n_warning("Aborting export of game data as databases are in use\n");
		return;
	}
	bm->SaveBlueprint("export:data/tables/blueprints.xml");

	blog.AddEntry(console, "Blueprint Manager", "data/tables");
	console->Clear();
	this->logs.Append(blog);

	ToolLog llog("Levels");

    Ptr<Db::Database> gamedb = Db::DbFactory::Instance()->CreateDatabase();
    gamedb->SetURI("export:db/game.db4");
    gamedb->SetAccessMode(Db::Database::ReadWriteExisting);
    gamedb->Open();
    Ptr<Db::Database> staticdb = Db::DbFactory::Instance()->CreateDatabase();
    staticdb->SetURI("export:db/static.db4");
    staticdb->SetAccessMode(Db::Database::ReadWriteExisting);
    staticdb->Open();
	
	llog.AddEntry(console, "Blueprint Manager", "Databases");
	console->Clear();
	this->logs.Append(llog);

    Ptr<ToolkitUtil::LevelDbWriter> dbwriter = ToolkitUtil::LevelDbWriter::Create();
    dbwriter->Open(gamedb,staticdb);
    String levelDir = "proj:work/levels";
    Array<String> files = IoServer::Instance()->ListFiles(IO::URI(levelDir), "*.xml", true);
    for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {        		
        Ptr<IO::Stream> levelStream = IoServer::Instance()->CreateStream(files[fileIndex]);
        Ptr<XmlReader> xmlReader = XmlReader::Create();
        levelStream->Open();
        xmlReader->SetStream(levelStream);
        xmlReader->Open();
        dbwriter->LoadXmlLevel(xmlReader);
        xmlReader->Close();
        levelStream->Close();        
		llog.AddEntry(console, "Level Writer", files[fileIndex]);
		console->Clear();
		this->logs.Append(llog);
		if (!dbwriter->GetReferences().IsEmpty())
		{
			dbwriter->SetReferenceMode(true);
			const Util::Array<Util::String> refs = dbwriter->GetReferences();
			for (int refIndex = 0; refIndex < refs.Size();refIndex++)
			{
				IO::URI path("proj:work/levels/" + refs[refIndex] + ".xml");
				Ptr<IO::Stream> levelStream = IoServer::Instance()->CreateStream(path);
				Ptr<XmlReader> xmlReader = XmlReader::Create();
				levelStream->Open();
				xmlReader->SetStream(levelStream);
				xmlReader->Open();
				dbwriter->LoadXmlLevel(xmlReader);
				xmlReader->Close();
				levelStream->Close();
				llog.AddEntry(console, "Level Writer", refs[refIndex]);
				console->Clear();
				this->logs.Append(llog);
			}
			dbwriter->SetReferenceMode(false);
			dbwriter->ClearReferences();
		}
    }
    dbwriter->Close();    
    gamedb->Close();
    staticdb->Close();
    bm = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
GameExporter::ExportTables()
{
    String projectFolder = "proj:";


    IO::AssignRegistry::Instance()->SetAssign(Assign("home","proj:"));

    Ptr<Db::DbFactory> sqlite3Factory;
    if(Db::DbFactory::HasInstance())
    {
        sqlite3Factory = Db::Sqlite3Factory::Instance();
    }
    else
    {
        sqlite3Factory = Db::Sqlite3Factory::Create();
    }


    Ptr<Toolkit::EditorBlueprintManager> bm;
    if(Toolkit::EditorBlueprintManager::HasInstance())
    {
        bm = Toolkit::EditorBlueprintManager::Instance();
    }
    else
    {
        bm = Toolkit::EditorBlueprintManager::Create();
        bm->SetLogger(this->logger);

        bm->ParseProjectInfo("proj:projectinfo.xml");
        bm->ParseProjectInfo("toolkit:projectinfo.xml");

        bm->ParseBlueprint("proj:data/tables/blueprints.xml");    
        bm->ParseBlueprint("toolkit:data/tables/blueprints.xml");
        // templates need to be parsed from tookit first to add virtual templates with their attributes
        bm->ParseTemplates("toolkit:data/tables/db");
        bm->ParseTemplates("proj:data/tables/db");


        bm->UpdateAttributeProperties();
        bm->CreateMissingTemplates();
    }
    bm->CreateDatabases("export:/db/");
    bm = 0;
}

} // namespace ToolkitUtil