#pragma once
//------------------------------------------------------------------------------
/**
    @class Toolkit::PhysicsExporter
    
    Exports physics
    
    (C) 2012 gscept
*/
#include "base/exporterbase.h"
#include "toolkitutil/platform.h"
#include "math/bbox.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class PhysicsExporter : public Base::ExporterBase
{
    __DeclareClass(PhysicsExporter);
public:
    /// constructor
    PhysicsExporter();
    /// destructor
    virtual ~PhysicsExporter();

    /// opens the exporter
    void Open();
    /// closes the exporter
    void Close();

    /// traverses the file system recursively and exports ALL models as physics
    void ExportAll();
    /// traverses the file system at a category and exports ALL models as physics for that category
    void ExportDir(const Util::String& category);
    /// exports a single model to physics
    void ExportFile(const IO::URI& file);


private:

    /// checks if there is a need to generate physics from bounding box
    bool NeedsDummyPhysics(const Util::String& category, const Util::String& file);
    /// checks whether or not a file needs to be updated (constructs dest path from source)
    bool NeedsConversion(const Util::String& path);
    /// checks whether or not a file needs to be updated 
    bool NeedsConversion(const Util::String& src, const Util::String& dst);

    /// creates and writes a bullet resource
    void CreateDummyPhysics(const Math::bbox& box, const Util::String& category, const Util::String& file);
}; 


} // namespace Toolkit
//------------------------------------------------------------------------------