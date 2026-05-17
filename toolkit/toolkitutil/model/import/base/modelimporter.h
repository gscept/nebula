#pragma once
//------------------------------------------------------------------------------
/**
    Generic base class for model exporters

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/base/assetprocessorbase.h"
#include "toolkit-common/base/exporttypes.h"
#include "toolkitutil/texutil/textureconverter.h"
#include "model/import/base/scene.h"
namespace ToolkitUtil
{

class ModelImporter : public Base::AssetProcessorBase
{
    
public:

    /// Constructor
    ModelImporter();
    /// Destructor
    ~ModelImporter();

    /// Run implementation scene parse
    virtual bool ParseScene(ToolkitUtil::ImportFlags importFlags, float scale);

    /// exports a single file
    void ProcessFile(const IO::URI& file, ToolkitUtil::ImportFlags importFlags, float scale);

protected:
    /// checks whether or not a file needs to be updated 
    bool NeedsConversion(const Util::String& path);

    Util::String file;
    IO::URI path;

    /// work array for output files that should be written to intermediate.
    Util::Array<IO::URI> outputFiles;

    Scene* scene;
};

} // namespace ToolkitUtil
