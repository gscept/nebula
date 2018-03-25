//------------------------------------------------------------------------------
//  modelconstants.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelconstants.h"
#include "io/xmlwriter.h"
#include "io/xmlreader.h"
#include "io/stream.h"
#include "io/ioserver.h"

using namespace IO;
using namespace Util;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::ModelConstants, 'MDCN', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ModelConstants::ModelConstants()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelConstants::~ModelConstants()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::AddCharacterNode( const Util::String& name, const ModelConstants::CharacterNode& node )
{
	n_assert(!this->characterNodes.Contains(name));
	this->characterNodes.Add(name, node);
}

//------------------------------------------------------------------------------
/**
*/
const ModelConstants::CharacterNode& 
ModelConstants::GetCharacterNode(const Util::String& name) const
{
	n_assert(this->characterNodes.Contains(name));
	return this->characterNodes[name];
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelConstants::HasCharacterNode(const Util::String& name)
{
	return this->characterNodes.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelConstants::DeleteCharacterNode(const Util::String& name)
{
	n_assert(this->characterNodes.Contains(name));
	this->characterNodes.Erase(name);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::AddShapeNode( const Util::String& name, const ModelConstants::ShapeNode& node )
{
	n_assert(!this->shapeNodes.Contains(name));
	this->shapeNodes.Add(name, node);
}

//------------------------------------------------------------------------------
/**
*/
const ModelConstants::ShapeNode& 
ModelConstants::GetShapeNode( const Util::String& name )
{
	n_assert(this->shapeNodes.Contains(name));
	return this->shapeNodes[name];
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelConstants::HasShapeNode(const Util::String& name)
{
	return this->shapeNodes.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelConstants::DeleteShapeNode(const Util::String& name)
{
	n_assert(this->shapeNodes.Contains(name));
	this->shapeNodes.Erase(name);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::AddSkin( const Util::String& name, const ModelConstants::Skin& skin )
{
	n_assert(!this->skins.Contains(name));
	this->skins.Add(name, skin);
}

//------------------------------------------------------------------------------
/**
*/
const ModelConstants::Skin& 
ModelConstants::GetSkin( const Util::String& name )
{
	n_assert(this->skins.Contains(name));
	return this->skins[name];
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelConstants::HasSkin(const Util::String& name)
{
	return this->skins.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelConstants::DeleteSkin(const Util::String& name)
{
	n_assert(this->skins.Contains(name));
	this->skins.Erase(name);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::AddParticleNode( const Util::String& name, const ModelConstants::ParticleNode& node )
{
	n_assert(!this->particleNodes.Contains(name));
	this->particleNodes.Add(name, node);
}

//------------------------------------------------------------------------------
/**
*/
const ModelConstants::ParticleNode& 
ModelConstants::GetParticleNode( const Util::String& name )
{
	n_assert(this->particleNodes.Contains(name));
	return this->particleNodes[name];
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelConstants::HasParticleNode(const Util::String& name)
{
	return this->particleNodes.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelConstants::DeleteParticleNode(const Util::String& name)
{
	n_assert(this->particleNodes.Contains(name));
	this->particleNodes.Erase(name);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::ReplaceParticleNode( const Util::String& name, const ModelConstants::ParticleNode& node )
{
	n_assert(this->particleNodes.Contains(name));
	this->particleNodes[name] = node;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::AddPhysicsNode( const Util::String& name, const ModelConstants::PhysicsNode& node )
{
	n_assert(!this->physicsNodes.Contains(name));
	this->physicsNodes.Add(name, node);
}

//------------------------------------------------------------------------------
/**
*/
const ModelConstants::PhysicsNode& 
ModelConstants::GetPhysicsNode( const Util::String& name )
{
	n_assert(this->physicsNodes.Contains(name));
	return this->physicsNodes[name];
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelConstants::HasPhysicsNode(const Util::String& name)
{
	return this->physicsNodes.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelConstants::DeletePhysicsNode(const Util::String& name)
{
	n_assert(this->physicsNodes.Contains(name));
	this->physicsNodes.Erase(name);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::Save(const Ptr<Stream>& stream)
{
	n_assert(stream.isvalid());

	// set correct access and open stream
	stream->SetAccessMode(Stream::WriteAccess);
	if (stream->Open())
	{
		// create xml writer
		Ptr<XmlWriter> writer = XmlWriter::Create();
		writer->SetStream(stream);
		writer->Open();

		// start with nebula tag
		writer->BeginNode("Nebula3");

		// set version
		writer->SetInt("version", ModelConstants::Version);

		// start model node
		writer->BeginNode("Model");

		// set name of model
		writer->SetString("name", this->name);

		// set bounding box
		writer->SetFloat4("bboxcenter", this->globalBoundingBox.center());
		writer->SetFloat4("bboxextents", this->globalBoundingBox.extents());

		if (this->characterNodes.Size() > 0)
		{
			// write content, start with character nodes
			writer->BeginNode("CharacterNodes");

			// go through shapes and write nodes
			IndexT i;
			for (i = 0; i < this->characterNodes.Size(); i++)
			{

				// get shape node
				const ModelConstants::CharacterNode& node = this->characterNodes.ValueAtIndex(i);

				// begin character node
				writer->BeginNode("CharacterNode");

				// set name of character node
				writer->SetString("name", node.name);

				// set animation resource
				writer->SetString("animation", node.animation);

				// add skin lists
				IndexT j;
				for (j = 0; j < node.skinLists.Size(); j++)
				{
					// get skin list
					const ToolkitUtil::Skinlist& skinList = node.skinLists[j];

					// write skin list
					writer->BeginNode("Skinlist");

					// set name of skin list
					writer->SetString("name", skinList.name);

					// add all skins
					IndexT k;
					for (k = 0; k < skinList.skins.Size(); k++)
					{
						// write skin
						writer->BeginNode("Skin");

						// write name of skin
						writer->SetString("name", skinList.skins[k]);

						// end skin
						writer->EndNode();
					}

					// end skin list
					writer->EndNode();
				}

				// add joints
				for (j = 0; j < node.joints.Size(); j++)
				{
					// get joint
					const ToolkitUtil::Joint& joint = node.joints[j];

					// write joint
					writer->BeginNode("Joint");

					// write name of jointer
					writer->SetString("name", joint.name);

					// write index of joint
					writer->SetInt("index", joint.index);

					// write parent index
					writer->SetInt("parent", joint.parent);

					// write transform
					writer->SetFloat4("position", joint.translation);
					writer->SetFloat4("rotation", Math::float4(
						joint.rotation.x(), 
						joint.rotation.y(), 
						joint.rotation.z(), 
						joint.rotation.w()));
					writer->SetFloat4("scale", joint.scale);

					// end joint
					writer->EndNode();
				}

				// end character node
				writer->EndNode();
			}

			// end character nodes
			writer->EndNode();
		}
		
		if (this->shapeNodes.Size() > 0)
		{
			// write shape nodes
			writer->BeginNode("ShapeNodes");

			// go through shapes and write nodes
			IndexT i;
			for (i = 0; i < this->shapeNodes.Size(); i++)
			{
				// get shape node
				const ModelConstants::ShapeNode& node = this->shapeNodes.ValueAtIndex(i);

				// write shape node
				writer->BeginNode("ShapeNode");

				// write name
				writer->SetString("name", node.name);

				// write type
				writer->SetString("type", node.type);

				// write path
				writer->SetString("path", node.path);
				
				// write if lod should be used
				writer->SetBool("useLOD", node.useLOD);

				if (node.useLOD)
				{
					// write lod distances
					writer->SetFloat("LODMax", node.LODMax);
					writer->SetFloat("LODMin", node.LODMin);
				}				

				// write primitive group index
				writer->SetInt("primitive", node.primitiveGroupIndex);

				// set position
				writer->SetFloat4("position", node.transform.position);

				// set rotation
				writer->SetFloat4("rotation", Math::float4(
					node.transform.rotation.x(), 
					node.transform.rotation.y(), 
					node.transform.rotation.z(), 
					node.transform.rotation.w()));

				// set scale
				writer->SetFloat4("scale", node.transform.scale);

				// set mesh
				writer->SetString("mesh", node.mesh);

				// set bounding box
				writer->SetFloat4("bboxcenter", node.boundingBox.center());
				writer->SetFloat4("bboxextents", node.boundingBox.extents());

				// end shape node
				writer->EndNode();
			}

			// end shape node tag
			writer->EndNode();
		}

		if (this->physicsNodes.Size() > 0)
		{
			// begin physics nodes
			writer->BeginNode("PhysicsNodes");

			// go through and write out physics nodes
			IndexT i;
			for (i = 0; i < this->physicsNodes.Size(); i++)
			{
				// get physics node
				const ModelConstants::PhysicsNode& node = this->physicsNodes.ValueAtIndex(i);

				// write physics node
				writer->BeginNode("PhysicsNode");

				// write data
				writer->SetString("name", node.name);
				writer->SetString("path", node.path);
				writer->SetString("mesh", node.mesh);
				writer->SetInt("primitive", node.primitiveGroupIndex);

				// set position
				writer->SetFloat4("position", node.transform.position);

				// set rotation
				writer->SetFloat4("rotation", Math::float4(
					node.transform.rotation.x(), 
					node.transform.rotation.y(), 
					node.transform.rotation.z(), 
					node.transform.rotation.w()));

				// set scale
				writer->SetFloat4("scale", node.transform.scale);

				// end physics node
				writer->EndNode();
			}

			// end physics nodes
			writer->EndNode();
		}

		if (this->particleNodes.Size() > 0)
		{
			// write shape nodes
			writer->BeginNode("ParticleNodes");

			// go through shapes and write nodes
			IndexT i;
			for (i = 0; i < this->particleNodes.Size(); i++)
			{
				// get shape node
				const ModelConstants::ParticleNode& node = this->particleNodes.ValueAtIndex(i);

				// write shape node
				writer->BeginNode("ParticleNode");

				// write name
				writer->SetString("name", node.name);

				// write type
				writer->SetString("type", node.type);

				// write path
				writer->SetString("path", node.path);

				// write primitive group index
				writer->SetInt("primitive", node.primitiveGroupIndex);

				// set position
				writer->SetFloat4("position", node.transform.position);

				// set rotation
				writer->SetFloat4("rotation", Math::float4(
					node.transform.rotation.x(), 
					node.transform.rotation.y(), 
					node.transform.rotation.z(), 
					node.transform.rotation.w()));

				// set scale
				writer->SetFloat4("scale", node.transform.scale);

				// set bounding box
				writer->SetFloat4("bboxcenter", node.boundingBox.center());
				writer->SetFloat4("bboxextents", node.boundingBox.extents());

				// end shape node
				writer->EndNode();
			}

			// end shape node tag
			writer->EndNode();
		}

		if (this->skins.Size() > 0)
		{
			// write skins
			writer->BeginNode("Skins");

			// write skins 
			IndexT j;
			for (j = 0; j < this->skins.Size(); j++)
			{
				// get skin
				const ModelConstants::Skin& skin = this->skins.ValueAtIndex(j);			

				// write skin
				writer->BeginNode("Skin");

				// write name of skin
				writer->SetString("name", skin.name);

				// write type
				writer->SetString("type", skin.type);

				// write path of skin
				writer->SetString("path", skin.path);

				// write bounding box of skin
				writer->SetFloat4("bboxcenter", skin.boundingBox.center());
				writer->SetFloat4("bboxextents", skin.boundingBox.extents());

				// set position
				writer->SetFloat4("position", skin.transform.position);

				// set rotation
				writer->SetFloat4("rotation", Math::float4(
					skin.transform.rotation.x(), 
					skin.transform.rotation.y(), 
					skin.transform.rotation.z(),
					skin.transform.rotation.w()));

				// set scale
				writer->SetFloat4("scale", skin.transform.scale);

				// write mesh
				writer->SetString("mesh", skin.mesh);

				// write skin nodes
				IndexT k;
				for (k = 0; k < skin.skinFragments.Size(); k++)
				{
					// get skin fragment
					const ModelConstants::SkinNode& fragment = skin.skinFragments[k];

					// write fragment
					writer->BeginNode("Fragment");

					// write fragment name
					writer->SetString("name", fragment.name);

					// write path
					writer->SetString("path", fragment.path);

					// write primitive group
					writer->SetInt("primitive", fragment.primitiveGroupIndex);

					// write tag for joints
					writer->BeginNode("Joints");

					// we write every joint here as content
					IndexT l;
					for (l = 0; l < fragment.fragmentJoints.Size(); l++)
					{
						writer->WriteContent(String::FromInt(fragment.fragmentJoints[l]));
						if (l < fragment.fragmentJoints.Size() - 1)
						{
							writer->WriteContent(", ");
						}
					}

					// end tag for joints
					writer->EndNode();

					// end fragment
					writer->EndNode();
				}

				// end skin
				writer->EndNode();
			}

			// end skins
			writer->EndNode();
		}

		// close model node
		writer->EndNode();

		// close nebula3 node
		writer->EndNode();

		// closes writer and file
		writer->Close();
		stream->Close();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::Load(const Ptr<Stream>& stream)
{
	n_assert(stream.isvalid());

	// set correct access mode and open stream
	stream->SetAccessMode(Stream::ReadAccess);
	if (stream->Open())
	{
		// create xml reader
		Ptr<XmlReader> reader = XmlReader::Create();
		reader->SetStream(stream);
		reader->Open();

		// get version
		int version = reader->GetInt("version");

		// if the versions mismatch, don't continue with the loading process
		if (version != ModelConstants::Version)
		{
			n_warning("Invalid version in model constants: %s\n", stream->GetURI().LocalPath().AsCharPtr());
			stream->Close();
			return;
		}

		// then make sure we have the options tag
		n_assert2(reader->SetToFirstChild("Model"), "CORRUPT .constants FILE!: First tag must be Model!");

		// get name
		this->name = reader->GetString("name");

		// get bounding box
		this->globalBoundingBox = Math::bbox(reader->GetFloat4("bboxcenter"), reader->GetFloat4("bboxextents"));

		// first check to see if we have a character node
		if (reader->SetToFirstChild("CharacterNodes"))
		{
			// go through all character nodes
			if (reader->SetToFirstChild("CharacterNode")) do 
			{
				// create character node
				ModelConstants::CharacterNode characterNode;

				// get name
				characterNode.name = reader->GetString("name");

				// get animation
				characterNode.animation = reader->GetString("animation");

				// go through all skin lists
				if (reader->SetToFirstChild("Skinlist")) do 
				{
					// create skin list
					ToolkitUtil::Skinlist skinList;

					// get name
					skinList.name = reader->GetString("name");

					// go through all skins
					if (reader->SetToFirstChild("Skin")) do 
					{
						// simply retrieve skin from reader
						skinList.skins.Append(reader->GetString("name"));						
					} 
					while (reader->SetToNextChild("Skin"));

					// add skinlist to character
					characterNode.skinLists.Append(skinList);
				} 
				while (reader->SetToNextChild("Skinlist"));


				// iterate over joints
				if (reader->SetToFirstChild("Joint")) do 
				{
					// create joint
					ToolkitUtil::Joint joint;

					// get name
					joint.name = reader->GetString("name");

					// get index of joint
					joint.index = reader->GetInt("index");

					// get index of parent
					joint.parent = reader->GetInt("parent");

					// get translation
					joint.translation = reader->GetFloat4("position");

					// get rotation
					joint.rotation = reader->GetFloat4("rotation");

					// get scale
					joint.scale = reader->GetFloat4("scale");

					// add joint to character
					characterNode.joints.Append(joint);
				} 
				while (reader->SetToNextChild("Joint"));

				// add character to constants
				this->AddCharacterNode(characterNode.name, characterNode);
			} 
			while (reader->SetToNextChild("CharacterNode"));

			// jump back to parent
			reader->SetToParent();
		}

		// go through shapes
		if (reader->SetToFirstChild("ShapeNodes"))
		{
			// iterate over all shapes
			if (reader->SetToFirstChild("ShapeNode")) do
			{
				// get shape node
				ModelConstants::ShapeNode node;

				// get name
				node.name = reader->GetString("name");

				// get node type
				node.type = reader->GetString("type");

				// get path of node
				node.path = reader->GetString("path");

				// get if lod should be used
				node.useLOD = reader->GetBool("useLOD");

				if (node.useLOD)
				{
					// get lod distances
					node.LODMax = reader->GetFloat("LODMax");
					node.LODMin = reader->GetFloat("LODMin");
				}				

				// get primitive index
				node.primitiveGroupIndex = reader->GetInt("primitive");

				// get position
				node.transform.position = reader->GetFloat4("position");

				// get rotation
				node.transform.rotation = reader->GetFloat4("rotation");

				// get scale
				node.transform.scale = reader->GetFloat4("scale");

				// get mesh
				node.mesh = reader->GetString("mesh");

				// get bounding box
				node.boundingBox = Math::bbox(reader->GetFloat4("bboxcenter"), reader->GetFloat4("bboxextents"));

				// add shape to constants
				this->AddShapeNode(node.name, node);

			}
			while (reader->SetToNextChild("ShapeNode"));

			// jump back to parent
			reader->SetToParent();
		}

		// go through physics
		if (reader->SetToFirstChild("PhysicsNodes"))
		{
			// iterate over all physics nodes
			if (reader->SetToFirstChild("PhysicsNode")) do
			{
				// create physics node
				ModelConstants::PhysicsNode node;

				// set data
				node.name = reader->GetString("name");
				node.path = reader->GetString("path");
				node.mesh = reader->GetString("mesh");
				node.primitiveGroupIndex = reader->GetInt("primitive");

				// get position
				node.transform.position = reader->GetFloat4("position");

				// get rotation
				node.transform.rotation = reader->GetFloat4("rotation");

				// get scale
				node.transform.scale = reader->GetFloat4("scale");

				// add node
				this->AddPhysicsNode(node.name, node);
			}
			while (reader->SetToNextChild("PhysicsNode"));

			// go to parent
			reader->SetToParent();
		}

		// go through shapes
		if (reader->SetToFirstChild("ParticleNodes"))
		{
			// iterate over all shapes
			if (reader->SetToFirstChild("ParticleNode")) do
			{
				// create particle node
				ModelConstants::ParticleNode node;

				// get name
				node.name = reader->GetString("name");

				// get path of node
				node.path = reader->GetString("path");

				// get node type
				node.type = reader->GetString("type");

				// get primitive index
				node.primitiveGroupIndex = reader->GetInt("primitive");

				// get position
				node.transform.position = reader->GetFloat4("position");

				// get rotation
				node.transform.rotation = reader->GetFloat4("rotation");

				// get scale
				node.transform.scale = reader->GetFloat4("scale");

				// get bounding box
				node.boundingBox = Math::bbox(reader->GetFloat4("bboxcenter"), reader->GetFloat4("bboxextents"));

				// add shape to constants
				this->AddParticleNode(node.name, node);

			}
			while (reader->SetToNextChild("ParticleNode"));

			// jump back to parent
			reader->SetToParent();
		}

		// go through skins
		if (reader->SetToFirstChild("Skins"))
		{
			// iterate through skins
			if (reader->SetToFirstChild("Skin")) do 
			{
				// get skin
				ModelConstants::Skin skin;

				// get name of skin
				skin.name = reader->GetString("name");

				// get node type
				skin.type = reader->GetString("type");

				// get path of skin
				skin.path = reader->GetString("path");

				// get mesh of fragment
				skin.mesh = reader->GetString("mesh");

				// get position of fragment
				skin.transform.position = reader->GetFloat4("position");

				// get rotation of fragment
				skin.transform.rotation = reader->GetFloat4("rotation");

				// get scale of fragment
				skin.transform.scale = reader->GetFloat4("scale");

				// get bounding box
				skin.boundingBox = Math::bbox(reader->GetFloat4("bboxcenter"), reader->GetFloat4("bboxextents"));

				// iterate through fragments
				if (reader->SetToFirstChild("Fragment")) do 
				{					
					// create skin fragment
					ModelConstants::SkinNode fragment;

					// get name of fragment
					fragment.name = reader->GetString("name");

					// get path of node
					fragment.path = reader->GetString("path");

					// get primitive of fragment
					fragment.primitiveGroupIndex = reader->GetInt("primitive");

					// go to joints
					reader->SetToNode("Joints");

					// read contents
					String jointIndices = reader->GetContent();

					// go back to parent
					reader->SetToParent();

					// split jointindices into individual parts
					Array<String> indices = jointIndices.Tokenize(", ");

					// go through every joint and convert from string to int
					IndexT i;
					for (i = 0; i < indices.Size(); i++)
					{
						fragment.fragmentJoints.Append(indices[i].AsInt());
					}

					// add fragment to skin
					skin.skinFragments.Append(fragment);
				} 
				while (reader->SetToNextChild("Fragment"));

				// add skin to constants
				this->AddSkin(skin.name, skin);
			} 
			while (reader->SetToNextChild("Skin"));

			// go back to parent
			reader->SetToParent();
		}

		// closes reader and file
		reader->Close();
		stream->Close();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::Clear()
{
	this->shapeNodes.Clear();
	this->characterNodes.Clear();
	this->skins.Clear();
	this->physicsNodes.Clear();
	this->particleNodes.Clear();
}


} // namespace ToolkitUtil