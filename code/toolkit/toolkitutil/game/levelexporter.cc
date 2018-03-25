//------------------------------------------------------------------------------
//  levelexporter.cc
//  (C) 2012-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/levelexporter.h"
#include "db/column.h"
#include "io/ioserver.h"
#include "io/xmlreader.h"
#include "db/sqlite3/sqlite3factory.h"
#include "db/sqlite3/sqlite3command.h"
#include "db/dataset.h"
#include "basegamefeature/basegameattr/basegameattributes.h"
#include "graphicsfeature/graphicsattr/graphicsattributes.h"

using namespace Db;
using namespace IO;
using namespace Util;
using namespace Attr;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::LevelExporter, 'LEXP', Base::ExporterBase);


void
SetValueTableEntry(const Ptr<ValueTable>& valueTable, IndexT newRow, Attr::AttrId id, const String & value);

//------------------------------------------------------------------------------
/**
*/
LevelExporter::LevelExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
LevelExporter::~LevelExporter()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::Open()
{
	ExporterBase::Open();
	this->gameDb = DbFactory::Instance()->CreateDatabase();
	this->gameDb->SetURI(URI("db:game.db4"));
	this->gameDb->SetAccessMode(Database::ReadWriteCreate);
	this->gameDb->SetIgnoreUnknownColumns(true);

	String s;
	s.Format("Could not open game database: %s", this->gameDb->GetURI().GetHostAndLocalPath().AsCharPtr());
	n_assert2(this->gameDb->Open(), s.AsCharPtr());

	this->staticDb = DbFactory::Instance()->CreateDatabase();
	this->staticDb->SetURI(URI("db:static.db4"));
	this->staticDb->SetAccessMode(Database::ReadWriteCreate);
	this->staticDb->SetIgnoreUnknownColumns(true);

	s.Format("Could not open static database: %s", this->staticDb->GetURI().GetHostAndLocalPath().AsCharPtr());
	n_assert2(this->staticDb->Open(), s.AsCharPtr());

	this->dbHasStartLevel = false;
	if (this->gameDb->HasTable("_Instance_Levels"))
	{
		Ptr<Sqlite3Command> command = Sqlite3Command::Create();
		String query = "DELETE FROM _Instance_Levels WHERE id='EmptyWorld'";
		command->CompileAndExecute(this->gameDb, query);
	}
	if (this->staticDb->HasTable("_Template_Levels"))
	{
		Ptr<Sqlite3Command> command = Sqlite3Command::Create();
		String query = "DELETE FROM _Template_Levels WHERE id='EmptyWorld'";
		command->CompileAndExecute(this->staticDb, query);
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::Close()
{

	ExporterBase::Close();
	// when levels are exported, make sure there is the leveleditor/contentbrowser startup level
	// for leveleditor2	

	// in case nothing was saved, just ignore this
	Util::String start = dbHasStartLevel ? "0" : "1";
	if(this->gameDb->IsOpen())
	{				
		if (this->gameDb->HasTable("_Instance_Levels"))
		{
			Ptr<Table> table = this->gameDb->GetTableByName("_Instance_Levels");
			Ptr<Db::Dataset> dataset = table->CreateDataset();
			dataset->AddAllTableColumns();
			dataset->PerformQuery();
			Ptr<ValueTable> valueTable = dataset->Values();

			IndexT row = valueTable->AddRow();

			valueTable->SetString(Attr::Id, row, "EmptyWorld");
			valueTable->SetString(Attr::Name, row, "EmptyWorld");
			valueTable->SetBool(Attr::StartLevel, row, false);
			valueTable->SetString(Attr::PostEffectPreset, row, "Default");
			valueTable->SetMatrix44(Attr::GlobalLightTransform, row, Math::matrix44());
			dataset->CommitChanges();
			table->CommitChanges();
			table = 0;
			
		}
		this->gameDb->Close();
	}

	if(this->staticDb->IsOpen())
	{
		if (this->staticDb->HasTable("_Template_Levels"))
		{
			Ptr<Sqlite3Command> command = Sqlite3Command::Create();
			String query = "INSERT INTO _Template_Levels (Id, Name, StartLevel) VALUES ('EmptyWorld', 'EmptyWorld', "+start+")";
			command->CompileAndExecute(this->staticDb,query);
		}		
		this->staticDb->Close();
	}
	
	this->dbFactory = 0;
	this->gameDb = 0;	
	this->staticDb = 0;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Table> 
LevelExporter::CreateTable( const Ptr<Database>& db, const String& tableName )
{
	Ptr<Table> table = 0;
	if(db->HasTable(tableName))
	{
		table = db->GetTableByName(tableName);
	}
	else
	{
		table = DbFactory::Instance()->CreateTable();
		table->SetName(tableName);
	}
	return table;
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::CreateColumn( const Ptr<Table>& table, Column::Type type, AttrId attributeId )
{
	if(false == table->HasColumn(attributeId))
	{
		Column column;
		column.SetType(type);
		column.SetAttrId(attributeId);
		table->AddColumn(column);
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::ExportAll()
{
	//if all are going to be exported, we want to delete all the entrys in the Levels tables. (resets the rowcounter)
	if (this->gameDb->HasTable("_Instance_Levels"))
	{
		Ptr<Sqlite3Command> command = Sqlite3Command::Create();
		String query = "DELETE FROM _Instance_Levels";
		command->CompileAndExecute(this->gameDb, query);
	}
	if (this->staticDb->HasTable("_Template_Levels"))
	{
		Ptr<Sqlite3Command> command = Sqlite3Command::Create();
		String query = "DELETE FROM _Template_Levels";
		command->CompileAndExecute(this->staticDb, query);
	}

	String levelDir = "proj:work/levels";
	Array<String> files = IoServer::Instance()->ListFiles(IO::URI(levelDir), "*.xml");
	for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
	{
		this->ExportFile(levelDir + "/" + files[fileIndex]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::ExportDir( const String& category )
{
	/// fall-through, we want to overload base class but we don't care about category
	this->ExportAll();
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::ExportFile( const URI& file )
{
	n_assert(this->isOpen);
   
	Ptr<Stream> levelStream = IoServer::Instance()->CreateStream(file);

	Ptr<XmlReader> xmlReader = XmlReader::Create();
	Ptr<Sqlite3Command> command;
    
	if(!levelStream->Open())
    {        
        this->logger->Error("Could not open level: %s\n",file.GetHostAndLocalPath().AsCharPtr());
        this->SetHasErrors(true);
        return;
    }
    if(!levelStream->GetSize())
    {
		this->logger->Error("Empty level file: %s\n", file.GetHostAndLocalPath().AsCharPtr());
        this->SetHasErrors(true);
        return;
    }
	xmlReader->SetStream(levelStream);
	if (!xmlReader->Open())
	{
		this->logger->Error("Could not open level: %s\n", file.GetHostAndLocalPath().AsCharPtr());
		this->SetHasErrors(true);
		return;
	}
    
	if (xmlReader->HasNode("/Level") && xmlReader->HasNode("/Level/NebulaLevel"))
	{
		this->Progress(5, file.AsString());
		this->logger->Print("Exporting: %s\n", file.GetHostAndLocalPath().ExtractFileName().AsCharPtr());
		xmlReader->SetToFirstChild();

		String levelName = xmlReader->GetString("name");
		String levelId = xmlReader->GetString("id");		
		if(!xmlReader->HasAttr("Version") || xmlReader->GetInt("Version") != 2)
		{
			this->logger->Warning("Level File %s is an old version, please save it once using the leveleditor\n", file.AsString().AsCharPtr());			
			return;
		}
		
		command = Sqlite3Command::Create();
		String table;
		String query;

		for (int tableIndex = 0; tableIndex < this->gameDb->GetNumTables(); tableIndex++)
		{
			table = this->gameDb->GetTableByIndex(tableIndex)->GetName();
			query = "DELETE FROM " + table + " WHERE _Level='" + levelId + "'";
			command->CompileAndExecute(this->gameDb, query);
		}

		if(xmlReader->SetToFirstChild("Entities"))
		{
			this->ExportLevel(xmlReader, this->gameDb);
			xmlReader->SetToParent();
		}
		
		
		//delete old
		{
			command = Sqlite3Command::Create();
			String query = "DELETE FROM _Instance_Levels WHERE id='" + levelId + "'";
			command->CompileAndExecute(this->gameDb, query);
		}		
		
		if (this->staticDb->HasTable("_Template_Levels"))
		{
			//delete old
			command = Sqlite3Command::Create();
			String query = "DELETE FROM _Template_Levels WHERE id='" + levelId + "'";
			command->CompileAndExecute(this->staticDb, query);

			command = Sqlite3Command::Create();
			query = "INSERT INTO _Template_Levels (Id, Name) VALUES ('" + levelId + "', '" + levelName + "')";
			command->CompileAndExecute(this->staticDb, query);
		}

		// add global stuff if exists
		if(xmlReader->HasNode("/Level/NebulaLevel/Global"))//SetToNextChild("Global"))
		{
			xmlReader->SetToNode("/Level/NebulaLevel/Global");
			Ptr<Table> table;
			Ptr<Dataset> dataset;
			Ptr<ValueTable> valueTable;

			table = this->gameDb->GetTableByName("_Instance_Levels");
			dataset = table->CreateDataset();
			dataset->AddAllTableColumns();
			dataset->PerformQuery();
			valueTable = dataset->Values();

			IndexT row = valueTable->AddRow();
			
			valueTable->SetString(Attr::Id, row, levelId);
			valueTable->SetString(Attr::Name, row, levelName);							

			if(xmlReader->HasAttr("WorldCenter"))
			{
				AttrId id = AttrId("WorldCenter");
				valueTable->SetFloat4(id,row,xmlReader->GetFloat4("WorldCenter"));
			}
			if(xmlReader->HasAttr("WorldExtents"))
			{
				AttrId id = AttrId("WorldExtents");
				valueTable->SetFloat4(id,row,xmlReader->GetFloat4("WorldExtents"));
			}
			if (xmlReader->HasAttr("PostEffectPreset"))
			{
				AttrId id = AttrId("PostEffectPreset");
				valueTable->SetString(id, row, xmlReader->GetString("PostEffectPreset"));
			}
			else
			{
				AttrId id = AttrId("PostEffectPreset");
				valueTable->SetString(id, row, "Default");
			}
			if (xmlReader->HasAttr("GlobalLightTransform"))
			{
				AttrId id = AttrId("GlobalLightTransform");				
				Util::String trans = xmlReader->GetString("GlobalLightTransform");
				// avoid crashing when changing level format
				if (!valueTable->HasColumn(id))
				{
					valueTable->AddColumn(id);
				}
				valueTable->SetMatrix44(id, row, trans.AsMatrix44());
			}
			else
			{
				AttrId id = AttrId("GlobalLightTransform");				
				Math::matrix44 ident;
				valueTable->SetMatrix44(id, row, ident);
			}
			if (xmlReader->HasAttr("_Layers"))
			{
				valueTable->SetString(AttrId("_Layers"), row, xmlReader->GetString("_Layers"));
			}
			dataset->CommitChanges();
			table->CommitChanges();									
			table = 0;
			
		}
		else
		{
			command = Sqlite3Command::Create();
			query = "INSERT INTO _Instance_Levels (Id, Name) VALUES ('" + levelId + "', '" + levelName + "')";
			command->CompileAndExecute(this->gameDb, query);
		}
	}
	else
	{
		n_printf("Level: %s is either corrupt or not a level", file.GetHostAndLocalPath().AsCharPtr());
	}

	// cleanup readers
	levelStream->Close();
	xmlReader->Close();


}

//------------------------------------------------------------------------------
/**
	Helper function for setting column values in table
*/
void
SetValueTableEntry(const Ptr<ValueTable>& valueTable, IndexT newRow, Attr::AttrId id, const String & val)
{
	switch (id.GetValueType())
	{
	case Attr::StringType:
		valueTable->SetString(id, newRow, val);
		break;
	case Attr::IntType:
		valueTable->SetInt(id, newRow, val.AsInt());
		break;
	case Attr::BoolType:
		valueTable->SetBool(id, newRow, val.AsBool());
		break;
	case Attr::FloatType:
		valueTable->SetFloat(id, newRow, val.AsFloat());
		break;
	case Attr::Matrix44Type:
		valueTable->SetMatrix44(id, newRow, val.AsMatrix44());
		break;
	case Attr::Float4Type:
		valueTable->SetFloat4(id, newRow, val.AsFloat4());
		break;
	case Attr::GuidType:
		valueTable->SetGuid(id, newRow, Guid::FromString(val));
		break;
	}

}


//------------------------------------------------------------------------------
/**
*/
Util::Dictionary<Util::String, Util::String> 
LevelExporter::LoadObjectAttributes(const Ptr<IO::XmlReader> & reader)
{
	Util::Dictionary<Util::String, Util::String> vals;

	if(reader->SetToFirstChild("Attributes"))
	{
		if(reader->SetToFirstChild())
		{
			do
			{
				String val;
				if(reader->HasContent())
				{
					val = reader->GetContent();
				}
				vals.Add(reader->GetCurrentNodeName(),val);
			}
			while(reader->SetToNextChild());	
			reader->SetToParent();
		}		
	}	
	return vals;
}

//------------------------------------------------------------------------------
/**
*/
bool 
LevelExporter::ExportLevel( const Ptr<IO::XmlReader>& reader, const Ptr<Database>& db)
{
	if (reader->SetToFirstChild("Object"))
	{
		bool remainingObject;
		do 
		{
			remainingObject = false;
			Ptr<Table> table;
			Ptr<Dataset> dataset;
			Ptr<ValueTable> valueTable;

			String category = reader->GetString("category");
			if (category == "_Group")
			{
				if (reader->SetToNextChild("Object"))
				{
					remainingObject = true;
				}
				continue;
			}
			Dictionary<String,String> attributesDict = LoadObjectAttributes(reader);
			Array<String> attributes = attributesDict.KeysAsArray();

			String tableName = "_Instance_" + category;
			
			
			table = db->GetTableByName(tableName);
			for (int attrIndex = 0; attrIndex < attributes.Size(); attrIndex++)
			{
				if (AttrId::IsValidName(attributes[attrIndex]))
				{
					AttrId id = AttrId(attributes[attrIndex]);
					if (!table->HasColumn(id))
					{
						this->CreateColumn(table, Column::Default, id);
					}
				}
			}

			dataset = table->CreateDataset();
			dataset->AddAllTableColumns();
			dataset->PerformQuery();

			valueTable = dataset->Values();
		
			bool repeat;
			do
			{
				repeat = false;
				IndexT newRow = valueTable->AddRow();
				for (int attrIndex = 0; attrIndex < attributes.Size(); attrIndex++)
				{
					if (AttrId::IsValidName(attributes[attrIndex]))
					{
						AttrId id = AttrId(attributes[attrIndex]);
						SetValueTableEntry(valueTable, newRow, id, attributesDict[attributes[attrIndex]]);

					}
				}
				if (valueTable->HasColumn(Attr::_LevelEntity))				
				{
					valueTable->SetBool(Attr::_LevelEntity, newRow, true);
				}
				
				//reader->SetToParent();
				if(reader->SetToNextChild("Object"))
				{
					if(category == reader->GetString("category"))
					{
						repeat = true;
						attributesDict = LoadObjectAttributes(reader);
						attributes = attributesDict.KeysAsArray();
					}
					else
					{
						remainingObject = true;
					}					
				}
				
			}while(repeat);

			dataset->CommitChanges();

			table = 0;
			dataset = 0;
			valueTable = 0;
		}while (remainingObject);		
	}
	return true;
}

} // namespace ToolkitUtil