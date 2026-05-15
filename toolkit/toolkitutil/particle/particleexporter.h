#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SurfaceExporter
    
    Exports surface files from the work folder and converts them to binary XMLs in export.
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/base/importerbase.h"
namespace ToolkitUtil
{
class ParticleExporter : public Base::ImporterBase
{
    __DeclareClass(ParticleExporter);
public:
    /// constructor
    ParticleExporter();
    /// destructor
    virtual ~ParticleExporter();

    /// exports a single file
    void ImportFile(const IO::URI& file);
};
} // namespace ToolkitUtil