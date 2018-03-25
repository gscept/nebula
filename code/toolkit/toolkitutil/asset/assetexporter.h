#pragma once
//------------------------------------------------------------------------------
/**
	@class ToolkitUtil::AssetExporter
	
	The asset exporter takes a single directory and exports any models, textures and gfx-sources.

    This isn't based on an exporter class, because it has no need for incremental batching.
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkitutil/base/exporterbase.h"
#include "toolkitutil/fbx/nfbxexporter.h"
#include "texutil/textureconverter.h"
#include "modelutil/modelbuilder.h"
#include "modelutil/modeldatabase.h"
#include "surface/surfaceexporter.h"
#include "toolkitconsolehandler.h"

namespace ToolkitUtil
{
class AssetExporter : public Base::ExporterBase
{
	__DeclareClass(AssetExporter);
public:

    enum ExportModes
    {
        FBX = 1 << 0,								// checking this will cause FBXes to get exported
        Models = 1 << 1,							// checking this will cause models to get exported
        Textures = 1 << 2,							// checking this will cause textures to get exported
		Surfaces = 1 << 3,							// checking this will cause surfaces to get exported
        All = FBX + Models + Textures + Surfaces,	// shortcut for exporting everything

        ForceFBX = 1 << 4,              // will force the FBX batcher to update meshes and characters despite time stamps
        ForceModels = 1 << 5,           // will force the model builder to create models despite time stamps
        ForceTextures = 1 << 6,         // will force the texture converter to convert textures despite time stamps
		ForceSurfaces = 1 << 7,			// will force the surface exporter to convert surfaces despite time stamps
        ForceAll = ForceFBX + ForceModels + ForceTextures
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

	/// explicitly exports the system directories (toolkit:system and toolkit:lighting)
	void ExportSystem();
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
    ToolkitUtil::TextureConverter textureExporter;
	Ptr<ToolkitUtil::SurfaceExporter> surfaceExporter;
    Ptr<ToolkitUtil::ModelBuilder> modelBuilder;	
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