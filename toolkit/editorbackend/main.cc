//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/commandlineargs.h"
#include "core/coreserver.h"
#include "io/ioserver.h"
#include "io/console.h"
#include "io/logfileconsolehandler.h"
#include "appsettings.h"
#include "util/string.h"
#include "net/tcpserver.h"
#include "net/messageclientconnection.h"
#include "db/dbserver.h"
#include "db/sqlite3/sqlite3factory.h"
#include "attributes.h"
#include "memdb/database.h"

using namespace Universe;

#define RET_NO_ERROR 0
#define RET_INVALID_CONFIG 1
#define RET_NET_ERROR 2
#define RET_DB_ERROR 3

//------------------------------------------------------------------------------
/**
*/
int __cdecl
main(int argc, const char** argv)
{
    Util::CommandLineArgs args(argc, argv);

    Ptr<Core::CoreServer> coreServer = Core::CoreServer::Create();
    coreServer->SetCompanyName("gscept");
    coreServer->SetAppName("Universe");
    coreServer->Open();
    
    // initialize io subsystem
    Ptr<IO::IoServer> ioServer = IO::IoServer::Create();

    Ptr<IO::LogFileConsoleHandler> logFileHandler = IO::LogFileConsoleHandler::Create();
    IO::Console::Instance()->AttachHandler(logFileHandler.upcast<IO::ConsoleHandler>());

    AppSettings::Create();

    Util::String projectName = AppSettings::GetString("project");
    if (projectName.IsEmpty())
    {
        n_warning("ERROR: Project not configured!\nCheck the config.json file in the executable binarys folder.\n");
        return RET_INVALID_CONFIG;
    }

    n_printf("Project is: %s\n", projectName.AsCharPtr());
    
    // --- Database ---

    Ptr<Db::DbServer> dbServer = Db::DbServer::Create();
    Util::String dbPath;
    dbPath.Format("bin:%s", projectName.AsCharPtr());

    if (!IO::IoServer::Instance()->DirectoryExists(dbPath))
        IO::IoServer::Instance()->CreateDirectoryA(dbPath);

    dbPath.Append("/world.sqlite3");

    // Check if database exists, otherwise, create a DB
    if (!IO::IoServer::Instance()->FileExists(dbPath))
    {
        Ptr<Db::DbFactory> dbFactory = Db::Sqlite3Factory::Instance();
        Ptr<Db::Database> db = dbFactory->CreateDatabase();
        db->SetURI(dbPath);
        db->SetAccessMode(Db::Database::ReadWriteCreate);
        if (!db->Open())
            return RET_DB_ERROR;
            
        Ptr<Db::Table> propertyDefinitionTable = dbFactory->CreateTable();
        propertyDefinitionTable->SetName("PropertyDefinitions");
        propertyDefinitionTable->AddColumn(Db::Column(Attr::PropertyName, Db::Column::Primary));
        propertyDefinitionTable->AddColumn(Db::Column(Attr::PropertyType));
        propertyDefinitionTable->AddColumn(Db::Column(Attr::PropertyDefaultValue));
        db->AddTable(propertyDefinitionTable);
        propertyDefinitionTable->CommitChanges();

        db->Close();
    }

    dbServer->OpenGameDatabase(dbPath);
    
    // --- MemDB ---

    Ptr<MemDb::Database> worldDatabase = MemDb::Database::Create();

    // --- TCP Server ---

    if (AppSettings::GetString("ip").IsEmpty())
    {
        n_warning("ERROR: No ip provided!\nCheck the config.json file in the executable binarys folder.\n");
        return RET_INVALID_CONFIG;
    }

    n_printf("Starting server at ");
    Ptr<Net::TcpServer> tcpServer = Net::TcpServer::Create();
    
    n_printf("%s:%i...\n", AppSettings::GetString("ip").AsCharPtr(), AppSettings::GetInt("port"));
    tcpServer->SetAddress(Net::IpAddress(AppSettings::GetString("ip"), AppSettings::GetInt("port")));
    tcpServer->SetClientConnectionClass(Net::MessageClientConnection::RTTI);
    if (!tcpServer->Open())
    {
        n_warning("Could not open TCP server!\n");
        return RET_NET_ERROR;
    }

    n_printf("--- Server up and running!\n\n");

    while (tcpServer->IsOpen())
    {
        auto connections = tcpServer->Recv();
        //for (auto const& connection : connections)
        //{
        //}
    }

    return RET_NO_ERROR;
}
