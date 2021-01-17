#ifndef DB_DBSERVER_H
#define DB_DBSERVER_H
//------------------------------------------------------------------------------
/**
    @class Db::DbServer
  
    Provides highlevel access to the world database.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "db/database.h"
#include "db/sqlite3/sqlite3factory.h"

//------------------------------------------------------------------------------
namespace Db
{
class DbServer : public Core::RefCounted
{
    __DeclareClass(DbServer);
    __DeclareSingleton(DbServer);
public:
    /// constructor
    DbServer();
    /// destructor
    virtual ~DbServer();

    /// open the static world database directly
    bool OpenStaticDatabase(const Util::String& dbUri);
    /// open the dynamic game database directly
    bool OpenGameDatabase(const Util::String& dbUri);
    /// open the db subsystem in NewGame mode
    bool OpenNewGame(const Util::String& profileURI, const Util::String& dbURI);
    /// open the db subsysten in ContinueGame mode
    bool OpenContinueGame(const Util::String& profileURI);
    /// open the db subsystem in LoadGame mode
    bool OpenLoadGame(const Util::String& profileURI, const Util::String& dbURI, const Util::String& saveGameURI);
    /// delete current game database file
    void DeleteCurrentGame(const Util::String& profileURI);
    /// close the static database
    void CloseStaticDatabase();
    /// close the dynamic database
    void CloseGameDatabase();
    /// general close method
    void Close();
    /// return true if static database is open
    bool IsStaticDatabaseOpen() const;
    /// return true if game database is open
    bool IsGameDatabaseOpen() const;    
    /// create a save game
    bool CreateSaveGame(const Util::String& profileURI, const Util::String& dbURI, const Util::String& saveGameURI);
    /// get the world database (for dynamic gameplay data)
    const Ptr<Database>& GetGameDatabase() const;
    /// get static database (for constant read-only data)
    const Ptr<Database>& GetStaticDatabase() const;
    /// return true if a current world database exists
    bool CurrentGameExists(const Util::String& profileURI) const;
    /// set flag to load database as working db into memory
    void SetWorkingDbInMemory(bool b);

private:
    bool isOpen;
    bool workDbInMemory;
    Ptr<Db::Sqlite3Factory> dbFactory;
    Ptr<Database> staticDatabase;
    Ptr<Database> gameDatabase;
}; 

//------------------------------------------------------------------------------
/**
*/
inline bool
DbServer::IsStaticDatabaseOpen() const
{
    return this->staticDatabase.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline bool
DbServer::IsGameDatabaseOpen() const
{
    return this->gameDatabase.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Database>&
DbServer::GetStaticDatabase() const
{
    return this->staticDatabase;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Database>&
DbServer::GetGameDatabase() const
{
    return this->gameDatabase;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
DbServer::SetWorkingDbInMemory(bool b)
{
    this->workDbInMemory = b;
}

} // namespace Db
//------------------------------------------------------------------------------
#endif