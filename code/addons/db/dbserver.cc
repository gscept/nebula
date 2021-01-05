//------------------------------------------------------------------------------
//  db/dbserver.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/dbserver.h"
#include "db/dbfactory.h"
#include "io/ioserver.h"

namespace Db
{
__ImplementClass(Db::DbServer, 'DBSV', Core::RefCounted);
__ImplementSingleton(Db::DbServer);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
DbServer::DbServer():
    workDbInMemory(false)
{
    __ConstructSingleton;
    this->dbFactory = Db::Sqlite3Factory::Create();
}

//------------------------------------------------------------------------------
/**
*/
DbServer::~DbServer()
{
    n_assert(!this->IsStaticDatabaseOpen());
    n_assert(!this->IsGameDatabaseOpen());
    this->dbFactory = nullptr;
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Directly open the static database. The static database contains
    immutable read-only data.
*/
bool
DbServer::OpenStaticDatabase(const String& dbURI)
{
    n_assert(!this->IsStaticDatabaseOpen());
    n_assert(dbURI.IsValid());

    this->staticDatabase = DbFactory::Instance()->CreateDatabase();
    this->staticDatabase->SetURI(dbURI);
    // FIXME why was this as exclusive, it is read only anyway
    this->staticDatabase->SetExclusiveMode(false);
    this->staticDatabase->SetAccessMode(Database::ReadOnly);
    this->staticDatabase->SetIgnoreUnknownColumns(true);
    this->staticDatabase->SetInMemoryDatabase(this->workDbInMemory);
    if (!this->staticDatabase->Open())
    {
        n_error("Failed to open static database '%s'!", dbURI.AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the static database.
*/
void
DbServer::CloseStaticDatabase()
{
    n_assert(this->IsStaticDatabaseOpen());
    this->staticDatabase->Close();
    n_assert(this->staticDatabase->GetRefCount() == 1);
    this->staticDatabase = nullptr;
}

//------------------------------------------------------------------------------
/**
    Directly open the game database, which contains dynamic game data.
*/
bool
DbServer::OpenGameDatabase(const String& dbURI)
{
    n_assert(!this->IsGameDatabaseOpen());
    n_assert(dbURI.IsValid());
    n_assert(!this->gameDatabase.isvalid());

    this->gameDatabase = DbFactory::Instance()->CreateDatabase();
    this->gameDatabase->SetURI(dbURI);
    this->gameDatabase->SetExclusiveMode(true);
    this->gameDatabase->SetAccessMode(Database::ReadWriteExisting);
    this->gameDatabase->SetIgnoreUnknownColumns(true);
    this->gameDatabase->SetInMemoryDatabase(this->workDbInMemory);
    if (!this->gameDatabase->Open())
    {
        n_error("Failed to open game database '%s'!", dbURI.AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the game database.
*/
void
DbServer::CloseGameDatabase()
{
    n_assert(this->IsGameDatabaseOpen());
    this->gameDatabase->Close();
    n_assert(this->gameDatabase->GetRefCount() == 1);
    this->gameDatabase = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
DbServer::Close()
{
    if (this->IsGameDatabaseOpen())
    {
        this->CloseGameDatabase();
    }
    if (this->IsStaticDatabaseOpen())
    {
        this->CloseStaticDatabase();
    }
}

//------------------------------------------------------------------------------
/**
    This deletes the current game state database.
*/
void
DbServer::DeleteCurrentGame(const String& profileURI)
{
    if (this->IsGameDatabaseOpen())
    {
        this->CloseGameDatabase();
    }

    // delete current database
    IO::IoServer* ioServer = IO::IoServer::Instance();
    if (ioServer->FileExists(profileURI))
    {
        bool dbDeleted = ioServer->DeleteFile(profileURI);
        n_assert(dbDeleted);
    }
}

//------------------------------------------------------------------------------
/**
    This opens the database in New Game mode: an original database will be
    copied into the user's profile directory into a Current Game State
    database and opened.
*/
bool
DbServer::OpenNewGame(const String& profileURI, const String& dbURI)
{
    n_assert(profileURI.IsValid());

    // make sure we're not open
    if (this->IsGameDatabaseOpen())
    {
        this->CloseGameDatabase();
    }
    
    if (!this->workDbInMemory)
    {        
        // make sure the target directory exists
        IO::IoServer* ioServer = IO::IoServer::Instance();
        ioServer->CreateDirectory(profileURI);
        
        // delete current database
        if (ioServer->FileExists(dbURI))
        {
            bool dbDeleted = ioServer->DeleteFile(dbURI);
            n_assert(dbDeleted);
        }
        IO::URI journalURI = dbURI + String("-journal");
        // delete current database journalfile
        if (ioServer->FileExists(journalURI))
        {
            bool dbDeleted = ioServer->DeleteFile(journalURI);
            n_assert(dbDeleted);
        }
        bool dbCopied = ioServer->CopyFile("export:db/game.db4", dbURI);
        n_assert(dbCopied);
        // open the copied database file
        return this->OpenGameDatabase(dbURI);       
    }
    else
    {
        return this->OpenGameDatabase("export:db/game.db4");
    }    
}

//------------------------------------------------------------------------------
/**
    This opens the database in Continue Game mode: the current game database
    in the user profile directory will simply be opened.
*/
bool
DbServer::OpenContinueGame(const String& profileURI)
{
    n_assert(profileURI.IsValid());

    // make sure we're not open
    if (this->IsGameDatabaseOpen())
    {
        this->CloseGameDatabase();
    }
    
    // open the current game database
    bool dbOpened = this->OpenGameDatabase(profileURI);
    return dbOpened;
}

//------------------------------------------------------------------------------
/**
    Return true if a current game database exists.
*/
bool
DbServer::CurrentGameExists(const String& profileURI) const
{
    n_assert(profileURI.IsValid());
    return IO::IoServer::Instance()->FileExists(profileURI);
}

//------------------------------------------------------------------------------
/**
    This opens the database in Load Game mode. This will overwrite the
    current game database with a save game database and open this as
    usual.
*/
bool
DbServer::OpenLoadGame(const String& profileURI, const String& dbURI, const String& saveGameURI)
{
    n_assert(profileURI.IsValid());
    
    // make sure we're not open
    if (this->IsGameDatabaseOpen())
    {
        this->CloseGameDatabase();
    }
    
    IO::IoServer* ioServer = IO::IoServer::Instance();
    if (!this->workDbInMemory)
    {
        // make sure the target directory exists        
        ioServer->CreateDirectory(profileURI);
    }
    
    // check if save game exists
    if (!ioServer->FileExists(saveGameURI))
    {
        return false;
    }
    
    if (!this->workDbInMemory)
    {
        // delete current database
        if (ioServer->FileExists(dbURI))
        {
            bool dbDeleted = ioServer->DeleteFile(dbURI);
            n_assert(dbDeleted);
        }
        bool dbCopied = ioServer->CopyFile(saveGameURI, dbURI);
        n_assert(dbCopied);
    }
    
    // open the copied database file
    bool dbOpened = this->OpenGameDatabase(dbURI);
    return dbOpened;
}

//------------------------------------------------------------------------------
/**
    This creates a new save game by making a copy of the current world
    database into the savegame directory. If a savegame of that exists,
    it will be overwritten.
*/
bool
DbServer::CreateSaveGame(const String& profileURI, const String& dbURI, const String& saveGameURI)
{
    n_assert2(!this->workDbInMemory, "TODO: Savegame from memory db not implemented yet!");
    
    // make sure the target directory exists
    IO::IoServer* ioServer = IO::IoServer::Instance();
    ioServer->CreateDirectory(profileURI);

    // check if current world database exists
    if (!ioServer->FileExists(dbURI))
    {
        return false;
    }

    // delete save game file if already exists
    if (ioServer->FileExists(saveGameURI))
    {
        bool saveGameDeleted = ioServer->DeleteFile(saveGameURI);
        n_assert(saveGameDeleted);
    }
    else
    {
        String saveGamePath = saveGameURI.ExtractDirName();        
        ioServer->CreateDirectory(saveGamePath);
    }
    bool saveGameCopied = ioServer->CopyFile(dbURI, saveGameURI);
    n_assert(saveGameCopied);
    return true;
}

} // namespace Db
