#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetExporter
    
    The asset exporter takes a single directory and exports any models, textures and gfx-sources.

    This isn't based on an exporter class, because it has no need for incremental batching.
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/base/exporterbase.h"
#include "toolkitutil/fbx/nfbxexporter.h"
#include "texutil/textureconverter.h"
#include "modelutil/modelbuilder.h"
#include "modelutil/modeldatabase.h"
#include "surface/surfaceexporter.h"
#include "toolkit-common/toolkitconsolehandler.h"
#include "toolkitutil/gltf/ngltfexporter.h"

namespace ToolkitUtil
{
class AssetExporter : public Base::ExporterBase
{
    __DeclareClass(AssetExporter);
public:

    enum ExportModes
    {
        FBX = 1 << 0,                                                 // checking this will cause FBXes to get exported
        Models = 1 << 1,                                              // checking this will cause models to get exported
        Textures = 1 << 2,                                            // checking this will cause textures to get exported
        Surfaces = 1 << 3,                                            // checking this will cause surfaces to get exported
        GLTF = 1 << 4,                                                // checking this will cause GLTFs to get exported
        Physics = 1 << 5,                                             // checking this will cause physics to get exported
        All = FBX + Models + Textures + Surfaces + GLTF + Physics,    // shortcut for exporting everything

        ForceFBX = 1 << 6,              // will force the FBX batcher to update meshes and characters despite time stamps
        ForceModels = 1 << 7,           // will force the model builder to create models despite time stamps
        ForceTextures = 1 << 8,         // will force the texture converter to convert textures despite time stamps
        ForceSurfaces = 1 << 9,         // will force the surface exporter to convert surfaces despite time stamps
        ForceGLTF = 1 << 10,            // will force the gltf exporter to convert meshes, textures and characters despite time stamps
        ForcePhysics = 1 << 11,         // will force the physics exporter to export physics assets despite time stamps
        ForceAll = ForceFBX + ForceModels + ForceTextures + ForceGLTF + ForcePhysics
    };

    /// constructor
    AssetExporter();
    /// destructor
    virtual ~AssetExporter();

    /// opens the exporter
    void Open();
    /// closes the exporter
    void Close();
    /// returns true if exporter is open
    bool IsOpen() const;

    /// trigger refresh of any resource databases due to source change
    void UpdateSource();
    /// exports a single category
    void ExportDir(const Util::String& category);
    /// export a single folder with absolute path
    void ExportFolder(const Util::String& folder, const Util::String& category);
    /// exports all files
    void ExportAll();

    /// exports list of files, used for parallel jobs
    void ExportList(const Util::Array<Util::String>& files);

    /// set export mode flag
    void SetExportMode(unsigned int mode);
    
    /// get failed files (if any)
    const Util::Array<ToolkitUtil::ToolLog> & GetMessages() const;

private:
    Ptr<ToolkitUtil::NFbxExporter> fbxExporter;
    Ptr<ToolkitUtil::NglTFExporter> gltfExporter;
    ToolkitUtil::TextureConverter textureExporter;
    Ptr<ToolkitUtil::SurfaceExporter> surfaceExporter;
    Ptr<ToolkitUtil::ModelBuilder> modelBuilder;    
    ToolkitUtil::TextureAttrTable textureAttrTable;
    Logger logger;
    unsigned int mode;
    Util::Array<ToolLog> messages;
};

///------------------------------------------------------------------------------
/**
*/
inline 
const Util::Array<ToolkitUtil::ToolLog> &
AssetExporter::GetMessages() const
{
    return this->messages;
}
} // namespace ToolkitUtil