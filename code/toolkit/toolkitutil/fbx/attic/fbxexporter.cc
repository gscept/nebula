//------------------------------------------------------------------------------
//  fbxparser.cc
//  (C) 2012 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbxexporter.h"
#include "io\uri.h"
#include "ToolkitUtil\animutil\animbuildersaver.h"
#include "ToolkitUtil\meshutil\meshbuildersaver.h"
#include "ToolkitUtil\n3util\n3writer.h"
#include "io\ioserver.h"
#include "ToolkitUtil\xmlmodelwriter.h"
#include "math\bbox.h"
#include "ToolkitUtil\n3util\n3xmlextractor.h"
#include "ToolkitUtil\n3util\n3xmlexporter.h"
#include "skinpartitioner.h"
#include "scenewriter.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::FBXExporter, 'FBXP', Base::ExporterBase);

using namespace Util;
using namespace IO;
using namespace Core;
using namespace Math;
//------------------------------------------------------------------------------
/**
*/
FBXExporter::FBXExporter() : 
	sdkManager(0)	
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FBXExporter::~FBXExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXExporter::Open()
{
	ExporterBase::Open();
	// Create the FBX SDK manager
	this->sdkManager = KFbxSdkManager::Create();
	this->ioSettings = KFbxIOSettings::Create(this->sdkManager, "Import settings");

	
	this->nodeRegistry = new FbxNodeRegistry();
	this->nodeRegistry->Open();

	this->modelParser = FBXModelParser::Create();
	this->animParser = FBXAnimationParser::Create();
	this->skelParser = FBXSkeletonParser::Create();

	this->modelParser->SetExporter(this);
	this->animParser->SetExporter(this);
	this->skelParser->SetExporter(this);

	this->sdkManager->SetIOSettings(this->ioSettings);

	this->modelParser->Setup(this->sdkManager);
	this->animParser->Setup(this->sdkManager);
	this->skelParser->Setup(this->sdkManager);

	this->batchAttributes = BatchAttributes::Create();
	this->batchAttributes->Open();
}


//------------------------------------------------------------------------------
/**
*/
void 
FBXExporter::Close()
{
	this->batchAttributes->Close();
	this->batchAttributes = 0;

	this->modelParser = 0;
	this->animParser = 0;
	this->skelParser = 0;

	ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXExporter::ExportList(const Util::Array<Util::String>& files)
{
	for(Util::Array<Util::String>::Iterator iter = files.Begin();iter != files.End();iter++)
	{
		Array<String> parts = iter->Tokenize("/");
		this->SetCategory(parts[0]);
		this->SetFile(parts[1]);
		//FIXME
		URI file("proj:work/gfxlib/" + *iter);
		ExportFile(file);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXExporter::ExportFile( const URI& fileName )
{
	n_assert(this->isOpen);
	IoServer* ioServer = IoServer::Instance();

	n_printf("-------------------Exporting: %s-------------------\n", (this->category + "/" + this->file).AsCharPtr());
	this->Progress((float)this->precision, "Exporting: " + fileName.GetHostAndLocalPath());

	// we want to see if the model file exists, because that's the only way to know if ALL the resources are old or new...
	if (!this->NeedsConversion(fileName.AsString()))
	{
		n_printf("Exported result is newer than original for: %s... Ignoring export\n", fileName.AsString().AsCharPtr());
		return;
	}

	// create scene and importer, sett progress callback
	this->scene = KFbxScene::Create(this->sdkManager, "Export scene");
	KFbxImporter* importer = KFbxImporter::Create(this->sdkManager, "Importer");
	importer->SetProgressCallback(this->progressCallback);

	String localPath = fileName.GetHostAndLocalPath();
	bool importStatus = importer->Initialize(localPath.AsCharPtr(), -1, this->ioSettings);
	if (importStatus)
	{
		importStatus = importer->Import(this->scene);
		if (!importStatus)
		{
			this->ReportError("Could not open file: %s for reading!\n", fileName.AsString());
			return;
		}
	}
	else
	{
		return;
	}

	
	KFbxSystemUnit systemUnit = scene->GetGlobalSettings().GetSystemUnit();
	this->scaleFactor = (float)systemUnit.GetScaleFactor();

	// make sure to rescale our scenes to 1.0. This will ensure a consistent result in Nebula
	if( this->scaleFactor != 1.0f )
	{
		KFbxSystemUnit wantedUnit(1.0f);
		wantedUnit.ConvertScene(scene);
	}

	this->nodeRegistry->Setup(this->scene);

	int numMeshes = this->scene->GetSrcObjectCount(FBX_TYPE(KFbxMesh));
	int numSkeletons = this->scene->GetSrcObjectCount(FBX_TYPE(KFbxSkeleton));
	for (int skelIndex = 0; skelIndex < numSkeletons; skelIndex++)
	{
		// filter root skeletons
		KFbxSkeleton* skeleton = this->scene->GetSrcObject(FBX_TYPE(KFbxSkeleton), skelIndex);
		if (!skeleton->IsSkeletonRoot())
		{
			numSkeletons--;
			skelIndex--;
		}
	}
	int numAnims = scene->GetSrcObjectCount(FBX_TYPE(KFbxAnimStack));
	int numResources = numMeshes + numSkeletons + numAnims;
	int increment = this->precision / numResources;

	this->skelParser->SetIncrement(increment);
	this->animParser->SetIncrement(increment);

	this->skelParser->SetScale(this->scaleFactor);
	this->animParser->SetScale(this->scaleFactor);

	this->modelParser->SetIncrement(increment);
	this->modelParser->SetScale(this->scaleFactor);

	if (BatchAttributes::Instance()->HasSplitter(category + "/" + file))
	{
		this->animParser->SetAnimSplitter(BatchAttributes::Instance()->GetSplitter(category + "/" + file));
	}

 	this->skelParser->Parse(this->scene);
 	Util::Array<Skeleton*> skeletons = this->skelParser->GetSkeletons();
	this->animParser->SetParseMode(FBXAnimationParser::Joints);
 	for (int skeletonIndex = 0; skeletonIndex < skeletons.Size(); skeletonIndex++)
 	{
 		AnimBuilder animBuilder;
 		this->animParser->SetSkeleton(skeletons[skeletonIndex]);
 		this->animParser->Parse(this->scene, &animBuilder);
		// if the skeleton has an animation resource
		if (animBuilder.CountKeys() > 0)
		{
			String animPath = "ani:" + this->category + "/" + this->file + ".nax3";
			if (skeletonIndex > 0)
			{
				animPath = "ani:" + this->category + "/" + this->file + "_" + skeletons[skeletonIndex]->root->name + ".nax3";
			}		
			AnimBuilderSaver::SaveNax3(animPath, animBuilder, platform);
		}
		else
		{
			// if there are no animations assigned with the skeleton, then we will treat our scene as a static mesh by removing the skeleton
			skeletons.EraseIndex(skeletonIndex);
		}	
 	}

	this->modelParser->SetSkeletons(skeletons);
	this->modelParser->Parse(this->scene);

	Util::Array<ShapeNode*> meshes = this->modelParser->GetMeshes();
	Util::Array<ShapeNode*> skinnedMeshes = this->modelParser->GetSkinnedMeshes();

	
	// only get one mesh if it exists (meshes share the same mesh builder)
	for (int meshIndex = 0; meshIndex < meshes.Size(); meshIndex++)
	{
		ShapeNode* mesh = meshes[meshIndex];

		// get unique name for mesh
		String meshName = this->file + "_s_" + String::FromInt(meshIndex);
		String meshDir = "msh:" + this->category;

		String meshResource = meshDir + "/" + this->file + "_s_0.nvx2";
		mesh->resource = meshResource;

		// from this point, we don't want to do anything else
		if (meshIndex > 0)
		{
			continue;
		}

		if (!ioServer->DirectoryExists(meshDir))
		{
			n_assert(ioServer->CreateDirectory(meshDir));
		}


		// export static mesh
		bool res = MeshBuilderSaver::SaveNvx2(IO::URI(meshDir + "/" + meshName + ".nvx2"), *mesh->meshSource, platform);
		n_assert(res);
	}

	// in case we get fragments, we need them saved
	Util::Dictionary<ShapeNode*, Util::Array<Ptr<SkinFragment> > > fragments;

	for (int skinIndex = 0; skinIndex < skinnedMeshes.Size(); skinIndex++)
	{
		ShapeNode* mesh = skinnedMeshes[skinIndex];

		// get unique name for skin
		String skinsDir = "msh:" + this->category + "/" + this->file + "/skins";
		String skinName = this->file + "_sk_" + String::FromInt(skinIndex);
		mesh->resource = skinsDir + "/" + skinName + ".nvx2";

		MeshBuilder partitionedMesh = *mesh->meshSource;
		if (skeletons.Size() > 0)
		{
			Util::Array<Ptr<SkinFragment> > meshFragments;
			Ptr<SkinPartitioner> partitioner = SkinPartitioner::Create();
			
			// fragment the skin. This is necessary because we need to split the skin in groups of meshes.
			// Every mesh is affected by 72 joints (the shader only takes 72 joints at a time)
			partitioner->FragmentSkin(*mesh->meshSource, partitionedMesh, meshFragments);
			fragments.Add(mesh, meshFragments);
		}


		// create directory if it doesn't exist
		if (!ioServer->DirectoryExists(skinsDir))
		{
			n_assert(ioServer->CreateDirectory(skinsDir));
		}

		// export fragmented skin
		bool res = MeshBuilderSaver::SaveNvx2(IO::URI(skinsDir + "/" + skinName  + ".nvx2"), partitionedMesh, platform);
		n_assert(res);
	}


	// only get new model file if we are exporting from maya
	if (this->exportFlag == File)
	{
		if (!this->WriteCharacterN3(skinnedMeshes, skeletons, fragments))
		{
			// in case we have skins which doesn't have skeletons!
			meshes.AppendArray(skinnedMeshes);
			this->WriteStaticN3(meshes);
		}
	}
	
	

	n_printf("-------------------Done exporting!-------------------\n");

	this->modelParser->Cleanup();
	this->skelParser->Cleanup();

	importer->ContentUnload();
	this->scene->ContentUnload();	

	importer->Destroy();

	this->scene->Destroy(true);
	this->scene = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXExporter::ExportDir( const Util::String& dirName )
{
	String categoryDir = "proj:work/gfxlib/" + dirName;
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
FBXExporter::ExportAll()
{
	String workDir = "proj:work/gfxlib";
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
	Writes the N3 model file for all static objects in the scene
*/
bool 
FBXExporter::WriteStaticN3(const Util::Array<ShapeNode*>& meshes)
{
	n_assert(this->isOpen);

	bool retVal = false;
	if (meshes.Size() == 0)
	{
		return retVal;
	}
	else
	{
		retVal = true;
	}

	Ptr<IoServer> ioServer = IoServer::Instance();

	String modelFile = "proj:work/models/" + this->category + "/" + this->file + ".xml";
	if (!ioServer->DirectoryExists("proj:work/models/"+category))
	{
		n_assert(ioServer->CreateDirectory("proj:work/models/"+category));
	}
	Ptr<Stream> modelStream = ioServer->CreateStream(modelFile);
	bool update = false;
	if (ioServer->FileExists(modelFile))
	{
		modelStream->SetAccessMode(Stream::ReadWriteAccess);
		update = true;
	}
	else
	{
		modelStream->SetAccessMode(Stream::WriteAccess);
	}

	if (modelStream->Open())
	{
		update &= !modelStream->Eof();
	}
	else
	{
		this->ReportError("Could not open model file: %s for writing!\n", modelFile);
		modelStream->Close();
		return false;
	}

	// makes sure not to update model file is file is empty
	Ptr<N3Writer> modelWriter = N3Writer::Create();
	modelWriter->SetModelWriter(XmlModelWriter::Create());
	modelWriter->Open(modelStream);

	if (update)
	{
		this->UpdateStaticN3(meshes, modelStream, modelWriter);
	}
	else
	{
		bbox sceneBox;
		sceneBox.begin_extend();
		for (int meshIndex = 0; meshIndex < meshes.Size(); meshIndex++)
		{
			sceneBox.extend(meshes[meshIndex]->boundingBox);
		}
		sceneBox.end_extend();

		modelWriter->BeginModel(this->category + "/" + this->file);
		modelWriter->BeginRoot(sceneBox);

		Ptr<SceneWriter> sceneWriter = SceneWriter::Create();
		sceneWriter->SetCategory(this->category);
		sceneWriter->SetFile(this->file);
		sceneWriter->SetModelWriter(modelWriter);
		sceneWriter->SetShadingMode(SceneWriter::Multiple);
		sceneWriter->SetWriterMode(SceneWriter::Static);
		sceneWriter->SetMeshes(meshes);
		sceneWriter->WriteScene();

		modelWriter->EndRoot();
		modelWriter->EndModel();
	}
	

	modelWriter->Close();
	modelStream->Close();

	Ptr<N3XMLExporter> converter = N3XMLExporter::Create();
	String fileName = modelFile.ExtractFileName();
	fileName.StripFileExtension();
	converter->Open();
	converter->ExportFile(modelFile);
	converter->Close();

	return retVal;
}

//------------------------------------------------------------------------------
/**
	Updates a model file instead of rewriting it (only updates primitive groups, the mesh resource and bounding boxes)
*/
void 
FBXExporter::UpdateStaticN3( const Util::Array<ShapeNode*>& meshes, const Ptr<IO::Stream>& modelStream, const Ptr<N3Writer>& modelWriter )
{

	Ptr<N3XMLExtractor> extractor = N3XMLExtractor::Create();
	extractor->SetStream(modelStream);
	extractor->Open();
	Array<String> nodes;
	Array<String> materials;
	Array<State> states;
	extractor->ExtractNodes(nodes);
	// remove root node
	IndexT rootIndex = nodes.FindIndex("root");
	if (rootIndex != InvalidIndex && nodes.Size() > rootIndex)
	{
		nodes.EraseIndex(rootIndex);
	}
	for (int nodeIndex = 0; nodeIndex < nodes.Size(); nodeIndex++)
	{
		String material;
		State state;
		extractor->ExtractMaterial(nodes[nodeIndex], material);
		extractor->ExtractState(nodes[nodeIndex], state);
		if (!material.IsEmpty())
		{
			materials.Append(material);
		}
		if (!state.textures.IsEmpty() || !state.variables.IsEmpty())
		{
			states.Append(state);
		}
	}
	extractor->Close();

	modelStream->Close();
	modelStream->SetAccessMode(Stream::WriteAccess);
	modelStream->Open();

	bbox sceneBox;
	sceneBox.begin_extend();
	for (int meshIndex = 0; meshIndex < meshes.Size(); meshIndex++)
	{
		sceneBox.extend(meshes[meshIndex]->boundingBox);
	}
	sceneBox.end_extend();


	modelWriter->BeginModel(this->category + "/" + this->file);
	modelWriter->BeginRoot(sceneBox);

	// creates and sets up a scene writer which will write our scene
	Ptr<SceneWriter> sceneWriter = SceneWriter::Create();
	sceneWriter->SetCategory(this->category);
	sceneWriter->SetFile(this->file);
	sceneWriter->SetModelWriter(modelWriter);
	sceneWriter->SetShadingMode(SceneWriter::Multiple);
	sceneWriter->SetWriterMode(SceneWriter::Static);
	sceneWriter->SetMaterials(materials);
	sceneWriter->SetStates(states);
	sceneWriter->SetMeshes(meshes);
	sceneWriter->WriteScene();

	modelWriter->EndRoot();
	modelWriter->EndModel();
}

//------------------------------------------------------------------------------
/**
	Writes a character to an N3 file, note that the list of meshes will decrease for every character.
	This is because we don't want any skinned meshes left in the list for when we go to write out static meshes.
	Also, every skeleton will have their own model files (only one skeleton can be present per model)
*/
bool 
FBXExporter::WriteCharacterN3( Util::Array<ShapeNode*>& meshes, 
							 const Util::Array<Skeleton*>& skeletons, 
							 const Util::Dictionary<ShapeNode*, Util::Array<Ptr<SkinFragment> > >& skinFragments )
{
	n_assert(this->isOpen);
	Ptr<IoServer> ioServer = IoServer::Instance();
	bool retVal = false;

	if (skeletons.Size() == 0)
	{
		return retVal;
	}
	else
	{
		retVal = true;
	}

	// make sure the folder exists
	if (!ioServer->DirectoryExists("proj:work/models/"+category))
	{
		n_assert(ioServer->CreateDirectory("proj:work/models/"+category));
	}

	for (int skelIndex = 0; skelIndex < skeletons.Size(); skelIndex++)
	{
		Skeleton* skeleton = skeletons[skelIndex];
		String modelFile;

		// if we have several skeletons, we want to split them into separate characters
		if (skelIndex == 0)
		{
			modelFile = "proj:work/models/" + this->category + "/" + this->file + ".xml";
		}
		else
		{
			modelFile = "proj:work/models/" + this->category + "/" + this->file + "_" + skeleton->root->fbxNode->GetName() + ".xml";
		}

		Ptr<Stream> modelStream = ioServer->CreateStream(modelFile);
		bool update = false;
		if (ioServer->FileExists(modelFile))
		{
			modelStream->SetAccessMode(Stream::ReadWriteAccess);
			update = true;
		}
		else
		{
			modelStream->SetAccessMode(Stream::WriteAccess);
		}

		if (modelStream->Open())
		{
			// makes sure not to update model file is file is empty
			update &= !modelStream->Eof();
		}
		else
		{
			this->ReportError("Could not open model file: %s for writing!\n", modelFile);
			modelStream->Close();
			return false;
		}

		Ptr<N3Writer> modelWriter = N3Writer::Create();
		modelWriter->SetModelWriter(XmlModelWriter::Create());
		modelWriter->Open(modelStream);

		if (update)
		{
			// we don't need to entirely rewrite everything (keep material and shader variables)
			this->UpdateCharacterN3(meshes, skeleton, modelStream, modelWriter, skinFragments);
		}
		else
		{
			String skinDir = "msh:" + this->category + "/" + this->file + "/skins";
			
			Util::Array<Util::String> skins;		
			for (int skinIndex = 0; skinIndex < skeleton->skinnedMeshes.Size(); skinIndex++)
			{
				ShapeNode* skinnedMesh = skeleton->skinnedMeshes[skinIndex];
				skins.Append(skinnedMesh->name);
			}

			if (skins.Size() == 0)
			{
				n_warning("No skin(s) found for skeleton: %s! Skeleton is ignored\n", skeleton->root->fbxNode->GetName());
				modelWriter->Close();
				continue;
			}

			// create skin list
			Util::Array<Skinlist> skinLists;
			Skinlist skinList;
			skinList.name = this->file;
			skinList.skins = skins;
			skinLists.Append(skinList);

			Util::Array<Joint> modelJoints;
			for (int jointIndex = 0; jointIndex < skeleton->joints.Size(); jointIndex++)
			{
				modelJoints.Append(skeleton->joints[jointIndex]->ConvertToModelJoint());
			}

			String animPath = "ani:" + this->category + "/" + this->file + ".nax3";
			// if this is not our first skeleton, make sure the other skeleton gets the correct animation resource
			if (skelIndex > 0)
			{
				animPath = "ani:" + this->category + "/" + this->file +  + "_" + skeleton->root->name + ".nax3";
			}

			modelWriter->BeginModel(this->category + "/" + this->file);
			modelWriter->BeginCharacter(this->file, skinLists, modelJoints, animPath);

			// create a scene writer, set it up, and run it
			Ptr<SceneWriter> sceneWriter = SceneWriter::Create();
			sceneWriter->SetCategory(this->category);
			sceneWriter->SetFile(this->file);
			sceneWriter->SetModelWriter(modelWriter);
			sceneWriter->SetShadingMode(SceneWriter::Multiple);
			sceneWriter->SetWriterMode(SceneWriter::Skinned);
			sceneWriter->SetFragments(skinFragments);
			sceneWriter->SetMeshes(meshes);
			sceneWriter->WriteScene();

			// clear the meshes to avoid writing the mesh as static.
			meshes.Clear();

			modelWriter->EndCharacter();
			modelWriter->EndModel();

		}
		modelWriter->Close();
		modelStream->Close();

		Ptr<N3XMLExporter> converter = N3XMLExporter::Create();
		String fileName = modelFile.ExtractFileName();
		fileName.StripFileExtension();
		converter->Open();
		converter->ExportFile(modelFile);
		converter->Close();
	}
	return retVal;
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXExporter::UpdateCharacterN3( Util::Array<ShapeNode*>& meshes, 
							  const Skeleton* skeleton, 
							  const Ptr<IO::Stream>& modelStream, 
							  const Ptr<N3Writer>& modelWriter, 
							  const Util::Dictionary<ShapeNode*, Util::Array<Ptr<SkinFragment> > >& skinFragments )
{

	n_assert(this->isOpen);
	Ptr<IoServer> ioServer = IoServer::Instance();
	bool retVal = false;

	Ptr<N3XMLExtractor> extractor = N3XMLExtractor::Create();
	extractor->SetStream(modelStream);
	extractor->Open();
	Array<String> nodes;
	Array<String> materials;
	Array<State> states;
	extractor->ExtractNodes(nodes);

	// we want to remove our character root node because it will be rewritten
	IndexT rootIndex = nodes.FindIndex(this->file);
	if (rootIndex != InvalidIndex && nodes.Size() > rootIndex)
	{
		nodes.EraseIndex(rootIndex);
	}
	for (int nodeIndex = 0; nodeIndex < nodes.Size(); nodeIndex++)
	{
		String node = nodes[nodeIndex];
		String material;
		State state;
		extractor->ExtractMaterial(node, material);
		extractor->ExtractState(node, state);
		if (!material.IsEmpty())
		{
			materials.Append(material);
		}
		if (!state.variables.IsEmpty() || !state.textures.IsEmpty())
		{
			states.Append(state);
		}
	}
	extractor->Close();

	modelStream->Close();
	modelStream->SetAccessMode(Stream::WriteAccess); 
	modelStream->Open();

	Util::Array<Util::String> skins;		
	for (int skinIndex = 0; skinIndex < skeleton->skinnedMeshes.Size(); skinIndex++)
	{
		ShapeNode* skinnedMesh = skeleton->skinnedMeshes[skinIndex];
		skins.Append(skinnedMesh->name);
	}

	if (skins.Size() == 0)
	{
		n_warning("No skin(s) found for skeleton: %s! Skeleton is ignored\n", skeleton->root->fbxNode->GetName());
		return;
	}

	// create skin list
	Util::Array<Skinlist> skinLists;
	Skinlist skinList;
	skinList.name = this->file;
	skinList.skins = skins;
	skinLists.Append(skinList);

	Util::Array<Joint> modelJoints;

	//FIXME: indices are not split by fragment yet
	for (int jointIndex = 0; jointIndex < skeleton->joints.Size(); jointIndex++)
	{
		modelJoints.Append(skeleton->joints[jointIndex]->ConvertToModelJoint());
	}

	String animPath = "ani:" + this->category + "/" + this->file + ".nax3";
	modelWriter->BeginModel(this->category + "/" + this->file);
	modelWriter->BeginCharacter(this->file, skinLists, modelJoints, animPath);

	// create a scene writer, set it up, and run it
	Ptr<SceneWriter> sceneWriter = SceneWriter::Create();
	sceneWriter->SetCategory(this->category);
	sceneWriter->SetFile(this->file);
	sceneWriter->SetModelWriter(modelWriter);
	sceneWriter->SetShadingMode(SceneWriter::Multiple);
	sceneWriter->SetWriterMode(SceneWriter::Skinned);
	sceneWriter->SetFragments(skinFragments);
	sceneWriter->SetMaterials(materials);
	sceneWriter->SetStates(states);
	sceneWriter->SetMeshes(meshes);
	sceneWriter->WriteScene();

	// clear the meshes to avoid writing the mesh as static.
	meshes.Clear();

	// finish up the character
	modelWriter->EndCharacter();
	modelWriter->EndModel();

}

//------------------------------------------------------------------------------
/**
	Probes the file system to see if the path exists, and returns a new path if it does
*/
const IO::URI
FBXExporter::GetUniquePath( const IO::URI& path )
{
	IoServer* ioServer = IoServer::Instance();
	IO::URI returnPath = path;
	int idCounter = 0;
	while (ioServer->FileExists(returnPath))
	{
		String fileAsString = path.AsString().ExtractFileName();
		String extension = fileAsString.GetFileExtension();
		fileAsString.StripFileExtension();
		
		Array<String> segments = fileAsString.Tokenize("_");
		idCounter = segments.Back().AsInt()+1;
		fileAsString.Clear();
		segments.Erase(segments.End()-1);
		for (int segmentIndex = 0; segmentIndex < segments.Size(); segmentIndex++)
		{
			fileAsString += segments[segmentIndex];
			if (segmentIndex != segments.Size())
			{
				fileAsString += "_";
			}
		}
		fileAsString += String::FromInt(idCounter);
		fileAsString += extension;
		returnPath.SetFragment(fileAsString);
		
	}
	return returnPath;
}

//------------------------------------------------------------------------------
/**
*/
bool 
FBXExporter::NeedsConversion( const Util::String& path )
{
	String category = path.ExtractLastDirName();
	String file = path.ExtractFileName();
	file.StripFileExtension();

	String dst = "mdl:" + category + "/" + file + ".n3";
	return this->NeedsConversion(path, dst);
}

//------------------------------------------------------------------------------
/**
*/
bool 
FBXExporter::NeedsConversion( const Util::String& src, const Util::String& dst )
{
	if (this->force)
	{
		return true;
	}

	IoServer* ioServer = IoServer::Instance();
	if (ioServer->FileExists(dst))
	{
		FileTime srcFileTime = ioServer->GetFileWriteTime(src);
		FileTime dstFileTime = ioServer->GetFileWriteTime(dst);
		if (dstFileTime > srcFileTime)
		{
			return false;
		}
	}

	// fallthrough
	return true;
}


} // namespace ToolkitUtil