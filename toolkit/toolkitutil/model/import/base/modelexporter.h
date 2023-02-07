#pragma once
//------------------------------------------------------------------------------
/**
    Generic base class for model exporters

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/base/exporterbase.h"
#include "toolkit-common/base/exporttypes.h"
#include "toolkitutil/texutil/textureconverter.h"
#include "model/import/base/scene.h"
namespace ToolkitUtil
{

class ModelExporter : public Base::ExporterBase
{
    
public:

    /// Constructor
    ModelExporter();
    /// Destructor
    ~ModelExporter();

    /// Run implementation scene parse
    virtual bool ParseScene();

    /// exports a single file
    void ExportFile(const IO::URI& file);
    /// set texture converter
    void SetTextureConverter(TextureConverter* texConv);

    /// set the mesh export flags
    void SetExportFlags(const ToolkitUtil::ExportFlags& exportFlags);

protected:
    /// checks whether or not a file needs to be updated 
    bool NeedsConversion(const Util::String& path);

    ToolkitUtil::ExportFlags exportFlags;
    float sceneScale;
    Util::String file;
    IO::URI path;

    Util::Array<Util::String> exportedMeshes;

    Scene* scene;
    TextureConverter* texConverter;
};

//------------------------------------------------------------------------------
/**
*/
inline void
ModelExporter::SetExportFlags(const ToolkitUtil::ExportFlags& exportFlags)
{
    this->exportFlags = exportFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelExporter::SetTextureConverter(TextureConverter* texConv)
{
    this->texConverter = texConv;
}


} // namespace ToolkitUtil
