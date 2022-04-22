#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::NglTFExporter

    Exports an GLTF file into the binary nebula format

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/base/exporterbase.h"
#include "toolkit-common/base/exporttypes.h"
#include "ngltfscenewriter.h"
#include "gltf/gltfdata.h"
#include "node/ngltfscene.h"
#include "toolkitutil/texutil/textureconverter.h"

namespace ToolkitUtil
{
class NglTFExporter : public Base::ExporterBase
{
    __DeclareClass(NglTFExporter);
public:

    /// constructor
    NglTFExporter();
    /// destructor
    ~NglTFExporter();

    /// opens the exporter
    void Open();
    /// closes the exporter
    void Close();

    /// starts exporter, opens scene and saves data
    bool StartExport(const IO::URI& file);
    /// ends exporter, cleans up all allocated resources
    void EndExport();

    /// set texture converter
    void SetTextureConverter(TextureConverter* texConv);

    /// exports a single file
    void ExportFile(const IO::URI& file);
    /// exports all files in a directory
    void ExportDir(const Util::String& dirName);
    /// exports ALL the models
    void ExportAll();

    /// set desired scale
    void SetScale(float f);
    /// get the desired scale
    const float GetScale() const;

    /// set the export method
    void SetExportMode(const ToolkitUtil::ExportMode& mode);
    /// set the mesh export flags
    void SetExportFlags(const ToolkitUtil::ExportFlags& exportFlags);

private:
    /// checks whether or not a file needs to be updated 
    bool NeedsConversion(const Util::String& path);

    ToolkitUtil::ExportMode exportMode;
    ToolkitUtil::ExportFlags exportFlags;
    Util::String file;

    Ptr<NglTFScene> scene;

    Ptr<NglTFSceneWriter> sceneWriter;

    Gltf::Document gltfScene;

    Util::Array<Util::String> exportedMeshes;

    TextureConverter* texConverter;

    float scaleFactor;
};

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFExporter::SetScale(float f)
{
    this->scaleFactor = f;
}

//------------------------------------------------------------------------------
/**
*/
inline const float
NglTFExporter::GetScale() const
{
    return this->scaleFactor;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFExporter::SetExportMode(const ToolkitUtil::ExportMode& mode)
{
    this->exportMode = mode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFExporter::SetExportFlags(const ToolkitUtil::ExportFlags& exportFlags)
{
    this->exportFlags = exportFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFExporter::SetTextureConverter(TextureConverter* texConv)
{
    this->texConverter = texConv;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------