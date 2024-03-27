#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::LevelExporter
    
    Exports Json-based level to binary .nlvl files
    
    (C) 2012-2015 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/base/exporterbase.h"
#include "toolkit-common/platform.h"
#include "io/jsonreader.h"
#include "toolkit-common/toolkitconsolehandler.h"
#include "memdb/database.h"

namespace Game
{
class World;
}

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
    void ExportFile(const IO::URI& file) override;
    /// exports a directory (does the same as ExportAll)
    void ExportDir(const Util::String& category) override;
    /// exports all levels
    void ExportAll() override;

    /// set pointer to a valid logger object
    void SetLogger(ToolkitUtil::Logger* logger);
private:

    /// exports level data
    bool ExportLevel(const Ptr<IO::JsonReader>& reader, Game::World* world, const IO::URI& outputFile);

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

} // namespace ToolkitUtil
//------------------------------------------------------------------------------