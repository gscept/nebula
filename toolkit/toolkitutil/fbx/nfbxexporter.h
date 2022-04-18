#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxExporter
    
    Exports an FBX file into the binary nebula format
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/base/exporterbase.h"
#include "node/nfbxscene.h"
#include "toolkit-common/base/exporttypes.h"
#include "modelutil/modelphysics.h"
#include "nfbxscenewriter.h"


//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class NFbxExporter : public Base::ExporterBase
{
    __DeclareClass(NFbxExporter);
public:

    /// constructor
    NFbxExporter();
    /// destructor
    virtual ~NFbxExporter();

    /// opens the exporter
    void Open();
    /// closes the exporter
    void Close();

    /// starts exporter, opens scene and saves data
    bool StartExport(const IO::URI& file);
    /// ends exporter, cleans up all allocated resources
    void EndExport();

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

    /// exports list of files, used for paralell jobs
    void ExportList(const Util::Array<Util::String>& files);

    /// set the export method
    void SetExportMode(const ToolkitUtil::ExportMode& mode);
    /// set the mesh export flags
    void SetExportFlags(const ToolkitUtil::ExportFlags& exportFlags);

    /// gets list of the meshes exported from the previous pass
    const Util::Array<Util::String>& GetExportedMeshes() const;
    /// gets list of exported models from the previous pass
    const Util::Array<Util::String>& GetExportedModels() const;
    /// gets list of exported animations from the previous pass
    const Util::Array<Util::String>& GetExportedAnimations() const;

    /// set the progress callback
    void SetFbxProgressCallback(FbxProgressCallback progressCallback);
private:

    /// checks whether or not a file needs to be updated (constructs dest path from source)
    bool NeedsConversion(const Util::String& path);

    FbxProgressCallback progressFbxCallback;
    static Threading::CriticalSection cs;
    static FbxManager* sdkManager;
    static FbxIOSettings* ioSettings;
    FbxScene* fbxScene;

    Ptr<NFbxSceneWriter> sceneWriter;
    Ptr<NFbxScene> scene;
    ToolkitUtil::ExportMode exportMode;
    ToolkitUtil::ExportFlags exportFlags;
    Util::String file;

    Util::Array<Util::String> exportedMeshes;
    Util::Array<Util::String> exportedModels;
    Util::Array<Util::String> exportedAnimations;

    float scaleFactor;  
}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxExporter::SetExportMode( const ToolkitUtil::ExportMode& mode )
{
    this->exportMode = mode;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxExporter::SetExportFlags( const ToolkitUtil::ExportFlags& exportFlags )
{
    this->exportFlags = exportFlags;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxExporter::SetFbxProgressCallback( FbxProgressCallback progressCallback )
{
    this->progressFbxCallback = progressCallback;
}


//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::String>& 
NFbxExporter::GetExportedMeshes() const
{
    return this->exportedMeshes;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxExporter::SetScale( float f )
{
    this->scaleFactor = f;
}

//------------------------------------------------------------------------------
/**
*/
inline const float 
NFbxExporter::GetScale() const
{
    return this->scaleFactor;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------