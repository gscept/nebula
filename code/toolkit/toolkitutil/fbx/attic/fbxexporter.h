#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FBXParser
    
    Main parser class for FBX
    
    (C) 2012 gscept
*/
#include "extlibs/fbx/fbxsdk.h"
#include "toolkitutil/base//exporterbase.h"
#include "fbxmodelparser.h"
#include "fbxskeletonparser.h"
#include "fbxanimationparser.h"
#include "fbxskinparser.h"
#include "toolkitutil/meshutil/meshbuilder.h"
#include "toolkitutil/platform.h"
#include "io/uri.h"
#include "io/stream.h"
#include "toolkitutil/n3util/n3writer.h"
#include "skinfragment.h"
#include "splitter/animsplitterhelper.h"
#include "io/console.h"
#include "splitter/batchattributes.h"
#include "node/fbxnoderegistry.h"
//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class FBXExporter : public Base::ExporterBase
{
	__DeclareClass(FBXExporter);
public:

	enum ExportMode
	{
		Static,
		Skeletal,

		NumImportModes
	};

	enum MeshFlag
	{
		None = 0,
		Merge = 1 << 0,
		RemoveRedudant = 1 << 1,

		NumMeshFlags = (1 << 2) - 1
	};

	/// constructor
	FBXExporter();
	/// destructor
	virtual ~FBXExporter();

	/// opens the exporter
	void Open();
	/// closes the exporter
	void Close();

	/// parses a file
	void ExportFile(const IO::URI& file);
	/// parses a directory
	void ExportDir(const Util::String& dirName);
	/// parses ALL the models
	void ExportAll();

	/// exports list of files, used for paralell jobs
	void ExportList(const Util::Array<Util::String>& files);

	/// set the export method
	void SetExportMode(const ExportMode& mode);
	/// set the mesh export flags
	void SetMeshFlags(const MeshFlag& meshFlag);

private:
	/// writes an N3 model as a character, and removes all skinned meshes from the mesh-list. Use this before WriteStaticN3
	bool WriteCharacterN3(Util::Array<ShapeNode*>& meshes, 
						  const Util::Array<Skeleton*>& skeletons, 
						  const Util::Dictionary<ShapeNode*, Util::Array<Ptr<SkinFragment> > >& skinFragments);

	/// writes an N3 model as a static model (not skinned)
	bool WriteStaticN3(const Util::Array<ShapeNode*>& meshes);
	
	/// updates a character n3 model
	void UpdateCharacterN3(Util::Array<ShapeNode*>& meshes,
						   const Skeleton* skeleton, 
						   const Ptr<IO::Stream>& modelStream, 
						   const Ptr<N3Writer>& modelWriter, 
						   const Util::Dictionary<ShapeNode*, Util::Array<Ptr<SkinFragment> > >& skinFragments);

	/// updates a static n3 model
	void UpdateStaticN3(const Util::Array<ShapeNode*>& meshes, 
						const Ptr<IO::Stream>& modelStream, 
						const Ptr<N3Writer>& modelWriter);

	/// reports an error, depending on what read state we are in, the error will be fatal or a warning
	void ReportError(const char* error, ...);

	/// takes a file name and path, checks if the file already exists, and returns the closest unique name for the new file (returns the same if no obstruction exists)
	const IO::URI GetUniquePath(const IO::URI& path);

	/// checks whether or not a file needs to be updated (constructs dest path from source)
	bool NeedsConversion(const Util::String& path);
	/// checks whether or not a file needs to be updated 
	bool NeedsConversion(const Util::String& src, const Util::String& dst);

	Ptr<FBXModelParser> modelParser;
	Ptr<FBXAnimationParser> animParser;
	Ptr<FBXSkeletonParser> skelParser;

	KFbxSdkManager* sdkManager;
	KFbxIOSettings* ioSettings;
	KFbxScene* scene;
	
	FbxNodeRegistry* nodeRegistry;

	Ptr<BatchAttributes> batchAttributes;
	ToolkitUtil::MeshBuilder* m_pMeshBuilder;
	float scaleFactor;	
	ExportMode exportMode;
	MeshFlag meshFlag;

}; 


//------------------------------------------------------------------------------
/**
*/
inline void 
FBXExporter::ReportError( const char* error, ... )
{
	va_list argList;
	va_start(argList, error);
	if (this->exportFlag == ExporterBase::File)
	{
		IO::Console::Instance()->Error(error, argList);
	}
	else
	{
		IO::Console::Instance()->Warning(error, argList);
	}
	va_end(argList);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FBXExporter::SetExportMode( const ExportMode& mode )
{
	this->exportMode = mode;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FBXExporter::SetMeshFlags( const MeshFlag& meshFlag )
{
	this->meshFlag = meshFlag;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------