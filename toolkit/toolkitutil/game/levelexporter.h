#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::LevelExporter
    
    Exports XML-based level to database
    
    (C) 2012-2015 Individual contributors, see AUTHORS file
*/
#include "base/exporterbase.h"
#include "platform.h"
#include "db/dbfactory.h"
#include "db/table.h"
#include "db/database.h"
#include "db/column.h"
#include "attr/attrid.h"
#include "io/xmlreader.h"
#include "toolkitutil/logger.h"


//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class LevelExporter : public Base::ExporterBase
{
    __DeclareClass(LevelExporter);
public:
    /// constructor
    LevelExporter();
    /// destructor
    virtual ~LevelExporter();

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

    /// sets the database factory
    void SetDbFactory(const Ptr<Db::DbFactory>& factory);
    /// set pointer to a valid logger object
    void SetLogger(ToolkitUtil::Logger* logger);
private:

    /// exports level data
    bool ExportLevel(const Ptr<IO::XmlReader>& reader, const Ptr<Db::Database>& db);

    /// creates a table in provided database
    Ptr<Db::Table> CreateTable(const Ptr<Db::Database>& db, const Util::String& tableName);
    /// creates a column in given database
    void CreateColumn(const Ptr<Db::Table>& table, Db::Column::Type type, Attr::AttrId attributeId);

    /// loads all attributes from a level object
    Util::Dictionary<Util::String, Util::String> LoadObjectAttributes(const Ptr<IO::XmlReader> & reader);

    bool dbHasStartLevel;
    Ptr<Db::DbFactory> dbFactory;
    Ptr<Db::Database> staticDb;
    Ptr<Db::Database> gameDb;
    ToolkitUtil::Logger* logger;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
LevelExporter::SetLogger(ToolkitUtil::Logger* l)
{
    this->logger = l;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
LevelExporter::SetDbFactory( const Ptr<Db::DbFactory>& factory )
{
    n_assert(!this->isOpen);
    this->dbFactory = factory;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------