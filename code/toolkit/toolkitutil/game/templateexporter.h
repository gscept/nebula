#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::TemplateExporter
    
    Exports templates to database
    
    (C) 2012-2015 Individual contributors, see AUTHORS file
*/
#include "base/exporterbase.h"
#include "db/database.h"
#include "db/dbfactory.h"
#include "io/xmlreader.h"
#include "editorfeatures/editorblueprintmanager.h"
#include "logger.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class TemplateExporter : public Base::ExporterBase
{
	__DeclareClass(TemplateExporter);
public:
	/// constructor
	TemplateExporter();
	/// destructor
	virtual ~TemplateExporter();

	/// opens the exporter
	void Open();
	/// closes the exporter
	void Close();

	/// exports a single level
	void ExportFile(const IO::URI& file);
	/// exports a directory (does the same as ExportAll)
	void ExportDir(const Util::String& category);
	/// exports all levels
	void ExportAll();
	/// exports ui table;
	void ExportUiProperties();

	/// sets the database factory
	void SetDbFactory(const Ptr<Db::DbFactory>& factory);

	/// creates a column in given database
	static void CreateColumn(const Ptr<Db::Table>& table, Db::Column::Type type, Attr::AttrId attributeId);

	/// creates a table in provided database
	static Ptr<Db::Table> CreateTable(const Ptr<Db::Database>& db, const Util::String& tableName);
	/// set pointer to a valid logger object
	void SetLogger(Logger* logger);

private:
	/// creates attributes from full attribute database (FAT)
	void CollectNIDLAttributes();
	/// creates attributes from blueprints
	void WriteBlueprintTables();
	/// fills attribute table with all registered attributes
	void CollectAttributes(const Ptr<Db::Database>& db);	
	/// create an entry in category table
	void AddCategory(const Util::String & name, bool isVirtual, bool isSystem, const Util::String& templateName, const Util::String& instanceName);
	

	Ptr<Db::DbFactory> dbFactory;
	Ptr<Db::Database> staticDb;
	Ptr<Db::Database> gameDb;
	Ptr<Db::Table> categoryTable;
	Ptr<Db::Dataset> categoryDataset;
	Ptr<Db::ValueTable> categoryValues;
	Logger* logger;
	Ptr<Toolkit::EditorBlueprintManager> blueprintManager;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
TemplateExporter::SetLogger(Logger* l)
{
	this->logger = l;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TemplateExporter::SetDbFactory( const Ptr<Db::DbFactory>& factory )
{
	n_assert(!this->isOpen);
	this->dbFactory = factory;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------