#pragma once
//------------------------------------------------------------------------------
/**
    Generic base class for model exporters

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/base/importerbase.h"
#include "toolkit-common/base/exporttypes.h"
#include "toolkitutil/texutil/textureconverter.h"
#include "model/import/base/scene.h"
namespace ToolkitUtil
{

class ModelImporter : public Base::ImporterBase
{
    
public:

    /// Constructor
    ModelImporter();
    /// Destructor
    ~ModelImporter();

    /// Run implementation scene parse
    virtual bool ParseScene(ToolkitUtil::ImportFlags importFlags, float scale);

    /// Set logger
    void SetLogger(ToolkitUtil::Logger* logger);
    /// exports a single file
    void ImportFile(const IO::URI& file, ToolkitUtil::ImportFlags importFlags, float scale);
    /// set texture converter
    void SetTextureConverter(TextureConverter* texConv);

protected:
    /// checks whether or not a file needs to be updated 
    bool NeedsConversion(const Util::String& path);

    Util::String file;
    IO::URI path;

    /// work array for output files that should be written to intermediate.
    Util::Array<IO::URI> outputFiles;

    Scene* scene;
    TextureConverter* texConverter;

    ToolkitUtil::Logger* logger;
};

//------------------------------------------------------------------------------
/**
*/
inline void
ModelImporter::SetTextureConverter(TextureConverter* texConv)
{
    this->texConverter = texConv;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelImporter::SetLogger(ToolkitUtil::Logger* logger)
{
    this->logger = logger;
}

} // namespace ToolkitUtil
