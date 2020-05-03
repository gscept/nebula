//------------------------------------------------------------------------------
//  databaseinsert.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "benchmarkaddon/databaseinsert.h"
#include "addons/db/sqlite3/sqlite3factory.h"
#include "io/ioserver.h"
#include "benchmarkaddon/dbattrs.h"
#include "addons/db/dataset.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::DatabaseInsert, 'DBIS', Benchmarking::Benchmark);

using namespace IO;
using namespace Db;
using namespace Util;
using namespace Timing;

//------------------------------------------------------------------------------
/**
*/
void
DatabaseInsert::PopulateStringTable()
{
    this->stringTable.SetSize(32);
    this->stringTable[0] = "Radon";
    this->stringTable[1] = "Labs";
    this->stringTable[2] = "Game";
    this->stringTable[3] = "Development";
    this->stringTable[4] = "Studio";
    this->stringTable[5] = "located";
    this->stringTable[6] = "Berlin";
    this->stringTable[7] = "Germany";
    this->stringTable[8] = "Association";
    this->stringTable[9] = "developers";
    this->stringTable[10] = "released";
    this->stringTable[11] = "technology";
    this->stringTable[12] = "public";
    this->stringTable[13] = "commercial";
    this->stringTable[14] = "research";
    this->stringTable[15] = "visualisation";
    this->stringTable[16] = "project";
    this->stringTable[17] = "community";
    this->stringTable[18] = "seamlessly";
    this->stringTable[19] = "integrates";
    this->stringTable[20] = "Animation";
    this->stringTable[21] = "Blendshapes";
    this->stringTable[22] = "Dynamic";
    this->stringTable[23] = "Realtime";
    this->stringTable[24] = "Particle System";
    this->stringTable[25] = "Hierarchical Animation";
    this->stringTable[26] = "DirectX 9 Shader";
    this->stringTable[27] = "One-Click-Lightmapping";
    this->stringTable[28] = "turnaround";
    this->stringTable[29] = "extremely";
    this->stringTable[30] = "visualisation";
    this->stringTable[31] = "growing";
}

//------------------------------------------------------------------------------
/**
*/
void
DatabaseInsert::Run(Timer& timer)
{
    Ptr<Db::DbFactory> dbFactory = Db::Sqlite3Factory::Create();
    const URI uri("temp:dbinsertbenchmark.db3");
    this->PopulateStringTable();

    timer.Start();
    // prepare the database (open, create tables)
    Ptr<Database> db = this->OpenDatabase(uri);

    // create database table
    Ptr<Table> table = this->CreateTable(db);

    // populate the database with random data
    this->PopulateDatabase(db, table);

    // finally close it, this will commit the data back into the database
    this->CloseDatabase(db);
    timer.Stop();
}

//------------------------------------------------------------------------------
/**
*/
void
DatabaseInsert::CloseDatabase(Database* db)
{
    n_assert(0 != db);
    Timer timer;
    timer.Start();
    db->Close();
    timer.Stop();
    n_printf("**** DatabaseInsert::CloseDatabase(): %d ticks, %f seconds\n", timer.GetTicks(), timer.GetTime());
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Database>
DatabaseInsert::OpenDatabase(const URI& uri)
{
    Timer timer;
    timer.Start();

    // delete existing database
    if (IO::IoServer::Instance()->FileExists(uri))
    {
        IO::IoServer::Instance()->DeleteFile(uri);
        n_assert(!IO::IoServer::Instance()->FileExists(uri));
    }

    // create database
    Ptr<Database> db = Db::DbFactory::Instance()->CreateDatabase();
    db->SetURI(uri);
    db->SetAccessMode(Database::ReadWriteCreate);
    db->Open();

    timer.Stop();
    n_printf("**** DatabaseInsert::OpenDatabase(): %d ticks, %f seconds\n", timer.GetTicks(), timer.GetTime());
    return db;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Table>
DatabaseInsert::CreateTable(Database* db)
{
    n_assert(0 != db);
    Timer timer;
    timer.Start();

    // create database main table
    Ptr<Table> t = Db::DbFactory::Instance()->CreateTable();
    t->SetName("MainTable");
    t->AddColumn(Column(Attr::Guid, Column::Primary));
    t->AddColumn(Column(Attr::Level, Column::Indexed));
    t->AddColumn(Column(Attr::Id, Column::Indexed));
    t->AddColumn(Column(Attr::Graphics));
    t->AddColumn(Column(Attr::Rot180));
    t->AddColumn(Column(Attr::CharacterSet));
    t->AddColumn(Column(Attr::AnimSet));
    t->AddColumn(Column(Attr::SoundSet));
    t->AddColumn(Column(Attr::Placeholder));
    t->AddColumn(Column(Attr::Name));
    t->AddColumn(Column(Attr::EntityGroup));
    t->AddColumn(Column(Attr::Faction));
    t->AddColumn(Column(Attr::Behaviour));
    t->AddColumn(Column(Attr::MaxVelocity));
    t->AddColumn(Column(Attr::LookatText));
    t->AddColumn(Column(Attr::NewHero));
    t->AddColumn(Column(Attr::AggroRadius));
    t->AddColumn(Column(Attr::SetupStorage));
    t->AddColumn(Column(Attr::SetupEquipment));
    t->AddColumn(Column(Attr::InventoryType));
    t->AddColumn(Column(Attr::MU));
    t->AddColumn(Column(Attr::KL));
    t->AddColumn(Column(Attr::IN));
    t->AddColumn(Column(Attr::CH));
    t->AddColumn(Column(Attr::FF));
    t->AddColumn(Column(Attr::GE));
    t->AddColumn(Column(Attr::KO));
    t->AddColumn(Column(Attr::KK));

    t->AddColumn(Column(Attr::TaArmbrust));
    t->AddColumn(Column(Attr::TaATArmbrust));
    t->AddColumn(Column(Attr::TaPAArmbrust));
    t->AddColumn(Column(Attr::TaDolche));
    t->AddColumn(Column(Attr::TaATDolche));
    t->AddColumn(Column(Attr::TaPADolche));
    t->AddColumn(Column(Attr::TaFechtwaffen));
    t->AddColumn(Column(Attr::TaATFechtwaffen));
    t->AddColumn(Column(Attr::TaPAFechtwaffen));
    t->AddColumn(Column(Attr::TaHiebwaffen));
    t->AddColumn(Column(Attr::TaATHiebwaffen));
    t->AddColumn(Column(Attr::TaPAHiebwaffen));
    t->AddColumn(Column(Attr::TaInfanteriewaffen));
    t->AddColumn(Column(Attr::TaATInfanteriewaffen));
    t->AddColumn(Column(Attr::TaPAInfanteriewaffen));
    t->AddColumn(Column(Attr::TaKettenwaffen));
    t->AddColumn(Column(Attr::TaATKettenwaffen));
    t->AddColumn(Column(Attr::TaPAKettenwaffen));
    t->AddColumn(Column(Attr::TaSaebel));
    t->AddColumn(Column(Attr::TaATSaebel));
    t->AddColumn(Column(Attr::TaPASaebel));
    t->AddColumn(Column(Attr::TaSchwerter));
    t->AddColumn(Column(Attr::TaATSchwerter));
    t->AddColumn(Column(Attr::TaPASchwerter));
    t->AddColumn(Column(Attr::TaSpeere));
    t->AddColumn(Column(Attr::TaATSpeere));
    t->AddColumn(Column(Attr::TaPASpeere));
    t->AddColumn(Column(Attr::TaStaebe));
    t->AddColumn(Column(Attr::TaATStaebe));
    t->AddColumn(Column(Attr::TaPAStaebe));
    t->AddColumn(Column(Attr::TaZwHiebwaffen));
    t->AddColumn(Column(Attr::TaATZwHiebwaffen));
    t->AddColumn(Column(Attr::TaPAZwHiebwaffen));
    t->AddColumn(Column(Attr::TaZwSchwerter));
    t->AddColumn(Column(Attr::TaATZwSchwerter));
    t->AddColumn(Column(Attr::TaPAZwSchwerter));
    t->AddColumn(Column(Attr::TaRaufen));
    t->AddColumn(Column(Attr::TaATRaufen));
    t->AddColumn(Column(Attr::TaPARaufen));
    t->AddColumn(Column(Attr::TaBogen));
    t->AddColumn(Column(Attr::TaWurfbeile));
    t->AddColumn(Column(Attr::TaWurfmesser));
    t->AddColumn(Column(Attr::TaWurfspeer));

    t->AddColumn(Column(Attr::TaGaukeleien));
    t->AddColumn(Column(Attr::TaKoerperbeherrschung));
    t->AddColumn(Column(Attr::TaSchleichen));
    t->AddColumn(Column(Attr::TaSelbstbeherrschung));
    t->AddColumn(Column(Attr::TaSinnenschaerfe));
    t->AddColumn(Column(Attr::TaTanzen));
    t->AddColumn(Column(Attr::TaTaschendiebstahl));
    t->AddColumn(Column(Attr::TaZechen));
    t->AddColumn(Column(Attr::TaFallenstellen));
    t->AddColumn(Column(Attr::TaFischen));
    t->AddColumn(Column(Attr::TaOrientierung));
    t->AddColumn(Column(Attr::TaWildnisleben));
    t->AddColumn(Column(Attr::TaMagiekunde));
    t->AddColumn(Column(Attr::TaPflanzenkunde));
    t->AddColumn(Column(Attr::TaTierkunde));
    t->AddColumn(Column(Attr::TaSchaetzen));
    t->AddColumn(Column(Attr::TaSprachkunde));
    t->AddColumn(Column(Attr::TaAlchimie));
    t->AddColumn(Column(Attr::TaBogenbau));
    t->AddColumn(Column(Attr::TaGrobschmied));
    t->AddColumn(Column(Attr::TaHeilkundeWunden));
    t->AddColumn(Column(Attr::TaSchloesser));
    t->AddColumn(Column(Attr::TaMusizieren));
    t->AddColumn(Column(Attr::TaFalschspiel));
    t->AddColumn(Column(Attr::TaLederarbeiten));
    t->AddColumn(Column(Attr::TaBetoeren));
    t->AddColumn(Column(Attr::TaUeberreden));
    t->AddColumn(Column(Attr::TaMenschenkenntnis));
    
    t->AddColumn(Column(Attr::ZaAdlerauge));
    t->AddColumn(Column(Attr::ZaAnalys));
    t->AddColumn(Column(Attr::ZaExposami));
    t->AddColumn(Column(Attr::ZaOdemArcanum));
    t->AddColumn(Column(Attr::ZaDuplicatus));
    t->AddColumn(Column(Attr::ZaBlitz));
    t->AddColumn(Column(Attr::ZaFulminictus));
    t->AddColumn(Column(Attr::ZaIgnifaxius));
    t->AddColumn(Column(Attr::ZaIgnisphaero));
    t->AddColumn(Column(Attr::ZaKulminatio));
    t->AddColumn(Column(Attr::ZaPlumbumbaraum));
    t->AddColumn(Column(Attr::ZaAttributo));
    t->AddColumn(Column(Attr::ZaBeherrschungBrechen));
    t->AddColumn(Column(Attr::ZaEinflussBannen));
    t->AddColumn(Column(Attr::ZaVerwandlungBeenden));
    t->AddColumn(Column(Attr::ZaGardianum));
    t->AddColumn(Column(Attr::ZaTempusStatis));
    t->AddColumn(Column(Attr::ZaBannbaladin));
    t->AddColumn(Column(Attr::ZaHalluzination));
    t->AddColumn(Column(Attr::ZaHorriphobus));
    t->AddColumn(Column(Attr::ZaSanftmut));
    t->AddColumn(Column(Attr::ZaSomnigravis));
    t->AddColumn(Column(Attr::ZaElementarerDiener));
    t->AddColumn(Column(Attr::ZaTatzeSchwinge));
    t->AddColumn(Column(Attr::ZaAxxeleratus));
    t->AddColumn(Column(Attr::ZaForamen));
    t->AddColumn(Column(Attr::ZaTransversalis));
    t->AddColumn(Column(Attr::ZaMotoricus));
    t->AddColumn(Column(Attr::ZaBalsam));
    t->AddColumn(Column(Attr::ZaRuheKoerper));
    t->AddColumn(Column(Attr::ZaAdlerschwinge));
    t->AddColumn(Column(Attr::ZaArmatrutz));
    t->AddColumn(Column(Attr::ZaParalysis));
    t->AddColumn(Column(Attr::ZaUnsichtbarerJaeger));
    t->AddColumn(Column(Attr::ZaVisibility));
    t->AddColumn(Column(Attr::ZaStandfestKatzengleich));
    t->AddColumn(Column(Attr::ZaApplicatus));
    t->AddColumn(Column(Attr::ZaClaudibus));
    t->AddColumn(Column(Attr::ZaFlimFlam));
    t->AddColumn(Column(Attr::ZaFortifex));
    t->AddColumn(Column(Attr::ZaSilentium));
    t->AddColumn(Column(Attr::ZaEisenrost));
    
    t->AddColumn(Column(Attr::LEmax));
    t->AddColumn(Column(Attr::AUmax));
    t->AddColumn(Column(Attr::AEmax));
    t->AddColumn(Column(Attr::ATbasis));
    t->AddColumn(Column(Attr::PAbasis));
    t->AddColumn(Column(Attr::FKbasis));
    t->AddColumn(Column(Attr::MR));
    t->AddColumn(Column(Attr::LE));
    t->AddColumn(Column(Attr::AU));
    t->AddColumn(Column(Attr::AE));
    t->AddColumn(Column(Attr::AT));
    t->AddColumn(Column(Attr::PA));
    t->AddColumn(Column(Attr::BE));
    
    t->AddColumn(Column(Attr::SetupGroups));
    t->AddColumn(Column(Attr::TargetSize));
    t->AddColumn(Column(Attr::MapMarkerResource));
    t->AddColumn(Column(Attr::VisibilityType));
    t->AddColumn(Column(Attr::ScriptPreset));
    t->AddColumn(Column(Attr::ScriptOverride));

    // attach table to db
    db->AddTable(t);

    // create a combined index on Level and Id
    Array<Attr::AttrId> indexColumns;
    indexColumns.Append(Attr::Level);
    indexColumns.Append(Attr::Id);
    t->CreateMultiColumnIndex(indexColumns);

    timer.Stop();
    n_printf("**** DatabaseInsert::CreateTable(): %d ticks, %f seconds\n", timer.GetTicks(), timer.GetTime());
    return t;
}

//------------------------------------------------------------------------------
/**    
*/
int
DatabaseInsert::GetRandomInt() const
{
    return rand() % 1024;
}

//------------------------------------------------------------------------------
/**
*/
float
DatabaseInsert::GetRandomFloat() const
{
    return rand() * 100.0f;
}

//------------------------------------------------------------------------------
/**
*/
const String&
DatabaseInsert::GetRandomString() const
{
    int i = rand() % this->stringTable.Size();
    return stringTable[i];
}

//------------------------------------------------------------------------------
/**
*/
void
DatabaseInsert::WriteRandomValue(ValueTable* values, IndexT colIndex, IndexT rowIndex)
{
    switch (values->GetColumnValueType(colIndex))
    {
        case Attr::IntType:
            values->SetInt(colIndex, rowIndex, this->GetRandomInt());
            break;

        case Attr::FloatType:
            values->SetFloat(colIndex, rowIndex, this->GetRandomFloat());
            break;

        case Attr::BoolType:
            values->SetBool(colIndex, rowIndex, true);
            break;

        case Attr::StringType:
            values->SetString(colIndex, rowIndex, this->GetRandomString());
            break;

        case Attr::GuidType:
            {
                Guid guid;
                guid.Generate();    // NOTE: this is quite expensive and pollutes the result
                values->SetGuid(colIndex, rowIndex, guid);
            }
            break;

        default:
            // unhandled datatype
            n_error("DatabaseInsert::PopulateDatabase(): data type not handled!");
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DatabaseInsert::PopulateDatabase(Database* db, Table* table)
{
    n_assert(0 != db);

    const SizeT numInserts = 20000;
    const SizeT numUpdates = 1000;
    const SizeT numUpdatesPerRow = 20;

    Timer timer;
    timer.Start();

    // create a dataset object
    Ptr<Dataset> dataSet = table->CreateDataset();
    dataSet->AddAllTableColumns();
    ValueTable* values = dataSet->Values();
    values->ReserveRows(numInserts);

    // adding a few rows
    IndexT i;
    for (i = 0; i < numInserts; i++)
    {
        IndexT rowIndex = values->AddRow();

        IndexT colIndex;
        SizeT numColumns = values->GetNumColumns();
        for (colIndex = 0; colIndex < numColumns; colIndex++)
        {
            this->WriteRandomValue(values, colIndex, rowIndex);
        }
    }
    timer.Stop();
    n_printf("**** DatabaseInsert (%d rows x %d columns):: populate value table: %d ticks, %f seconds\n", 
        values->GetNumRows(), 
        values->GetNumColumns(), 
        timer.GetTicks(), 
        timer.GetTime());

    // commit the changes (should do a lot of inserts)
    timer.Reset();
    timer.Start();
    dataSet->CommitChanges();
    timer.Stop();
    n_printf("**** DatabaseInsert (%d rows x %d columns): write value table to db: %d ticks, %f seconds\n",
        values->GetNumRows(), 
        values->GetNumColumns(), 
        timer.GetTicks(), 
        timer.GetTime());

    // the next CommitChanges should do nothing
    timer.Reset();
    timer.Start();
    dataSet->CommitChanges();
    timer.Stop();
    n_printf("**** DatabaseInsert: empty CommitChanges(): %d ticks, %f seconds\n", timer.GetTicks(), timer.GetTime());

    // run some updates on the dataset
    for (i = 0; i < numUpdates; i++)
    {
        IndexT rowIndex = rand() % values->GetNumRows();
        IndexT j;
        for (j = 0; j < 20; j++)
        {
            IndexT colIndex = rand() % values->GetNumColumns();
            
            // don't overwrite the primary key...
            if (values->GetColumnId(colIndex) != Attr::Guid)
            {
                this->WriteRandomValue(values, colIndex, rowIndex);
            }
        }
    }
    timer.Reset();
    timer.Start();
    dataSet->CommitChanges();
    timer.Stop();
    n_printf("**** DatabaseInsert: update (%d rows): %d ticks, %f seconds\n", 
        numUpdates,
        timer.GetTicks(), 
        timer.GetTime());

    // close the data set
    timer.Reset();
    timer.Start();
    dataSet->CommitChanges();
    timer.Stop();
    n_printf("**** DatabaseInsert: close data set: %d ticks, %f seconds\n", timer.GetTicks(), timer.GetTime());
}

} // namespace Benchmarking

