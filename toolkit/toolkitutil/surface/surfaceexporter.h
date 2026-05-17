#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SurfaceExporter
    
    Exports surface files from the work folder and converts them to binary XMLs in export.
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/base/assetprocessorbase.h"
namespace ToolkitUtil
{
class SurfaceExporter : public Base::AssetProcessorBase
{
    __DeclareClass(SurfaceExporter);
public:
    /// constructor
    SurfaceExporter();
    /// destructor
    virtual ~SurfaceExporter();

    /// exports a single file
    void ProcessFile(const IO::URI& file);
};
} // namespace ToolkitUtil