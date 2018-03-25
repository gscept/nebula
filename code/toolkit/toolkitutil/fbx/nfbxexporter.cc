//------------------------------------------------------------------------------
//  fbxexporter.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbx/nfbxexporter.h"
#include "io/ioserver.h"
#include "util/array.h"
#include "meshutil/meshbuildersaver.h"
#include "animutil/animbuildersaver.h"
#include "modelutil/modeldatabase.h"
#include "modelutil/modelattributes.h"

using namespace Util;
using namespace IO;
using namespace ToolkitUtil;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::NFbxExporter, 'FBXE', Base::ExporterBase);


FbxManager* NFbxExporter::sdkManager = NULL;
FbxIOSettings* NFbxExporter::ioSettings = NULL;
Threading::CriticalSection NFbxExporter::cs;
//------------------------------------------------------------------------------
/**
*/
NFbxExporter::NFbxExporter() : 	
	scene(0),
	scaleFactor(1.0f),
	progressFbxCallback(0),
	exportMode(Static),
	exportFlags(ToolkitUtil::FlipUVs)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
NFbxExporter::~NFbxExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxExporter::Open()
{
	ExporterBase::Open();

	cs.Enter();
	if (!this->sdkManager)
	{
		// Create the FBX SDK manager
		this->sdkManager = FbxManager::Create();
		this->ioSettings = FbxIOSettings::Create(this->sdkManager, "Import settings");
	}
	cs.Leave();
	

	this->scene = NFbxScene::Create();
	this->scene->Open();
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxExporter::Close()
{
	this->scene->Close();
	this->scene = 0;

	ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxExporter::ExportDir( const Util::String& dirName )
{
	String categoryDir = "src:assets/" + dirName;
	Array<String> files = IoServer::Instance()->ListFiles(categoryDir, "*.fbx");
	for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
	{
		String file = categoryDir + "/" + files[fileIndex];
		this->SetFile(files[fileIndex]);
		this->ExportFile(file);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxExporter::ExportAll()
{
	String workDir = "src:assets";
	Array<String> directories = IoServer::Instance()->ListDirectories(workDir, "*");
	for (int directoryIndex = 0; directoryIndex < directories.Size(); directoryIndex++)
	{
		String category = workDir + "/" + directories[directoryIndex];
		this->SetCategory(directories[directoryIndex]);
		Array<String> files = IoServer::Instance()->ListFiles(category, "*.fbx");
		for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
		{
			String file = category + "/" + files[fileIndex];
			this->SetFile(files[fileIndex]);
			this->ExportFile(file);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxExporter::ExportList(const Util::Array<Util::String>& files)
{
    for (Array<String>::Iterator iter = files.Begin(); iter != files.End(); iter++)
	{
		Array<String> parts = iter->Tokenize("/");
		this->SetCategory(parts[0]);
		this->SetFile(parts[1]);

		//FIXME
		URI file("src:assets/" + *iter);
		this->ExportFile(file);
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
NFbxExporter::StartExport(const IO::URI& file)
{
	n_assert(this->isOpen);
	IoServer* ioServer = IoServer::Instance();

	String localPath = file.GetHostAndLocalPath();
	this->file = localPath;
	String exportType = this->exportMode == ToolkitUtil::Static ? "standard" : "character";
	n_printf("Exporting FBX as %s:\n        %s\n", exportType.AsCharPtr(), localPath.AsCharPtr());

	// we want to see if the model file exists, because that's the only way to know if ALL the resources are old or new...
	if (!this->NeedsConversion(localPath))
	{
		n_printf("    [File has not changed, ignoring export]\n\n", localPath.AsCharPtr());
		return false;
	}

	this->fbxScene = FbxScene::Create(this->sdkManager, "Export scene");
	FbxImporter* importer = FbxImporter::Create(this->sdkManager, "Importer");

	bool importStatus = importer->Initialize(localPath.AsCharPtr(), -1, NULL);
	importer->SetProgressCallback(this->progressFbxCallback);
	if (importStatus)
	{
		importStatus = importer->Import(this->fbxScene);
		if (!importStatus)
		{
			n_error("    [Could not open %s for reading! Something went terribly wrong!]\n\n", localPath.AsCharPtr());
			this->SetHasErrors(true);
			return false;
		}
	}
	else
	{
		n_error("    [Could not initialize %s for reading! Something went terribly wrong!]\n\n", localPath.AsCharPtr());
		this->SetHasErrors(true);
		return false;
	}

	importer->Destroy();
	importer = 0;

	// deduct file name from URL
	String fileName = localPath.ExtractFileName();
	fileName.StripFileExtension();
	String catName = localPath.ExtractLastDirName();

	// set name of scene
	this->scene->SetName(fileName);
	this->scene->SetCategory(catName);
	this->scene->Setup(this->fbxScene, this->exportFlags, this->exportMode, this->scaleFactor);

	// if we intend to export this as a skeletal mesh, make sure to fragment the skins into pairs of 72 joints
	if (this->exportMode & Skeletal)
	{
		this->scene->FragmentSkins();
	}

	// flatten scene
	this->scene->Flatten();

	// clear list of exported meshes
	this->exportedMeshes.Clear();

	// get category
	String category = localPath.ExtractLastDirName();

	// get mesh
	MeshBuilder* mesh = scene->GetMesh();

	// format file destination
	String destinationFile;
	destinationFile.Format("msh:%s/%s.nvx2", catName.AsCharPtr(), fileName.AsCharPtr());

	// save mesh to file
	if (false == MeshBuilderSaver::SaveNvx2(URI(destinationFile), *mesh, this->platform))
	{
		n_error("Failed to save Nvx2 file: %s\n", destinationFile.AsCharPtr());
	}		

	// print info
	n_printf("[Generated graphics mesh: %s]\n", destinationFile.AsCharPtr());

	// get physics mesh
	MeshBuilder* physicsMesh = scene->GetPhysicsMesh();

	// only save physics mesh if it exists
	if (physicsMesh->GetNumTriangles() > 0)
	{
		// reformat destination
		destinationFile.Format("msh:%s/%s_ph.nvx2", catName.AsCharPtr(), fileName.AsCharPtr());

		// save mesh
		if (false == MeshBuilderSaver::SaveNvx2(URI(destinationFile), *physicsMesh, this->platform))
		{
			n_error("Failed to save physics Nvx2 file: %s\n", destinationFile.AsCharPtr());
		}		

		// print info
		n_printf("[Generated physics mesh: %s]\n", destinationFile.AsCharPtr());
	}
	

	// get animations from scene
	Array<Ptr<NFbxJointNode> > skeletonRoots = this->scene->GetSkeletonRoots();

	IndexT rootIndex;
	for (rootIndex = 0; rootIndex < skeletonRoots.Size(); rootIndex++)
	{
		Ptr<NFbxJointNode> skeletonRoot = skeletonRoots[rootIndex];

		// get animation
		AnimBuilder anim = skeletonRoot->GetAnimation();

		// now we must format the animation name
		String animationName;
		animationName.Format("%s", fileName.AsCharPtr());

		// format destination
		String destinationFile;
		destinationFile.Format("ani:%s/%s.nax3", category.AsCharPtr(), animationName.AsCharPtr());

		// now fix animation stuff	
		anim.FixInvalidKeyValues();
		anim.FixInactiveCurveStaticKeyValues();
		anim.FixAnimCurveFirstKeyIndices();
		anim.BuildVelocityCurves();

		// now save actual animation
		if (false == AnimBuilderSaver::SaveNax3(URI(destinationFile), anim, this->platform))
		{
			n_error("Failed to save animation file file: %s\n", destinationFile.AsCharPtr());
		}			
		n_printf("[Generated animation: %s]\n", destinationFile.AsCharPtr());
	}

	n_printf("\n");

	// create scene writer
	this->sceneWriter = NFbxSceneWriter::Create();
	this->sceneWriter->SetScene(this->scene);
	this->sceneWriter->SetPlatform(this->platform);

	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxExporter::EndExport()
{
	n_assert(this->IsOpen());

	// format base path
	String basePath;
	basePath.Format("src:assets/%s/", this->file.ExtractLastDirName().AsCharPtr());

	// generate models
 	this->sceneWriter->GenerateModels(basePath, this->exportFlags, this->exportMode);	
 	this->sceneWriter = 0;

	// cleanup data
	this->scene->Cleanup();	
	this->fbxScene->Clear();
	this->fbxScene = 0;
}

//------------------------------------------------------------------------------
/**
	Basically just calls start and end in sequence
*/
void 
NFbxExporter::ExportFile( const IO::URI& file )
{
	// get directory and file
	String fileString = file.GetHostAndLocalPath();
	String dir = fileString.ExtractLastDirName();
	String fbx = fileString.ExtractFileName();
	fbx.StripFileExtension();

	// get model attributes
	const Ptr<ModelAttributes>& attrs = ModelDatabase::Instance()->LookupAttributes(dir + "/" + fbx);

	this->SetExportFlags(attrs->GetExportFlags());
	this->SetExportMode(attrs->GetExportMode());
	this->SetScale(attrs->GetScale());

	if (this->StartExport(file))
	{
		this->EndExport();
	}	
}

//------------------------------------------------------------------------------
/**
	Check if we need to export. This are the dependencies we have:

	.attributes
		n3			(model resource)
		nax3		(animation resource)
	.constants
		n3			(model resource, dictates which nodes should be picked from the attributes)
		nvx2		(mesh resource)
	.physics
		np3			(physics resource)
		_ph.nvx2	(optional physics mesh resource)
*/
bool
NFbxExporter::NeedsConversion(const Util::String& path)
{
	String category = path.ExtractLastDirName();
	String file = path.ExtractFileName();
	file.StripFileExtension();

    // check both if FBX is newer than .n3
	String model = "mdl:" + category + "/" + file + ".n3";
	String physModel = "phys:" + category + "/" + file + ".np3";
	String mesh = "msh:" + category + "/" + file + ".nvx2";
	String physMesh = "msh:" + category + "/" + file + "_ph.nvx2";
	String animation = "ani:" + category + "/" + file + ".nax3";
	String constants = "src:assets/" + category + "/" + file + ".constants";
	String attributes = "src:assets/" + category + "/" + file + ".attributes";
	String physics = "src:assets/" + category + "/" + file + ".physics";

	// check if fbx is newer than model
    bool fbxNewer = ExporterBase::NeedsConversion(path, model);

    // and if the .constants is older than the fbx
	bool constantsNewer = ExporterBase::NeedsConversion(constants, model);

	// and if the .attributes is older than the n3 (attributes controls both model, and animation resource)
	bool attributesNewer = ExporterBase::NeedsConversion(attributes, model);

	// ...and if the .physics is older than the n3
	bool physicsNewer = ExporterBase::NeedsConversion(physics, physModel);

	// ...if the mesh is newer
	bool meshNewer = ExporterBase::NeedsConversion(path, mesh);

	// check if physics settings were changed. no way to tell if we have a new physics mesh in it, so we just export it anyway
	bool physicsMeshNewer = ExporterBase::NeedsConversion(physics, mesh);

    // return true if either is true
	return fbxNewer || constantsNewer || attributesNewer || physicsNewer || meshNewer || physicsMeshNewer;
}

} // namespace ToolkitUtil