#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::GameExporter
    
    Exports both levels and templates, and ensures the database is always clean
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "base/exporterbase.h"
#include "levelexporter.h"
#include "templateexporter.h"
#include "toolkitconsolehandler.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class GameExporter : public Base::ExporterBase
{
    __DeclareClass(GameExporter);
public:
    /// constructor
    GameExporter();
    /// destructor
    virtual ~GameExporter();

    /// opens the exporter
    void Open();
    /// closes the exporter
    void Close();

    /// export only template and instance tables
    void ExportTables();
    /// exports both game and levels
    void ExportAll();
    /// set pointer to a valid logger object
    void SetLogger(Logger* logger);
    /// get tool logs
    const Util::Array<ToolkitUtil::ToolLog> & GetLogs() const;
    
private:    
    Logger* logger;
    Util::Array<ToolkitUtil::ToolLog> logs;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
GameExporter::SetLogger(Logger* l)
{
    this->logger = l;
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Util::Array<ToolkitUtil::ToolLog> &
GameExporter::GetLogs() const
{
    return this->logs;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------