#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SurfaceExporter
    
    Exports surface files from the work folder and converts them to binary XMLs in export.
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/base/exporterbase.h"
namespace ToolkitUtil
{
class SurfaceExporter : public Base::ExporterBase
{
    __DeclareClass(SurfaceExporter);
public:
    /// constructor
    SurfaceExporter();
    /// destructor
    virtual ~SurfaceExporter();

    /// exports a single file
    void ExportFile(const IO::URI& file);
};
} // namespace ToolkitUtil