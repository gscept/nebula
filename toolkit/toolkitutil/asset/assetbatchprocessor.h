#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetBatchProcessor
    
    The asset batcher takes a folder, file 

    This isn't based on an exporter class, because it has no need for incremental batching.
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "asset/assetprocessorbase.h"

#include "toolkitutil/model/import/fbx/fbxfileimporter.h"
#include "texutil/textureconverter.h"
#include "toolkitutil/model/modelutil/modelbuilder.h"
#include "toolkitutil/model/modelutil/modeldatabase.h"
#include "toolkitutil/surface/surfaceexporter.h"
#include "toolkitutil/toolkitconsolehandler.h"
#include "toolkitutil/model/import/gltf/gltffileimporter.h"
#include "toolkitutil/particle/particleexporter.h"

namespace ToolkitUtil
{
class AssetBatchProcessor : public Base::AssetProcessorBase
{
    __DeclareClass(AssetBatchProcessor);
public:

    enum class PackageModes
    {
        None = 0x0,
        Assets = 1 << 0,                                              // checking this will cause .nasset files to be ewxported
        Textures = 1 << 1,                                            // checking this will cause textures to get exported
        Materials = 1 << 2,                                            // checking this will cause surfaces to get exported
        Particles = 1 << 3,
        Audio = 1 << 4,
        Navigation = 1 << 10,                                         // bakes .nanav files, after everything else is packaged
        All = Assets | Textures | Materials | Particles | Audio | Navigation,    // shortcut for exporting everything

        ForceAssets = 1 << 5,           // will force exporting .nasset
        ForceTextures = 1 << 6,         // will force the texture converter to convert textures despite time stamps
        ForceMaterials = 1 << 7,         // will force the surface exporter to convert surfaces despite time stamps
        ForceParticles = 1 << 8,         // will force the surface exporter to convert surfaces despite time stamps
        ForceAudio = 1 << 9,
        ForceNavigation = 1 << 11,      // will bake navmeshes even if their inputs didn't change
        ForceAll = ForceAssets | ForceTextures | ForceMaterials | ForceParticles | ForceAudio | ForceNavigation
    };

    enum class ImportModes
    {
        None = 0x0,
        FBX = 1 << 0,
        GLTF = 1 << 1,
        Images = 1 << 2,
        Sound = 1 << 3,

        All = FBX | GLTF | Images | Sound,

        ForceFBX = 1 << 4,
        ForceGLTF = 1 << 5,
        ForceImages = 1 << 6,
        ForceSound = 1 << 7,
        ForceAll = ForceFBX | ForceGLTF | ForceImages | ForceSound
    };

    /// constructor
    AssetBatchProcessor();
    /// destructor
    virtual ~AssetBatchProcessor();

    /// trigger refresh of any resource databases due to source change
    void UpdateSource();
    
    /// exports a single file
    void ProcessFile(const IO::URI& file) override;
    /// exports a single directory
    void ProcessDir(const Util::String& dir);
    /// exports all files
    void ProcessAll(const Util::String& source);

    /// exports list of files, used for parallel jobs
    void ProcessList(const Util::Array<Util::String>& files);
    /// bakes the navmeshes the processed files queued. Navmeshes read the exported meshes and textures,
    /// so call this after everything else is processed
    void ProcessNavMeshes();

    /// Set import mode
    void SetImportMode(unsigned int mode);
    /// Set package mode
    void SetPackageMode(unsigned int mode);
    
    /// get failed files (if any)
    const Util::Array<ToolkitUtil::ToolLog> & GetMessages() const;

private:
    unsigned int importMode, packageMode;
    Util::Array<ToolLog> messages;
    /// work file and export file of every navmesh waiting for ProcessNavMeshes
    Util::Array<Util::KeyValuePair<IO::URI, Util::String>> pendingNavMeshes;
};

__ImplementEnumBitOperators(AssetBatchProcessor::PackageModes);
__ImplementEnumBitOperators(AssetBatchProcessor::ImportModes);

///------------------------------------------------------------------------------
/**
*/
inline const Util::Array<ToolkitUtil::ToolLog> &
AssetBatchProcessor::GetMessages() const
{
    return this->messages;
}


//------------------------------------------------------------------------------
/**
*/
inline void
AssetBatchProcessor::SetImportMode(unsigned int mode)
{
    this->importMode = mode;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AssetBatchProcessor::SetPackageMode(unsigned int mode)
{
    this->packageMode = mode;
}

} // namespace ToolkitUtil