//------------------------------------------------------------------------------
//  fbxscenewriter.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "nfbxscenewriter.h"
#include "xmlmodelwriter.h"
#include "io/ioserver.h"
#include "n3util/n3xmlexporter.h"
#include "modelutil/modeldatabase.h"
#include "modelutil/modelconstants.h"
#include "modelutil/modelbuilder.h"
#include "io/memorystream.h"
#include "util/crc.h"

using namespace IO;
using namespace Util;
using namespace ToolkitUtil;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::NFbxSceneWriter, 'FBSW', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
NFbxSceneWriter::NFbxSceneWriter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
NFbxSceneWriter::~NFbxSceneWriter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
	Generates model files, the path provided should be the path to the folder
*/
void 
NFbxSceneWriter::GenerateModels( const String& basePath, const ExportFlags& flags, const ExportMode& mode)
{
	n_assert(this->scene.isvalid());

	// create N3 model writer and set it up
	Ptr<N3Writer> n3Writer = N3Writer::Create();
	Ptr<XmlModelWriter> modelWriter = XmlModelWriter::Create();
	n3Writer->SetModelWriter(modelWriter.upcast<ModelWriter>());

	// make sure category directory exists
	IoServer::Instance()->CreateDirectory(basePath);

	const Array<Ptr<NFbxMeshNode> >& meshes = this->scene->GetMeshNodes();
	if (mode == Static)
	{
		// create merged model, will basically go through every node and add them into ONE .xml file
		this->CreateStaticModel(n3Writer, meshes, basePath + this->scene->GetName() + ".xml");
	}
	else
	{
		// create skeletal model
		this->CreateSkeletalModel(n3Writer, meshes, basePath + this->scene->GetName() + ".xml");
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxSceneWriter::CreateStaticModel( const Ptr<ToolkitUtil::N3Writer>& modelWriter, const Util::Array<Ptr<NFbxMeshNode> >& meshes, const Util::String& path)
{
	// extract file path from export path
	String file = path.ExtractFileName();
	file.StripFileExtension();
	String category = path.ExtractLastDirName();

	// merge file and category into a single name
	String fileCat = category + "/" + this->scene->GetName();

	// get constant model set
	Ptr<ModelConstants> constants = ModelDatabase::Instance()->LookupConstants(fileCat);

	// get model attributes
	Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(fileCat);

	// get physics
	Ptr<ModelPhysics> physics = ModelDatabase::Instance()->LookupPhysics(fileCat);
	
	if(scene->GetPhysicsMesh()->GetNumTriangles() > 0)
	{
		Util::String phname;
		phname.Format("phymsh:%s_ph.nvx2", fileCat.AsCharPtr());
		physics->SetPhysicsMesh(phname);
	}

	// clear constants
	constants->Clear();

	// loop through bounding boxes and get our scene bounding box
	Math::bbox globalBox;
	const Util::Array<Ptr<NFbxMeshNode> >& meshNodes = meshes;
	IndexT meshIndex;
	globalBox.begin_extend();
	for (meshIndex = 0; meshIndex < meshNodes.Size(); meshIndex++)
	{
		Ptr<NFbxMeshNode> mesh = meshNodes[meshIndex];
		globalBox.extend(mesh->GetBoundingBox());
	}
	globalBox.end_extend();

	// set global box for constants
	constants->SetGlobalBoundingBox(globalBox);

	IndexT i;
	for (i = 0; i < meshes.Size(); i++)
	{
		// get mesh node
		const Ptr<NFbxMeshNode>& mesh = meshes[i];

		// create full node path
		String nodePath;
		nodePath.Format("root/%s", mesh->GetName().AsCharPtr());

		// create mesh name
		String meshPath;
		meshPath.Format("msh:%s/%s.nvx2", category.AsCharPtr(), scene->GetName().AsCharPtr());

		// create and add a shape node
		ModelConstants::ShapeNode shapeNode;
		shapeNode.mesh = meshPath;
		shapeNode.path = nodePath;
		shapeNode.useLOD = mesh->HasLOD();
		if (shapeNode.useLOD)
		{
			shapeNode.LODMax = mesh->GetLODMaxDistance();
			shapeNode.LODMin = mesh->GetLODMinDistance();
		}
		shapeNode.transform.position = mesh->GetInitialPosition();
		shapeNode.transform.rotation = mesh->GetInitialRotation();
		shapeNode.transform.scale = mesh->GetInitialScale();
		shapeNode.boundingBox = mesh->GetBoundingBox();
		shapeNode.name = mesh->GetName();
		shapeNode.type = scene->GetSceneFeatureString();
		shapeNode.primitiveGroupIndex = mesh->GetGroupId();

		// add to constants
		constants->AddShapeNode(mesh->GetName(), shapeNode);

		// get state from attributes
		State state;

		// retrieve state if exists
		if (attributes->HasState(nodePath))
		{
			state = attributes->GetState(nodePath);
		}

		// set material of state
		if (!state.material.IsValid())
		{
			state.material = "sur:system/placeholder";
		}		

		// search for diffuse map and set it to white if it isn't set
		Texture diffTex;
		diffTex.textureName = "DiffuseMap";
		diffTex.textureResource = "tex:system/placeholder";

		Texture normalTex;
		normalTex.textureName = "NormalMap";
		normalTex.textureResource = "tex:system/nobump";

		// if the diffuse map is not set, set it to white
		if (state.textures.FindIndex(diffTex) == InvalidIndex)
		{
			state.textures.Append(diffTex);
		}

		// do the same for the normal map
		if (state.textures.FindIndex(normalTex) == InvalidIndex)
		{
			state.textures.Append(normalTex);
		}

		// set state for attributes
		attributes->SetState(nodePath, state);
	}

	const Util::Array<Ptr<NFbxMeshNode> >& physicsNodes = scene->GetPhysicsNodes();
	for (i = 0; i < physicsNodes.Size(); i++)
	{
		// get mesh
		const Ptr<NFbxMeshNode>& mesh = physicsNodes[i];

		// create full node path
		String nodePath;
		nodePath.Format("root/%s", mesh->GetName().AsCharPtr());

		// create mesh name
		String meshPath;
		meshPath.Format("phymsh:%s/%s_ph.nvx2", category.AsCharPtr(), scene->GetName().AsCharPtr());

		// create physics node
		ModelConstants::PhysicsNode node;
		node.mesh = meshPath;
		node.path = nodePath;
		node.transform.position = mesh->GetInitialPosition();
		node.transform.rotation = mesh->GetInitialRotation();
		node.transform.scale = mesh->GetInitialScale();
		node.primitiveGroupIndex = mesh->GetGroupId();
		node.name = mesh->GetName();

		// add to constants
		constants->AddPhysicsNode(mesh->GetName(), node);
	}

	String constantsFile;
	constantsFile.Format("src:assets/%s/%s.constants", category.AsCharPtr(), file.AsCharPtr());

	// update
	this->UpdateConstants(constantsFile, constants);

	// format attributes file
	String attributesFile;
	attributesFile.Format("src:assets/%s/%s.attributes", category.AsCharPtr(), file.AsCharPtr());

	// update
	this->UpdateAttributes(attributesFile, attributes);

	// format attributes file
	String physicsFile;
	physicsFile.Format("src:assets/%s/%s.physics", category.AsCharPtr(), file.AsCharPtr());

	// update
	this->UpdatePhysics(physicsFile, physics);

	// create model builder
	Ptr<ModelBuilder> modelBuilder = ModelBuilder::Create();

	// set constants and attributes
	modelBuilder->SetConstants(constants);
	modelBuilder->SetAttributes(attributes);
	modelBuilder->SetPhysics(physics);

	// format name of model
	String destination;
	destination.Format("mdl:%s/%s.n3", category.AsCharPtr(), file.AsCharPtr());

	// save file
	modelBuilder->SaveN3(destination, this->platform);
	
	// save physics
	destination.Format("phys:%s/%s.np3",category.AsCharPtr(), file.AsCharPtr());
	modelBuilder->SaveN3Physics(destination,this->platform);
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxSceneWriter::CreateSkeletalModel( const Ptr<N3Writer>& modelWriter,  const Util::Array<Ptr<NFbxMeshNode> >& meshes, const String& path)
{
	// extract file path from export path
	String file = path.ExtractFileName();
	file.StripFileExtension();
	String category = path.ExtractLastDirName();

	// merge file and category into a single name
	String fileCat = category + "/" + this->scene->GetName();

	// get constant model set
	Ptr<ModelConstants> constants = ModelDatabase::Instance()->LookupConstants(fileCat);

	// get model attributes
	Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(fileCat);

	// get physics
	Ptr<ModelPhysics> physics = ModelDatabase::Instance()->LookupPhysics(fileCat);

	if(scene->GetPhysicsMesh()->GetNumTriangles() > 0)
	{
		Util::String phname;
		phname.Format("phymsh:%s_ph.nvx2", fileCat.AsCharPtr());
		physics->SetPhysicsMesh(phname);
	}

	// clear constants
	constants->Clear();

	// loop through bounding boxes and get our scene bounding box
	Math::bbox globalBox;

	// extend over all boxes
	IndexT meshIndex;
	globalBox.begin_extend();
	for (meshIndex = 0; meshIndex < meshes.Size(); meshIndex++)
	{
		Ptr<NFbxMeshNode> mesh = meshes[meshIndex];
		globalBox.extend(mesh->GetBoundingBox());
	}
	globalBox.end_extend();

	// set global box for constants
	constants->SetGlobalBoundingBox(globalBox);

	// create joint array which will be extracted from the skeleton root
	Array<Joint> joints;

	const Array<Ptr<NFbxJointNode> >& skeletons = this->scene->GetSkeletonRoots();
	SizeT numSkeletons = skeletons.Size();
	IndexT skeletonIndex;
	for (skeletonIndex = 0; skeletonIndex < numSkeletons; skeletonIndex++)
	{
		const Ptr<NFbxJointNode>& skeletonRoot = skeletons[skeletonIndex];

		// extract joints from skeleton
		this->GetJoints(skeletonRoot, joints);
	}

	// format animation resource
	String animRes;
	animRes.Format("ani:%s/%s.nax3", category.AsCharPtr(), file.AsCharPtr());

	// create skin list
	Skinlist skinList;
	skinList.name = this->scene->GetName();

	// add each mesh as a skin
	for (meshIndex = 0; meshIndex < meshes.Size(); meshIndex++)
	{
		skinList.skins.Append(meshes[meshIndex]->GetName());
	}

	// create array of skin lists
	Array<Skinlist> skinLists;
	skinLists.Append(skinList);

	// create character node
	ModelConstants::CharacterNode characterNode;
	characterNode.joints = joints;
	characterNode.animation = animRes;
	characterNode.skinLists = skinLists;
	characterNode.name = this->scene->GetName() + "_ch";

	// add character node to constants
	constants->AddCharacterNode(characterNode.name, characterNode);

	// go over all meshes
	for (meshIndex = 0; meshIndex < meshes.Size(); meshIndex++)
	{
		// get mesh
		const Ptr<NFbxMeshNode>& mesh = meshes[meshIndex];

		// get skin fragments from mesh
		const Array<Ptr<SkinFragment> >& skinFragments = mesh->GetSkinFragments();

		// create skin
		ModelConstants::Skin skin;

		// don't write skin if it doesn't contain any fragments
		if (skinFragments.Size() == 0)
		{
			continue;
		}

		skin.name = mesh->GetName();
		skin.type = scene->GetSceneFeatureString();
		skin.boundingBox = mesh->GetBoundingBox();

		// create string for path to skin
		String skinPath;
		skinPath.Format("%s", skin.name.AsCharPtr());

		// create mesh name
		String meshPath;
		meshPath.Format("msh:%s/%s.nvx2", category.AsCharPtr(), scene->GetName().AsCharPtr());

		skin.path = skinPath;
		skin.mesh = meshPath;
		skin.transform.position = mesh->GetInitialPosition();
		skin.transform.rotation = mesh->GetInitialRotation();
		skin.transform.scale = mesh->GetInitialScale();

		// get state from attributes
		State state;

		// retrieve state if exists
		if (attributes->HasState(skinPath))
		{
			state = attributes->GetState(skinPath);
		}

		// set material of state
		if (!state.material.IsValid())
		{
			state.material = "PlaceholderSkinned";
		}		

		// search for diffuse map and set it to white if it isn't set
		Texture diffTex;
		diffTex.textureName = "DiffuseMap";
		diffTex.textureResource = "tex:system/placeholder";

		Texture normalTex;
		normalTex.textureName = "NormalMap";
		normalTex.textureResource = "tex:system/nobump";

		// if the diffuse map is not set, set it to white
		if (state.textures.FindIndex(diffTex) == InvalidIndex)
		{
			state.textures.Append(diffTex);
		}

		// do the same for the normal map
		if (state.textures.FindIndex(normalTex) == InvalidIndex)
		{
			state.textures.Append(normalTex);
		}

		// set state for attributes
		attributes->SetState(skinPath, state);

		IndexT fragIndex;
		for (fragIndex = 0; fragIndex < skinFragments.Size(); fragIndex++)
		{
			// get skin fragment
			const Ptr<SkinFragment>& skinFragment = skinFragments[fragIndex];

			// format node path
			String nodePath;
			nodePath.Format("%s/%s", skin.name.AsCharPtr(), (mesh->GetName() + "_" + String::FromInt(fragIndex)).AsCharPtr());

			// create skin node
			ModelConstants::SkinNode skinNode;
			skinNode.path = nodePath;
			skinNode.fragmentJoints = skinFragment->GetJointPalette();
			skinNode.primitiveGroupIndex = skinFragment->GetGroupId();
			skinNode.name = mesh->GetName() + "_" + String::FromInt(fragIndex);

			// add skin node to skin
			skin.skinFragments.Append(skinNode);
		}

		// now add skin to constants
		constants->AddSkin(mesh->GetName(), skin);
	}

	const Util::Array<Ptr<NFbxMeshNode> >& physicsNodes = scene->GetPhysicsNodes();
	IndexT i;
	for (i = 0; i < physicsNodes.Size(); i++)
	{
		// get mesh
		const Ptr<NFbxMeshNode>& mesh = physicsNodes[i];

		// create full node path
		String nodePath;
		nodePath.Format("root/%s", mesh->GetName().AsCharPtr());

		// create mesh name
		String meshPath;
		meshPath.Format("phymsh:%s/%s_ph.nvx2", category.AsCharPtr(), scene->GetName().AsCharPtr());

		// create physics node
		ModelConstants::PhysicsNode node;
		node.mesh = meshPath;
		node.path = nodePath;
		node.transform.position = mesh->GetInitialPosition();
		node.transform.rotation = mesh->GetInitialRotation();
		node.transform.scale = mesh->GetInitialScale();
		node.primitiveGroupIndex = mesh->GetGroupId();
		node.name = mesh->GetName();

		// add to constants
		constants->AddPhysicsNode(mesh->GetName(), node);
	}

	String constantsFile;
	constantsFile.Format("src:assets/%s/%s.constants", category.AsCharPtr(), file.AsCharPtr());

	// update
	this->UpdateConstants(constantsFile, constants);

	// format attributes file
	String attributesFile;
	attributesFile.Format("src:assets/%s/%s.attributes", category.AsCharPtr(), file.AsCharPtr());

	// update
	this->UpdateAttributes(attributesFile, attributes);

	// format attributes file
	String physicsFile;
	physicsFile.Format("src:assets/%s/%s.physics", category.AsCharPtr(), file.AsCharPtr());

	// update
	this->UpdatePhysics(physicsFile, physics);

	// create model builder
	Ptr<ModelBuilder> modelBuilder = ModelBuilder::Create();

	// set constants and attributes
	modelBuilder->SetConstants(constants);
	modelBuilder->SetAttributes(attributes);
	modelBuilder->SetPhysics(physics);

	// format name of model
	String destination;
	destination.Format("mdl:%s/%s.n3", category.AsCharPtr(), file.AsCharPtr());

	// save file
	modelBuilder->SaveN3(destination, this->platform);

	destination.Format("phys:%s/%s.np3",category.AsCharPtr(), file.AsCharPtr());
	modelBuilder->SaveN3Physics(destination,this->platform);
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxSceneWriter::GetJoints( const Ptr<NFbxJointNode> joint, Util::Array<Joint>& joints )
{
	// extract joint info into writer-friendly joint struct
	Joint currentJoint;
	currentJoint.name = joint->GetName();
	currentJoint.translation = joint->GetInitialPosition();
	currentJoint.rotation = joint->GetInitialRotation();
	currentJoint.scale = joint->GetInitialScale();
	currentJoint.index = joint->GetJointIndex();
	currentJoint.parent = joint->GetParentJoint();
	joints.Append(currentJoint);

	// iterate over children
	SizeT childCount = joint->GetChildCount();
	IndexT childIndex;
	for (childIndex = 0; childIndex < childCount; childIndex++)
	{
		Ptr<NFbxNode> child = joint->GetChild(childIndex);

		// only traverse further if child is joint
		if (child->IsA(NFbxJointNode::RTTI))
		{
			Ptr<NFbxJointNode> jointChild = child.downcast<NFbxJointNode>();
			this->GetJoints(jointChild, joints);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxSceneWriter::UpdateConstants( const Util::String& file, const Ptr<ModelConstants>& constants )
{
	// create stream
	Ptr<Stream> stream = IoServer::Instance()->CreateStream(file);
	stream->SetAccessMode(Stream::ReadAccess);
	stream->Open();

	if (stream->IsOpen())
	{
		// create memory stream
		Ptr<Stream> memStream = MemoryStream::Create();

		// now save constants
		constants->Save(memStream);

		// read from file
		uchar* origData = (uchar*)stream->Map();
		SizeT origSize = stream->GetSize();

		// open memstream again
		memStream->SetAccessMode(Stream::ReadAccess);
		memStream->Open();

		// get data from memory
		uchar* newData = (uchar*)memStream->Map();
		SizeT newSize = memStream->GetSize();

		// create crc codes
		Crc crc1;
		Crc crc2;

		// compute first crc
		crc1.Begin();
		crc1.Compute(origData, origSize);
		crc1.End();

		crc2.Begin();
		crc2.Compute(newData, newSize);
		crc2.End();

		// compare crc codes
		if (crc1.GetResult() != crc2.GetResult())
		{
			// write new file if crc-codes mismatch
			stream->Close();
			stream->SetAccessMode(Stream::WriteAccess);
			stream->Open();
			stream->Write(newData, newSize);
			stream->Close();
		}

		// close memstream
		memStream->Close();
	}
	else
	{
		// save directly
		constants->Save(stream);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxSceneWriter::UpdateAttributes( const Util::String& file, const Ptr<ModelAttributes>& attributes )
{
	// create stream
	Ptr<Stream> stream = IoServer::Instance()->CreateStream(file);
	stream->SetAccessMode(Stream::ReadAccess);
	stream->Open();

	if (stream->IsOpen())
	{
		// create memory stream
		Ptr<Stream> memStream = MemoryStream::Create();

		// now save constants
		attributes->Save(memStream);

		// read from file
		uchar* origData = (uchar*)stream->Map();
		SizeT origSize = stream->GetSize();

		// open memstream again
		memStream->SetAccessMode(Stream::ReadAccess);
		memStream->Open();

		// get data from memory
		uchar* newData = (uchar*)memStream->Map();
		SizeT newSize = memStream->GetSize();

		// write new file if crc-codes mismatch
		stream->Close();
		stream->SetAccessMode(Stream::WriteAccess);
		stream->Open();
		stream->Write(newData, newSize);
		stream->Close();
		/*
		// create crc codes
		Crc crc1;
		Crc crc2;

		// compute first crc
		crc1.Begin();
		crc1.Compute(origData, origSize);
		crc1.End();

		crc2.Begin();
		crc2.Compute(newData, newSize);
		crc2.End();
		
		// compare crc codes
		if (crc1.GetResult() != crc2.GetResult())
		{

		}
		else
		{
			// update file write time
			//IoServer::Instance()->SetFileWriteTime(file, IoServer::Instance()->GetFileWriteTime())
		}
		*/

		// close memstream
		memStream->Close();
	}
	else
	{
		attributes->Save(stream);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxSceneWriter::UpdatePhysics( const Util::String& file, const Ptr<ModelPhysics>& physics )
{
	// create stream
	Ptr<Stream> stream = IoServer::Instance()->CreateStream(file);
	stream->SetAccessMode(Stream::ReadAccess);
	stream->Open();

	if (stream->IsOpen())
	{
		// create memory stream
		Ptr<Stream> memStream = MemoryStream::Create();

		// now save constants
		physics->Save(memStream);

		// read from file
		uchar* origData = (uchar*)stream->Map();
		SizeT origSize = stream->GetSize();

		// open memstream again
		memStream->SetAccessMode(Stream::ReadAccess);
		memStream->Open();

		// get data from memory
		uchar* newData = (uchar*)memStream->Map();
		SizeT newSize = memStream->GetSize();

		// create crc codes
		Crc crc1;
		Crc crc2;

		// compute first crc
		crc1.Begin();
		crc1.Compute(origData, origSize);
		crc1.End();

		crc2.Begin();
		crc2.Compute(newData, newSize);
		crc2.End();

		// compare crc codes
		if (crc1.GetResult() != crc2.GetResult())
		{
			// write new file if crc-codes mismatch
			stream->Close();
			stream->SetAccessMode(Stream::WriteAccess);
			stream->Open();
			stream->Write(newData, newSize);
			stream->Close();
		}

		// close memstream
		memStream->Close();
	}
	else
	{
		physics->Save(stream);
	}
}
} // namespace ToolkitUtil