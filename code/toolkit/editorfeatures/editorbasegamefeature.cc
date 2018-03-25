//------------------------------------------------------------------------------
//  managers/editorbasegamefeature.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "basegamefeature/basegamefeatureunit.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "physicsfeature/physicsfeatureunit.h"
#include "appgame/gameapplication.h"
#include "core/factory.h"
#include "basegamefeature/basegameattr/basegameattributes.h"
#include "loader/loaderserver.h"
#include "appgame/appconfig.h"
#include "game/gameserver.h"
#include "game/gameserver.h"
#include "db/dbserver.h"
#include "io/ioserver.h"
#include "io/console.h"
#include "loader/entityloader.h"
#include "loader/environmentloader.h"
#include "basegametiming/systemtimesource.h"
#include "basegametiming/inputtimesource.h"
#include "basegametiming/gametimesource.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "debugrender/debugrender.h"
#include "http/httpserverproxy.h"
#include "debug/objectinspectorhandler.h"
// include all properties for known by managers::factorymanager
#include "properties/timeproperty.h"
#include "properties/transformableproperty.h"
#include "editorbasegamefeature.h"
#include "basegametiming/timemanager.h"
#include "statehandlers/gamestatehandler.h"
#include "io/assignregistry.h"
#include "system/nebulasettings.h"
#include "game/levelexporter.h"
#include "game/templateexporter.h"
#include "db/sqlite3/sqlite3factory.h"
#include "posteffect/posteffectexporter.h"
#include "managers/levelattrsmanager.h"

namespace Toolkit
{
	__ImplementClass(EditorBaseGameFeatureUnit, 'EBGF' , BaseGameFeature::BaseGameFeatureUnit);
	__ImplementSingleton(EditorBaseGameFeatureUnit);

using namespace Timing;
using namespace App;
using namespace Game;
using namespace GraphicsFeature;
using namespace BaseGameFeature;

//------------------------------------------------------------------------------
/**
*/
EditorBaseGameFeatureUnit::EditorBaseGameFeatureUnit()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
EditorBaseGameFeatureUnit::~EditorBaseGameFeatureUnit()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorBaseGameFeatureUnit::OnActivate()
{
	FeatureUnit::OnActivate();

	// setup database subsystem
	this->dbServer = Db::DbServer::Create();
	this->dbServer->SetWorkingDbInMemory(true);

	// disable autosaving
	this->enableAutosave = false;

	Ptr<IO::IoServer> ioServer = IO::IoServer::Instance();

	// setup toolkit assign
	if (System::NebulaSettings::Exists("gscept","ToolkitShared", "path"))
	{
		Util::String toolkitdir = System::NebulaSettings::ReadString("gscept","ToolkitShared", "path");
		IO::AssignRegistry::Instance()->SetAssign(IO::Assign("sdk",toolkitdir));
	}else
	{
		IO::AssignRegistry::Instance()->SetAssign(IO::Assign("sdk","root:"));
	}

	IO::AssignRegistry::Instance()->SetAssign(IO::Assign("sdk","root:"));

	IO::AssignRegistry::Instance()->SetAssign(IO::Assign("editordb","root:intermediate/editordb"));


	if(ioServer->FileExists("editordb:game.db4"))
	{
		ioServer->DeleteFile("editordb:game.db4");
	}
	if(ioServer->FileExists("editordb:static.db4"))
	{
		ioServer->DeleteFile("editordb:static.db4");
	}

	// create our own versions of db files
	{
		Ptr<EditorBlueprintManager> bm = EditorBlueprintManager::Create();

		bm->Open();
        ToolkitUtil::Logger logger;
		bm->SetLogger(&logger);
        bm->ParseProjectInfo("toolkit:projectinfo.xml");
        bm->ParseProjectInfo("proj:projectinfo.xml");
        bm->ParseBlueprint("toolkit:data/tables/blueprints.xml");
        bm->ParseTemplates("toolkit:data/tables/db");
        bm->UpdateAttributeProperties();
        bm->CreateMissingTemplates();
        bm->CreateDatabases("editordb:");
		bm = 0;
	}

	if (!this->dbServer->OpenStaticDatabase("editordb:static.db4"))
	{
		n_error("BaseGameFeature: Failed to open static database 'editordb:static.db4'!");
	}
	// open game database to load global attrs
	if (!this->dbServer->OpenGameDatabase("editordb:game.db4"))
	{
		n_error("BaseGameFeature: Failed to open static database 'editordb:game.db4'!");
	}
		
	// attach loader to BaseGameFeature::LoaderServer
	Ptr<BaseGameFeature::EntityLoader> entityloader = BaseGameFeature::EntityLoader::Create();	
	this->loaderServer->AttachEntityLoader(entityloader.upcast<BaseGameFeature::EntityLoaderBase>());	

	// create manager and attach to fetaure
	this->timeManager = TimeManager::Create();
	FactoryManager::SetBlueprintsFilename("blueprints.xml", "toolkit:data/tables/");
	if (!this->factoryManager.isvalid())
	{
		this->factoryManager = FactoryManager::Create();		
	}	

	this->focusManager = FocusManager::Create();
	this->entityManager = EntityManager::Create();
	this->globalAttrManager = GlobalAttrsManager::Create();
    this->levelAttrManager = LevelAttrsManager::Create();
	this->categoryManager = CategoryManager::Create();	
	this->audioManager = AudioManager::Create();

	this->AttachManager(this->timeManager.upcast<Game::Manager>());
	this->AttachManager(this->factoryManager.upcast<Game::Manager>());
	this->AttachManager(this->focusManager.upcast<Game::Manager>());
	this->AttachManager(this->entityManager.upcast<Game::Manager>());
	this->AttachManager(this->globalAttrManager.upcast<Game::Manager>()); 
    this->AttachManager(this->levelAttrManager.upcast<Game::Manager>());
    this->AttachManager(this->categoryManager.upcast<Game::Manager>());
	this->AttachManager(this->audioManager.upcast<Game::Manager>());

	this->envQueryManager = EnvQueryManager::Create();    
	this->AttachManager(this->envQueryManager.upcast<Game::Manager>());

	Ptr<SystemTimeSource> systemTimeSource = SystemTimeSource::Create();
	Ptr<GameTimeSource> gameTimeSource = GameTimeSource::Create();
	Ptr<InputTimeSource> inputTimeSource = InputTimeSource::Create();

	timeManager->AttachTimeSource(systemTimeSource.upcast<TimeSource>());
	timeManager->AttachTimeSource(gameTimeSource.upcast<TimeSource>());
	timeManager->AttachTimeSource(inputTimeSource.upcast<TimeSource>());

#if __NEBULA3_HTTP__
	// create handler for http debug requests
	this->debugRequestHandler = Debug::ObjectInspectorHandler::Create();
	Http::HttpServerProxy::Instance()->AttachRequestHandler(this->debugRequestHandler);
#endif

	// close after all globals loading from globalattrmanager and everyone who needs it on start
	//this->dbServer->CloseGameDatabase();
    // setup vib interface
    this->vibInterface = Vibration::VibrationInterface::Create();
    this->vibInterface->Open();
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorBaseGameFeatureUnit::NewGame()
{
	BaseGameFeature::UserProfile* userProfile = BaseGameFeature::LoaderServer::Instance()->GetUserProfile();    
	App::StateHandler* curAppStateHandler = App::GameApplication::Instance()->GetCurrentStateHandler();
	n_assert(curAppStateHandler);

	// open database in NewGame mode
	bool dbOpened = false;
#if __WIN32__ 
	//dbOpened = Db::DbServer::Instance()->OpenNewGame(userProfile->GetProfileDirectory(), userProfile->GetDatabasePath());    
#else 
//	dbOpened = Db::DbServer::Instance()->OpenNewGame(userProfile->GetProfileDirectory(), "export:db/game.db4");
#endif
	//n_assert(dbOpened);

	// load attributes and reload categories
	GlobalAttrsManager::Instance()->LoadAttributes();

	// setup the world	
	this->SetCurrentLevel(((BaseGameFeature::GameStateHandler*)curAppStateHandler)->GetLevelName());
	curAppStateHandler->OnLoadBefore();
	this->SetupWorldFromCurrentLevel();
	curAppStateHandler->OnLoadAfter();

	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorBaseGameFeatureUnit::OnEndFrame()
{
	FeatureUnit::OnEndFrame();
}

}