//------------------------------------------------------------------------------
//  templateexporter.cc
//  (C) 2012-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/templateexporter.h"
#include "attr/attributedefinitionbase.h"
#include "db/dbfactory.h"
#include "attr/attrid.h"
#include "db/sqlite3/sqlite3factory.h"
#include "io/ioserver.h"
#include "db/sqlite3/sqlite3command.h"
#include "io/xmlreader.h"
#include "io/stream.h"
#include "db/valuetable.h"
#include "db/dataset.h"
#include "db/table.h"
#include "editorfeatures/editorblueprintmanager.h"
#include "idldocument/idlattribute.h"

using namespace Db;
using namespace IO;
using namespace Attr;
using namespace Util;
using namespace Tools;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::TemplateExporter, 'TEEX', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
TemplateExporter::TemplateExporter() : 
dbFactory(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TemplateExporter::~TemplateExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
TemplateExporter::Open()
{
	if(!Toolkit::EditorBlueprintManager::HasInstance())
	{
		this->blueprintManager = Toolkit::EditorBlueprintManager::Create();
		blueprintManager->SetLogger(this->logger);
		blueprintManager->Open();		
	}
	else
	{
		this->blueprintManager = Toolkit::EditorBlueprintManager::Instance();
	}
		
	// create new database files
	ExporterBase::Open();
	n_assert(this->dbFactory);
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
	
	// collect all attributes written in nidl files and add them to the attributes tables
	this->CollectNIDLAttributes();

	// create global categories table object
	this->categoryTable = TemplateExporter::CreateTable(this->staticDb, "_Categories");
	TemplateExporter::CreateColumn(this->categoryTable, Column::Primary, Attr::CategoryName);
	TemplateExporter::CreateColumn(this->categoryTable, Column::Default, Attr::IsVirtualCategory);
	TemplateExporter::CreateColumn(this->categoryTable, Column::Default, Attr::IsSpecialCategory);
	TemplateExporter::CreateColumn(this->categoryTable, Column::Default, Attr::CategoryTemplateTable);
	TemplateExporter::CreateColumn(this->categoryTable, Column::Default, Attr::CategoryInstanceTable);
	this->staticDb->AddTable(this->categoryTable);
	
	this->categoryDataset = this->categoryTable->CreateDataset();
	this->categoryDataset->AddAllTableColumns();
	this->categoryValues = this->categoryDataset->Values();

	// create some default entries
	this->AddCategory("Light", false, true, "", "_Instance_Light");
	this->AddCategory("_Environment", false, true, "", "_Instance__Environment");
	this->AddCategory("Levels", true, true, "_Template_Levels", "");
		
	if(!AttrId::IsValidName("Guid"))
	{
		AttributeDefinitionBase::RegisterDynamicAttribute("Guid", Util::FourCC(), StringType, ReadOnly);
	}
	if(!AttrId::IsValidName("_Level"))
	{
		AttributeDefinitionBase::RegisterDynamicAttribute("_Level", Util::FourCC(), StringType, ReadOnly);
	}
	if(!AttrId::IsValidName("_Layers"))
	{
		AttributeDefinitionBase::RegisterDynamicAttribute("_Layers", Util::FourCC(), StringType, ReadOnly);
	}
	if(!AttrId::IsValidName("Id"))
	{
		AttributeDefinitionBase::RegisterDynamicAttribute("Id", Util::FourCC(), StringType, ReadOnly);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
TemplateExporter::Close()
{
	this->WriteBlueprintTables();

	this->CollectAttributes(this->gameDb);
	this->CollectAttributes(this->staticDb);

	this->categoryDataset->CommitChanges();
	this->categoryTable->CommitChanges();
	this->categoryTable = 0;

	this->dbFactory = 0;
	this->staticDb->Close();
	this->staticDb = 0;
	this->gameDb->Close();
	this->gameDb = 0;
	this->blueprintManager = 0;

	ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**    
*/
void 
TemplateExporter::ExportAll()
{
    // first convert toolkit templates
    String templateDir = "toolkit:data/tables/db";
    
    Array<String> files = IoServer::Instance()->ListFiles(templateDir, "*.xml");
    for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        this->ExportFile(templateDir + "/" + files[fileIndex]);
    }

    // now the project specific ones
	templateDir = "proj:data/tables/db";
	files = IoServer::Instance()->ListFiles(templateDir, "*.xml");
	for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
	{
		this->ExportFile(templateDir + "/" + files[fileIndex]);
	}
	this->ExportUiProperties();
}

//------------------------------------------------------------------------------
/**
*/
void 
TemplateExporter::ExportDir( const Util::String& category )
{
	/// fall-through, we want to overload base class but we don't care about category
	this->ExportAll();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Table> 
TemplateExporter::CreateTable( const Ptr<Database>& db, const Util::String& tableName )
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
TemplateExporter::CreateColumn( const Ptr<Table>& table, Column::Type type, AttrId attributeId )
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
TemplateExporter::AddCategory(const Util::String & name, bool isVirtual, bool isSystem, const Util::String& templateName, const Util::String& instanceName)
{
	IndexT newRow = this->categoryValues->AddRow();
	this->categoryValues->SetString(Attr::CategoryName, newRow, name);
	this->categoryValues->SetBool(Attr::IsVirtualCategory, newRow, isVirtual);
	this->categoryValues->SetBool(Attr::IsSpecialCategory, newRow, isSystem);
	this->categoryValues->SetString(Attr::CategoryTemplateTable, newRow, templateName);
	this->categoryValues->SetString(Attr::CategoryInstanceTable, newRow, instanceName);
}

//------------------------------------------------------------------------------
/**
*/
void 
TemplateExporter::CollectNIDLAttributes()
{	
	Array<Ptr<IDLAttribute>> attrs = this->blueprintManager->GetAllAttributes();

	Ptr<Table> gameTable = this->CreateTable(this->gameDb, "_Attributes");
	Ptr<Table> staticTable = this->CreateTable(this->staticDb, "_Attributes");

	TemplateExporter::CreateColumn(gameTable, Column::Primary, Attr::AttrName);
	TemplateExporter::CreateColumn(gameTable, Column::Default, Attr::AttrType);
	TemplateExporter::CreateColumn(gameTable, Column::Default, Attr::AttrReadWrite);
	TemplateExporter::CreateColumn(gameTable, Column::Default, Attr::AttrDynamic);
	this->gameDb->AddTable(gameTable);
	TemplateExporter::CreateColumn(staticTable, Column::Primary, Attr::AttrName);
	TemplateExporter::CreateColumn(staticTable, Column::Default, Attr::AttrType);
	TemplateExporter::CreateColumn(staticTable, Column::Default, Attr::AttrReadWrite);
	TemplateExporter::CreateColumn(staticTable, Column::Default, Attr::AttrDynamic);
	this->staticDb->AddTable(staticTable);

	
	Ptr<Dataset> gameDataSet = gameTable->CreateDataset();
	gameDataSet->AddAllTableColumns();
	ValueTable * gameValueTable = gameDataSet->Values();

	Ptr<Dataset> staticDataSet = staticTable->CreateDataset();
	staticDataSet->AddAllTableColumns();
	ValueTable * staticValueTable = staticDataSet->Values();

	for(IndexT i = 0; i<attrs.Size();i++)
	{
		bool readOnly = attrs[i]->GetAccessMode() == "ReadOnly";
		
		IndexT newRow = gameValueTable->AddRow();		
		gameValueTable->SetString(Attr::AttrName, newRow, attrs[i]->GetName());
		gameValueTable->SetString(Attr::AttrType, newRow, attrs[i]->GetType());
		gameValueTable->SetBool(Attr::AttrReadWrite, newRow, readOnly);
		gameValueTable->SetBool(Attr::AttrDynamic, newRow, true);

		newRow = staticValueTable->AddRow();
		staticValueTable->SetString(Attr::AttrName, newRow, attrs[i]->GetName());
		staticValueTable->SetString(Attr::AttrType, newRow, attrs[i]->GetType());
		staticValueTable->SetBool(Attr::AttrReadWrite, newRow, readOnly);
		staticValueTable->SetBool(Attr::AttrDynamic, newRow, true);
	}

	gameDataSet->CommitChanges();
	staticDataSet->CommitChanges();

	gameTable->CommitChanges();
	staticTable->CommitChanges();
	
	gameTable = 0;
	staticTable = 0;

}


//------------------------------------------------------------------------------
/**
    will create all template tables and instance tables defined by blueprints
    in case they dont have a template db file
*/
void 
TemplateExporter::WriteBlueprintTables()
{
	Ptr<Db::Table> staticTable, gameTable;

    Ptr<Toolkit::EditorBlueprintManager> bm = Toolkit::EditorBlueprintManager::Instance();

	for(IndexT idx = 0 ; idx < bm->GetNumCategories();idx++)
    {
		// add category to global category table
		String category = bm->GetCategoryByIndex(idx);
		if (category != "Light")
		{
			this->AddCategory(category, false, false, "_Template_" + category, "_Instance_" + category);
		}

        // create new template table and add all attributes defined by nidl files to it
        if(!this->staticDb->HasTable("_Template_" + category))
        {
		    // in case the table wasnt created before make sure it contains a primary column
		    staticTable = TemplateExporter::CreateTable(this->staticDb, "_Template_" + category);
		    TemplateExporter::CreateColumn(staticTable, Column::Primary, Attr::AttrId("Id"));
        }
        else
        {
            staticTable = TemplateExporter::CreateTable(this->staticDb, "_Template_" + category);
        }

		// create instance table and add default required attributes and then all the defined attributes to it
        if(!this->gameDb->HasTable("_Instance_" + category))
        {
            gameTable = TemplateExporter::CreateTable(this->gameDb, "_Instance_" + category);
            TemplateExporter::CreateColumn(gameTable, Column::Primary, Attr::AttrId("_ID"));
            TemplateExporter::CreateColumn(gameTable, Column::Default, Attr::AttrId("_Level"));
            TemplateExporter::CreateColumn(gameTable, Column::Default, Attr::AttrId("_Layers"));
        }
        else
        {
            gameTable = TemplateExporter::CreateTable(this->gameDb, "_Instance_" + category);
        }

			
		if (Toolkit::EditorBlueprintManager::Instance()->HasCategory(category))
		{
			const Util::Array<AttrId>& attrs = Toolkit::EditorBlueprintManager::Instance()->GetCategoryAttributes(category).GetAttrs().KeysAsArray();

			for (int attrIndex = 0; attrIndex < attrs.Size(); attrIndex++)
			{
				TemplateExporter::CreateColumn(staticTable, Column::Default, attrs[attrIndex]);
				TemplateExporter::CreateColumn(gameTable, Column::Default, attrs[attrIndex]);
			}
		}			
		if (!this->staticDb->HasTable("_Template_" + category))
		{
			this->staticDb->AddTable(staticTable);
		}
		staticTable->CommitChanges();
								
		// make sure some essentials exist
		if (!gameTable->HasColumn("Id"))
		{
			TemplateExporter::CreateColumn(gameTable, Column::Default, Attr::AttrId("Id"));
		}

		if (!gameTable->HasColumn("Guid"))
		{
			this->CreateColumn(gameTable, Column::Default, Attr::AttrId("Guid"));
		}
		if (!gameTable->HasColumn("_LevelEntity"))
		{
			this->CreateColumn(gameTable, Column::Default, Attr::AttrId("_LevelEntity"));
		}			
        if(!this->gameDb->HasTable(gameTable->GetName()))
        {
			this->gameDb->AddTable(gameTable);
        }
		gameTable->CommitChanges();	
	}

	// create levels table and add attributes

	staticTable = TemplateExporter::CreateTable(this->staticDb, "_Template_Levels");
	if(!AttrId::IsValidName("StartLevel"))
		AttributeDefinitionBase::RegisterDynamicAttribute("StartLevel", Util::FourCC(), StringType, ReadOnly);
	if(!AttrId::IsValidName("Name"))
		AttributeDefinitionBase::RegisterDynamicAttribute("Name", Util::FourCC(), StringType, ReadOnly);
	if(!AttrId::IsValidName("Id"))
		AttributeDefinitionBase::RegisterDynamicAttribute("Id", Util::FourCC(), StringType, ReadOnly);
	if(!AttrId::IsValidName("_Layers"))
		AttributeDefinitionBase::RegisterDynamicAttribute("_Layers", Util::FourCC(), StringType, ReadWrite);
	if(!AttrId::IsValidName("Center"))
		AttributeDefinitionBase::RegisterDynamicAttribute("Center", Util::FourCC(), Float4Type, ReadOnly);
	if(!AttrId::IsValidName("Extents"))
		AttributeDefinitionBase::RegisterDynamicAttribute("Extents", Util::FourCC(), Float4Type, ReadOnly);
	if (!AttrId::IsValidName("WorldCenter"))
		AttributeDefinitionBase::RegisterDynamicAttribute("WorldCenter", Util::FourCC(), Float4Type, ReadOnly);
	if (!AttrId::IsValidName("WorldExtents"))
		AttributeDefinitionBase::RegisterDynamicAttribute("WorldExtents", Util::FourCC(), Float4Type, ReadOnly);
	if (!AttrId::IsValidName("PostEffectPreset"))
		AttributeDefinitionBase::RegisterDynamicAttribute("PostEffectPreset", Util::FourCC(), StringType, ReadWrite);
	if (!AttrId::IsValidName("GlobalLightTransform"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightTransform", Util::FourCC(), Matrix44Type, ReadWrite);
				
	TemplateExporter::CreateColumn(staticTable, Column::Default, AttrId("Id"));
	TemplateExporter::CreateColumn(staticTable, Column::Default, AttrId("Name"));
	TemplateExporter::CreateColumn(staticTable, Column::Default, AttrId("StartLevel"));
	TemplateExporter::CreateColumn(staticTable, Column::Default, AttrId("_Layers"));
	TemplateExporter::CreateColumn(staticTable, Column::Default, AttrId("Center"));
	TemplateExporter::CreateColumn(staticTable, Column::Default, AttrId("Extents"));
	
	this->staticDb->AddTable(staticTable);
	staticTable->CommitChanges();

	
	gameTable = TemplateExporter::CreateTable(this->gameDb, "_Instance_Levels");
												
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("Id"));
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("Name"));
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("StartLevel"));
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("_Layers"));
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("WorldCenter"));
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("WorldExtents"));
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("PostEffectPreset"));
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("GlobalLightTransform"));
	
	this->gameDb->AddTable(gameTable);
	gameTable->CommitChanges();
 
	gameTable = TemplateExporter::CreateTable(this->gameDb, "_Instance__Environment");
	TemplateExporter::CreateColumn(gameTable, Column::Primary, AttrId("_ID"));
	TemplateExporter::CreateColumn(gameTable, Column::Default, AttrId("_Level"));
	
	this->gameDb->AddTable(gameTable);
	gameTable->CommitChanges();

	/// seems to be unused
#if 0
	if(!this->gameDb->HasTable("_Instance_VisibilityCluster"))
	{
		table = this->CreateTable(this->gameDb,"_Instance_VisibilityCluster");
		Attr::AttrId id = Attr::AttrId("Guid");
		this->CreateColumn(table,Column::Primary,id);
		id = Attr::AttrId("_ID");
		this->CreateColumn(table,Column::Default,id);
		id = Attr::AttrId("_Level");
		this->CreateColumn(table,Column::Default,id);
		id = Attr::AttrId("Transform");
		this->CreateColumn(table,Column::Default,id);
		id = Attr::AttrId("VCNumBoundingBoxes");
		this->CreateColumn(table,Column::Default,id);
		id = Attr::AttrId("VCBoundingBoxes");
		this->CreateColumn(table,Column::Default,id);
		id = Attr::AttrId("VCNumEnvObjects");
		this->CreateColumn(table,Column::Default,id);
		id = Attr::AttrId("VCEnvObjects");
		this->CreateColumn(table,Column::Default,id);
		id = Attr::AttrId("VCEnvObjectNames");
		this->CreateColumn(table,Column::Default,id);

		this->gameDb->AddTable(table);

		dataset = table->CreateDataset();
		dataset->AddAllTableColumns();
		dataset->CommitChanges();

	}
#endif
	gameTable = 0;
	staticTable = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
TemplateExporter::ExportUiProperties()
{
	this->logger->Print("Exporting UI tables and layout information\n");
	if (IO::IoServer::Instance()->FileExists("root:data/tables/ui.xml"))
	{
		if (this->staticDb->HasTable("_UI_Layouts"))
		{
			this->staticDb->DeleteTable("_UI_Layouts");
		}

		Ptr<Db::Table> table = this->CreateTable(this->staticDb, "_UI_Layouts");
		this->CreateColumn(table, Column::Primary, AttrId("UILayoutFile"));
		this->CreateColumn(table, Column::Default, AttrId("UILayout"));

		this->staticDb->AddTable(table);

		Ptr<Dataset> dataset;
		Ptr<ValueTable> valueTable;

		dataset = table->CreateDataset();
		dataset->AddAllTableColumns();
		dataset->PerformQuery();
		valueTable = dataset->Values();

		if (this->staticDb->HasTable("_UI_Fonts"))
		{
			this->staticDb->DeleteTable("_UI_Fonts");
		}

		Ptr<Db::Table> ftable = this->CreateTable(this->staticDb, "_UI_Fonts");
		this->CreateColumn(ftable, Column::Primary, AttrId("UIFontFile"));
		this->CreateColumn(ftable, Column::Default, AttrId("UIFontFamily"));
		this->CreateColumn(ftable, Column::Default, AttrId("UIFontStyle"));
		this->CreateColumn(ftable, Column::Default, AttrId("UIFontWeight"));

		this->staticDb->AddTable(ftable);

		Ptr<Dataset> fdataset;
		Ptr<ValueTable> fvalueTable;
		fdataset = ftable->CreateDataset();
		fdataset->AddAllTableColumns();
		fdataset->PerformQuery();
		fvalueTable = fdataset->Values();

		if (this->staticDb->HasTable("_UI_Scripts"))
		{
			this->staticDb->DeleteTable("_UI_Scripts");
		}

		Ptr<Db::Table> stable = this->CreateTable(this->staticDb, "_UI_Scripts");
		this->CreateColumn(stable, Column::Primary, AttrId("UIScriptFile"));		

		this->staticDb->AddTable(stable);

		Ptr<Dataset> sdataset;
		Ptr<ValueTable> svalueTable;
		sdataset = stable->CreateDataset();
		sdataset->AddAllTableColumns();
		sdataset->PerformQuery();
		svalueTable = sdataset->Values();

		Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream("root:data/tables/ui.xml");
		stream->SetAccessMode(IO::Stream::ReadAccess);
		Ptr<IO::XmlReader> reader = IO::XmlReader::Create();

		reader->SetStream(stream);
		if (reader->Open())
		{
			if (reader->HasNode("/Layouts"))
			{
				reader->SetToNode("/Layouts");
				if (reader->SetToFirstChild())
				{
					do
					{
						if (reader->GetBool("autoload"))
						{

							IndexT row = valueTable->AddRow();
							valueTable->SetString(AttrId("UILayoutFile"), row, reader->GetString("file"));
							valueTable->SetString(AttrId("UILayout"), row, reader->GetString("name"));
						}
					} while (reader->SetToNextChild());
				}
			}
			if (reader->HasNode("/Fonts"))
			{
				reader->SetToNode("/Fonts");
				if (reader->SetToFirstChild())
				{
					do
					{
						if (reader->GetBool("autoload"))
						{
							IndexT row = fvalueTable->AddRow();
							fvalueTable->SetString(AttrId("UIFontFile"), row, reader->GetString("file"));
							fvalueTable->SetString(AttrId("UIFontFamily"), row, reader->GetString("family"));
							fvalueTable->SetInt(AttrId("UIFontStyle"), row, reader->GetInt("style"));
							fvalueTable->SetInt(AttrId("UIFontStyle"), row, reader->GetInt("weight"));

						}

					} while (reader->SetToNextChild());
				}
			}
			if (reader->HasNode("/Scripts"))
			{
				reader->SetToNode("/Scripts");
				if (reader->SetToFirstChild())
				{
					do
					{
						if (reader->GetBool("autoload"))
						{
							IndexT row = svalueTable->AddRow();
							svalueTable->SetString(AttrId("UIScriptFile"), row, reader->GetString("file"));							
						}

					} while (reader->SetToNextChild());
				}
			}
			reader->Close();
		}

		dataset->CommitChanges();
		table->CommitChanges();
		table = 0;
		fdataset->CommitChanges();
		ftable->CommitChanges();
		ftable = 0;
		sdataset->CommitChanges();
		stable->CommitChanges();
		stable = 0;
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
TemplateExporter::ExportFile( const IO::URI& file )
{
	this->logger->Print("Exporting template: %s\n", file.GetHostAndLocalPath().AsCharPtr());
	
	this->Progress(5, file.GetHostAndLocalPath().ExtractFileName());

	Ptr<Stream> templateStream = IoServer::Instance()->CreateStream(file);
	Ptr<XmlReader> xmlReader = XmlReader::Create();

	Ptr<Table> table;
	Ptr<Dataset> dataset;
	Ptr<ValueTable> valueTable;
	Ptr<Sqlite3Command> command = Sqlite3Command::Create();

	
		
	if (!templateStream->Open())
	{
		this->logger->Error("Could not open template: %s", file.GetHostAndLocalPath().AsCharPtr());
		this->SetHasErrors(true);
		return;
	}

	xmlReader->SetStream(templateStream);

	if (!xmlReader->Open())
	{
		this->logger->Error("Could not open XML stream: %s", file.GetHostAndLocalPath().AsCharPtr());
		this->SetHasErrors(true);
		return;
	}


	String fileName = file.AsString();
	fileName.StripFileExtension();
	String category = xmlReader->GetCurrentNodeName();	
	bool isVirtual = xmlReader->GetOptBool("IsVirtualCategory",false);

	Util::Array<AttrId> catAttributes;

	if(isVirtual)
	{
		// virtual categories dont have blueprints or properties and are used for storing
		// other data as animevents etc, we can just create the category and use the provided 
		// attributes in the file
		xmlReader->SetToFirstChild();		
		Array<String> attributes = xmlReader->GetAttrs();
		for(IndexT j = 0;j<attributes.Size();j++)
		{
			Ptr<IDLAttribute> at = this->blueprintManager->GetAttributeByName(attributes[j]);
			catAttributes.Append(AttrId(at->GetName()));			
		}
		xmlReader->SetToParent();		
	}
	else
	{
		if(!this->blueprintManager->HasCategory(category))
		{
			// unknown non virtual category, should probably assert, will just ignore it
			this->logger->Warning("Unknown category in db file: %s, ignoring\n", category.AsCharPtr());
			xmlReader->Close();
			return;
		}
		Attr::AttributeContainer catAttributesContainer = this->blueprintManager->GetCategoryAttributes(category);
		catAttributes = catAttributesContainer.GetAttrs().KeysAsArray();
	}

    if(!this->gameDb->HasTable("_Instance_"+category))
    {    
        // in case no blueprint exists we will add an instance table for it
        Ptr<Table> insttable = TemplateExporter::CreateTable(this->gameDb, "_Instance_" + category);
        TemplateExporter::CreateColumn(insttable, Column::Primary, AttrId("_ID"));
        TemplateExporter::CreateColumn(insttable, Column::Default, AttrId("_Level"));
        TemplateExporter::CreateColumn(insttable, Column::Default, AttrId("_Layers"));
        this->gameDb->AddTable(insttable);
        insttable->CommitChanges();		
    }

	bool templateExists = this->staticDb->HasTable("_Template_" + category);		
	if (!templateExists)
	{
		table = TemplateExporter::CreateTable(this->staticDb, "_Template_" + category);		
		if (!isVirtual)
		{
			TemplateExporter::CreateColumn(table, Column::Primary, Attr::AttrId("Id"));
		}		
		IndexT i;
		for(i=0;i<catAttributes.Size();i++)
		{
			if(catAttributes[i] != Attr::AttrId("Id"))
			{
				TemplateExporter::CreateColumn(table, Column::Default, catAttributes[i]);
			}			
		}	
		this->staticDb->AddTable(table);
		table->CommitChanges();
	}

	this->AddCategory(category, isVirtual, false, "_Template_" + category, "");
	
	table = this->staticDb->GetTableByName("_Template_" + category);
	
	dataset = table->CreateDataset();
	dataset->PerformQuery();

	valueTable = dataset->Values();
	

	if (xmlReader->SetToFirstChild("Item"))
	{
		do 
		{
			IndexT index = valueTable->AddRow();
			for(int i=0;i<catAttributes.Size();i++)
			{


				AttrId id = catAttributes[i];
				ValueType type = id.GetValueType();
				if (!valueTable->HasColumn(id))
				{
					valueTable->AddColumn(id);
				}
				if(xmlReader->HasAttr(id.GetName().AsCharPtr()))
				{
					if (type == StringType)
					{
						valueTable->SetString(id,index,xmlReader->GetString(id.GetName().AsCharPtr()));

					}
					else if (type == IntType)
					{
						valueTable->SetInt(id,index,xmlReader->GetInt(id.GetName().AsCharPtr()));

					}
					else if (type == BoolType)
					{
						valueTable->SetBool(id,index,xmlReader->GetBool(id.GetName().AsCharPtr()));

					}
					else if (type == FloatType)
					{
						valueTable->SetFloat(id,index,xmlReader->GetFloat(id.GetName().AsCharPtr()));

					}
					else if (type == Matrix44Type)
					{
						valueTable->SetMatrix44(id,index,xmlReader->GetMatrix44(id.GetName().AsCharPtr()));

					}
					else if (type == Float4Type)
					{
						valueTable->SetFloat4(id,index,xmlReader->GetFloat4(id.GetName().AsCharPtr()));
					}
					else if (type == GuidType)
					{
						valueTable->SetGuid(id,index,Util::Guid::FromString(xmlReader->GetString(id.GetName().AsCharPtr())));
					}
				}


			} 
		}
		while (xmlReader->SetToNextChild("Item"));
	}

	dataset->CommitChanges();
	dataset = 0;
	valueTable = 0;



	xmlReader->Close();
	templateStream->Close();

	table = 0;
	valueTable = 0;
	command = 0;
	templateStream = 0;
	xmlReader = 0;

}

//------------------------------------------------------------------------------
/**
*/
void 
TemplateExporter::CollectAttributes(const Ptr<Db::Database>& db)
{
	Ptr<Db::Table> table;
	Ptr<Db::Dataset> dataset;
	Ptr<Db::ValueTable> valueTable;
	
	table = db->GetTableByName("_Attributes");
	dataset = table->CreateDataset();
	dataset->AddAllTableColumns();	

	dataset->PerformQuery();
	valueTable = dataset->Values();

	IndexT index;
	String query;
	Ptr<Db::Sqlite3Command> command = Db::Sqlite3Command::Create();
	FixedArray<Attr::AttrId> attributes = Attr::AttrId::GetAllAttrIds();

	for (int attrIndex = 0; attrIndex < attributes.Size(); attrIndex++)
	{
		Attr::AttrId attribute = attributes[attrIndex];
		index = valueTable->FindRowIndexByAttr(Attr::Attribute(Attr::AttrName, attribute.GetName()));
		if (index == InvalidIndex)
		{
			index = valueTable->AddRow();
		}
		valueTable->SetString(Attr::AttrType, index, Attribute::ValueTypeToString(attribute.GetValueType()));
		valueTable->SetBool(Attr::AttrReadWrite, index, attribute.GetAccessMode() != ReadOnly);
		valueTable->SetBool(Attr::AttrDynamic, index, attribute.IsDynamic());
		valueTable->SetString(Attr::AttrName, index, attribute.GetName());		
	}

	dataset->CommitChanges();
}

} // namespace ToolkitUtil