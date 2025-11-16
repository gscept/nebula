//------------------------------------------------------------------------------
//  databasetest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testaddon/databasetest.h"
#include "testaddon/dbattrs.h"
#include "io/ioserver.h"
#include "io/filestream.h"
#include "db/database.h"
#include "db/sqlite3/sqlite3factory.h"

namespace Test
{
__ImplementClass(Test::DatabaseTest, 'dbtt', Test::TestCase);

using namespace IO;
using namespace Db;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
DatabaseTest::Run()
{
    Ptr<IO::IoServer> ioServer = IO::IoServer::Create();    
//    ioServer->RegisterUriScheme("file", IO::FileStream::RTTI);
    Ptr<Db::DbFactory> dbFactory = Db::Sqlite3Factory::Create();
    const URI uri("temp:nebula-db-test/databasetest.db3");

    // first make sure we start from scratch
    if (ioServer->FileExists(uri))
    {
        ioServer->DeleteFile(uri);
        n_assert(!ioServer->FileExists(uri));
    }

    // create a database object and open the database
    Ptr<Database> db = dbFactory->CreateDatabase();
    db->SetURI(uri);
    db->SetAccessMode(Database::ReadWriteCreate);
    VERIFY(db->Open());
    VERIFY(db->IsOpen());

    // create some table layouts and add them to the database
    Ptr<Table> table1 = dbFactory->CreateTable();
    table1->SetName("Table1");
    table1->AddColumn(Column(Attr::GuidValue, Column::Primary));
    table1->AddColumn(Column(Attr::BoolValue));
    table1->AddColumn(Column(Attr::StringValue, Column::Indexed));

    Ptr<Table> table2 = dbFactory->CreateTable();
    table2->SetName("Table2");
    table2->AddColumn(Column(Attr::GuidValue, Column::Primary));
    table2->AddColumn(Column(Attr::IntValue, Column::Indexed));
    table2->AddColumn(Column(Attr::Float4Value));
    table2->AddColumn(Column(Attr::Matrix44Value));

    Ptr<Table> table3 = dbFactory->CreateTable();
    table3->SetName("Table3");
    table3->AddColumn(Column(Attr::GuidValue, Column::Primary));
    table3->AddColumn(Column(Attr::BlobValue));

    // do some Table verification
    VERIFY(table1->GetName() == "Table1");
    VERIFY(!table1->IsConnected());
    VERIFY(3 == table1->GetNumColumns());
    VERIFY(table1->HasColumn(Attr::GuidValue));
    VERIFY(table1->HasColumn("BoolValue"));
    VERIFY(table1->HasColumn(FourCC('sval')));
    VERIFY(!table1->HasColumn(Attr::BlobValue));
    VERIFY(table1->GetColumn(Attr::GuidValue).GetType() == Column::Primary);
    VERIFY(!(table1->GetColumn(Attr::BoolValue).GetType() == Column::Primary));

    // add tables to database
    db->AddTable(table1);    
    table1->CommitChanges();
    db->AddTable(table2);
    table2->CommitChanges();
    db->AddTable(table3);
    table3->CommitChanges();
    VERIFY(3 == db->GetNumTables());
    VERIFY(db->GetTableByIndex(0) == table1);
    VERIFY(db->GetTableByIndex(1) == table2);
    VERIFY(db->GetTableByIndex(2) == table3);
    VERIFY(db->HasTable("Table1"));
    VERIFY(db->HasTable("Table2"));
    VERIFY(db->HasTable("Table3"));
    VERIFY(!db->HasTable("NoSuchTable"));
    VERIFY(db->GetTableByName("Table1") == table1);
    VERIFY(db->GetTableByName("Table2") == table2);
    VERIFY(db->GetTableByName("Table3") == table3);

    // close the database, this will write back any changes
    db->Close();
    VERIFY(!db->IsOpen());

    // re-open the database, the tables which we had added 
    // should now show up automatically
    VERIFY(db->Open());
    VERIFY(db->IsOpen());
    VERIFY(3 == db->GetNumTables());
    VERIFY(db->HasTable("Table1"));
    VERIFY(db->HasTable("Table2"));
    VERIFY(db->HasTable("Table3"));
    VERIFY(!db->HasTable("NoSuchTable"));

    // re-assign table object
    table1 = db->GetTableByName("Table1");
    VERIFY(table1->IsConnected());
    VERIFY(3 == table1->GetNumColumns());
    VERIFY(table1->HasColumn(Attr::GuidValue));
    VERIFY(table1->HasColumn("BoolValue"));
    VERIFY(table1->HasColumn(FourCC('sval')));
    VERIFY(!table1->HasColumn(Attr::BlobValue));
    VERIFY(table1->GetColumn(Attr::GuidValue).GetType() == Column::Primary);
    VERIFY(table1->GetColumn(Attr::BoolValue).GetType() != Column::Primary);
    VERIFY(table1->GetColumn(Attr::GuidValue).GetType() != Column::Indexed);
    VERIFY(table1->GetColumn(Attr::BoolValue).GetType() != Column::Indexed);

    // rename table1, this should immediately rename the database table name
    table1->SetName("Table1_Renamed");
    VERIFY(db->HasTable("Table1_Renamed"));
    VERIFY(!db->HasTable("Table1"));
    
    // add a few columns to table 1 (CommnitChanges() should be called in Close())
    table1->AddColumn(Column(Attr::Float4Value, Column::Indexed));
    table1->AddColumn(Column(Attr::BlobValue));    
    table1->CommitChanges();
    db->Close();
    VERIFY(!db->IsOpen());

    // open the database again and check if the above changes made it
    db->Open();
    VERIFY(db->IsOpen());
    VERIFY(db->HasTable("Table1_Renamed"));
    VERIFY(!db->HasTable("Table1"));
    table1 = db->GetTableByName("Table1_Renamed");
    VERIFY(5 == table1->GetNumColumns());
    VERIFY(table1->HasColumn(Attr::Float4Value));
    VERIFY(table1->HasColumn(Attr::BlobValue));
    VERIFY(table1->GetColumn(Attr::Float4Value).GetType() == Column::Indexed);
    VERIFY(table1->GetColumn(Attr::BlobValue).GetType() != Column::Indexed);
    db->Close();
}

}
