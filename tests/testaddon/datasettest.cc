//------------------------------------------------------------------------------
//  datasettest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testaddon/datasettest.h"
#include "db/dataset.h"
#include "testaddon/dbattrs.h"
#include "io/ioserver.h"
#include "io/filestream.h"
#include "db/sqlite3/sqlite3factory.h"

namespace Test
{
__ImplementClass(Test::DatasetTest, 'dstt', Test::TestCase);

using namespace IO;
using namespace Db;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
DatasetTest::Run()
{
    Ptr<IO::IoServer> ioServer = IO::IoServer::Create();    
    //ioServer->RegisterUriScheme("file", IO::FileStream::RTTI);
    Ptr<Db::DbFactory> dbFactory = Db::Sqlite3Factory::Create();
    const URI uri("temp:nebula-db-test/datasettest.db3");

    // first make sure we start from a fresh database
    if (ioServer->FileExists(uri))
    {
        ioServer->DeleteFile(uri);
        n_assert(!ioServer->FileExists(uri));
    }

    // create and populate a database
    Ptr<Database> db = dbFactory->CreateDatabase();
    db->SetURI(uri);
    db->SetAccessMode(Database::ReadWriteCreate);
    VERIFY(db->Open());
    VERIFY(db->IsOpen());
    this->PopulateDatabase(db);
    VERIFY(db->HasTable("Persons"));
    VERIFY(db->HasTable("Orders"));
    VERIFY(db->HasTable("Products"));
    VERIFY(db->HasTable("Cities"));

    // do some queries on the database    
    Ptr<Table> persons = db->GetTableByName("Persons");
    Ptr<Dataset> ordersInBerlin = persons->CreateDataset();
    ordersInBerlin->AddAllTableColumns();
   
    const Ptr<FilterSet>& filter = ordersInBerlin->Filter();
    filter->AddEqualCheck(Attr::Attribute(Attr::City, "Berlin"));

    ordersInBerlin->PerformQuery();
    Ptr<ValueTable> results = ordersInBerlin->Values();

    VERIFY(results->GetNumRows() == 2);
    VERIFY(results->GetString(Attr::Name, 0) == "Franz Jaeger");    
    VERIFY(results->GetString(Attr::Name, 1) == "Franz August");    

    db->Close();
}

//------------------------------------------------------------------------------
/**
    Populate the database with tables.
*/
void
DatasetTest::PopulateDatabase(Database* db)
{
    // create a few tables
    Ptr<Table> persons = DbFactory::Instance()->CreateTable();
    persons->SetName("Persons");
    persons->AddColumn(Column(Attr::Name, Column::Primary));    
    persons->AddColumn(Column(Attr::City, Column::Indexed));
    persons->AddColumn(Column(Attr::Street, Column::Indexed));
    persons->AddColumn(Column(Attr::Phone));
    persons->AddColumn(Column(Attr::GUID, Column::Indexed));
    db->AddTable(persons);

    Ptr<Db::Dataset> dataset = persons->CreateDataset();
    dataset->AddAllTableColumns();
    this->PopulatePersonsTable(dataset);
    dataset->CommitChanges();
    persons->CommitChanges();
    
    Ptr<Table> orders = DbFactory::Instance()->CreateTable();    
    orders->SetName("Orders");
    orders->AddColumn(Column(Attr::Name, Column::Indexed));
    orders->AddColumn(Column(Attr::Product, Column::Indexed));
    orders->AddColumn(Column(Attr::Nr));
    orders->AddColumn(Column(Attr::GUID, Column::Primary));
    db->AddTable(orders);

    Ptr<Db::Dataset> datasetOrders = orders->CreateDataset();
    datasetOrders->AddAllTableColumns();    
    this->PopulateOrdersTable(datasetOrders);    
    datasetOrders->CommitChanges();
    orders->CommitChanges();

    Ptr<Table> products = DbFactory::Instance()->CreateTable();
    products->SetName("Products");
    products->AddColumn(Column(Attr::Name, Column::Primary));
    products->AddColumn(Column(Attr::Price));
    products->AddColumn(Column(Attr::Stock));
    products->AddColumn(Column(Attr::GUID, Column::Indexed));
    db->AddTable(products);

    Ptr<Db::Dataset> datasetProducts = products->CreateDataset();
    datasetProducts->AddAllTableColumns();        
    this->PopulateProductsTable(datasetProducts);    
    datasetProducts->CommitChanges();
    products->CommitChanges();

    Ptr<Table> cities = DbFactory::Instance()->CreateTable();
    cities->SetName("Cities");
    cities->AddColumn(Column(Attr::Name, Column::Primary));
    cities->AddColumn(Column(Attr::Country));
    db->AddTable(cities);

    Ptr<Db::Dataset> datasetCities = cities->CreateDataset();
    datasetCities->AddAllTableColumns();
    this->PopulateCitiesTable(datasetCities);    
    datasetCities->CommitChanges();
    cities->CommitChanges();
}

//------------------------------------------------------------------------------
/**
*/
void
DatasetTest::PopulatePersonsTable(Dataset* t)
{
    Ptr<ValueTable> values = t->Values();
    Guid guid;
    IndexT rowIndex = values->AddRow();
    guid.Generate();            
    values->SetString(Attr::Name, rowIndex, "Franz Jaeger");    
    values->SetString(Attr::City, rowIndex, "Berlin");    
    values->SetString(Attr::Street, rowIndex, "Schwedter Strasse");    
    values->SetString(Attr::Phone, rowIndex, "030748403737");    
    values->SetGuid(Attr::GUID, rowIndex, guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Franz August");
    values->SetString(Attr::City, rowIndex, "Berlin");
    values->SetString(Attr::Street, rowIndex, "Bouchéstrasse");
    values->SetString(Attr::Phone, rowIndex, "03043423423");
    values->SetGuid(Attr::GUID, rowIndex, guid);
    
    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Alois Huber");
    values->SetString(Attr::City, rowIndex, "Muenchen");
    values->SetString(Attr::Street, rowIndex, "Prinzregentenstrasse");
    values->SetString(Attr::Phone, rowIndex, "0804839823487");
    values->SetGuid(Attr::GUID, rowIndex,guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Anton Guenther");
    values->SetString(Attr::City, rowIndex, "Bermsgruen");
    values->SetString(Attr::Street, rowIndex, "Gemeindestrasse");
    values->SetString(Attr::Phone, rowIndex, "03774093487");
    values->SetGuid(Attr::GUID, rowIndex,guid);
        
    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Lord Nelson");
    values->SetString(Attr::City, rowIndex, "London");
    values->SetString(Attr::Street, rowIndex, "Trafalgar Square");
    values->SetString(Attr::Phone, rowIndex, "94398793847");
    values->SetGuid(Attr::GUID, rowIndex,guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Pina Colada");
    values->SetString(Attr::City, rowIndex, "Rio de Janeiro");
    values->SetString(Attr::Street, rowIndex, "Baker");
    values->SetString(Attr::Phone, rowIndex, "30390498");
    values->SetGuid(Attr::GUID, rowIndex,guid);
}

//------------------------------------------------------------------------------
/**
*/
void
DatasetTest::PopulateProductsTable(Dataset* t)
{
    Ptr<ValueTable> values = t->Values();
    Guid guid;
    
    IndexT rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "BMW M6");
    values->SetInt(Attr::Price, rowIndex, 120000);
    values->SetInt(Attr::Stock, rowIndex, 10);
    values->SetGuid(Attr::GUID, rowIndex, guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Mixer");
    values->SetInt(Attr::Price, rowIndex, 100);
    values->SetInt(Attr::Stock, rowIndex, 50);
    values->SetGuid(Attr::GUID, rowIndex, guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Banana");
    values->SetInt(Attr::Price, rowIndex, 2);
    values->SetInt(Attr::Stock, rowIndex, 10000);
    values->SetGuid(Attr::GUID, rowIndex, guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Bicycle");
    values->SetInt(Attr::Price, rowIndex, 1500);
    values->SetInt(Attr::Stock, rowIndex, 20);
    values->SetGuid(Attr::GUID, rowIndex, guid);
}

//------------------------------------------------------------------------------
/**
*/
void
DatasetTest::PopulateOrdersTable(Dataset* t)
{
    Ptr<ValueTable> values = t->Values();
    Guid guid;

    IndexT rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Franz Jaeger");
    values->SetString(Attr::Product, rowIndex, "BMW M6");
    values->SetInt(Attr::Nr, rowIndex, 1);
    values->SetGuid(Attr::GUID, rowIndex, guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Franz August");
    values->SetString(Attr::Product, rowIndex, "Banana");
    values->SetInt(Attr::Nr, rowIndex, 10);
    values->SetGuid(Attr::GUID, rowIndex, guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Lord Nelson");
    values->SetString(Attr::Product, rowIndex, "Banana");
    values->SetInt(Attr::Nr, rowIndex, 50);
    values->SetGuid(Attr::GUID, rowIndex, guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Pina Colada");
    values->SetString(Attr::Product, rowIndex, "Bicycle");
    values->SetInt(Attr::Nr, rowIndex, 2);
    values->SetGuid(Attr::GUID, rowIndex, guid);
    
    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Anton Guenther");
    values->SetString(Attr::Product, rowIndex, "BMW M6");
    values->SetInt(Attr::Nr, rowIndex, 5);
    values->SetGuid(Attr::GUID, rowIndex, guid);

    rowIndex = values->AddRow();
    guid.Generate();
    values->SetString(Attr::Name, rowIndex, "Franz Jaeger");
    values->SetString(Attr::Product, rowIndex, "Bicycle");
    values->SetInt(Attr::Nr, rowIndex, 1);
    values->SetGuid(Attr::GUID, rowIndex, guid);
}

//------------------------------------------------------------------------------
/**
*/
void
DatasetTest::PopulateCitiesTable(Dataset* t)
{
    Ptr<ValueTable> values = t->Values();

    IndexT rowIndex = values->AddRow();
    values->SetString(Attr::Name, rowIndex, "Berlin");
    values->SetString(Attr::Country, rowIndex, "Deutschland");
    rowIndex = values->AddRow();
    values->SetString(Attr::Name, rowIndex, "Muenchen");
    values->SetString(Attr::Country, rowIndex, "Deutschland");
    rowIndex = values->AddRow();
    values->SetString(Attr::Name, rowIndex, "Bermsgruen");
    values->SetString(Attr::Country, rowIndex, "Deutschland");
    rowIndex = values->AddRow();
    values->SetString(Attr::Name, rowIndex, "London");
    values->SetString(Attr::Country, rowIndex, "Grossbritanien");
    rowIndex = values->AddRow();
    values->SetString(Attr::Name, rowIndex, "Rio de Janeiro");
    values->SetString(Attr::Country, rowIndex, "Brasilien");
}

}