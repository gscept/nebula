//------------------------------------------------------------------------------
//  modelconstants.cc
//  (C) 2012 Gustav Sterbrant
//  (C) 2012-2020 Individual contributors. See AUTHORS file.
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
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
ModelConstants::AddShapeNode(const ModelConstants::ShapeNode& node)
{
    this->shapeNodes.Append(node);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ModelConstants::ShapeNode>&
ModelConstants::GetShapeNodes() const
{
    return this->shapeNodes;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelConstants::AddSkinSetNode(const ModelConstants::SkinSetNode& skinSet)
{
    this->skinSetNodes.Append(skinSet);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ModelConstants::SkinSetNode>& 
ModelConstants::GetSkinSetNodes() const
{
    return this->skinSetNodes;
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
WriteTransform(const Ptr<XmlWriter>& writer, const ModelConstants::TransformNode* node, bool hasLOD = false)
{
    // write name
    writer->SetString("name", node->name);

    // write path
    writer->SetString("path", node->path);

    if (hasLOD)
    {
        // write if lod should be used
        writer->SetBool("useLOD", node->useLOD);

        if (node->useLOD)
        {
            // write lod distances
            writer->SetFloat("LODMax", node->LODMax);
            writer->SetFloat("LODMin", node->LODMin);
        }
    }

    // set position
    writer->SetVec4("position", node->transform.position);

    // set rotation
    writer->SetVec4("rotation", Math::vec4(
        node->transform.rotation.x,
        node->transform.rotation.y,
        node->transform.rotation.z,
        node->transform.rotation.w));

    // set scale
    writer->SetVec4("scale", node->transform.scale);

    // set bounding box
    writer->SetVec4("bboxcenter", node->boundingBox.center());
    writer->SetVec4("bboxextents", node->boundingBox.extents());
}

//------------------------------------------------------------------------------
/**
*/
void
ReadTransform(const Ptr<XmlReader>& reader, ModelConstants::TransformNode* node, bool hasLOD = false)
{
    node->name = reader->GetString("name");
    node->path = reader->GetString("path");
    node->boundingBox = Math::bbox(xyz(reader->GetVec4("bboxcenter")), xyz(reader->GetVec4("bboxextents")));

    if (hasLOD)
    {
        node->useLOD = reader->GetBool("useLOD");
        if (node->useLOD)
        {
            node->LODMin = reader->GetFloat("LODMin");
            node->LODMax = reader->GetFloat("LODMax");
        }
    }

    // get position
    node->transform.position = reader->GetVec4("position");

    // get rotation
    node->transform.rotation = reader->GetVec4("rotation");

    // get scale
    node->transform.scale = reader->GetVec4("scale").vec;
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
        writer->BeginNode("Nebula");

        // set version
        writer->SetInt("version", ModelConstants::Version);

        // start model node
        writer->BeginNode("Model");

        // set name of model
        writer->SetString("name", this->name);

        // set bounding box
        writer->SetVec4("bboxcenter", this->globalBoundingBox.center());
        writer->SetVec4("bboxextents", this->globalBoundingBox.extents());

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
                writer->SetString("animation", node.animationResource);
                writer->SetInt("animationIndex", node.animationIndex);

                // set skeleton resource
                writer->SetString("skeleton", node.skeletonResource);
                writer->SetInt("skeletonIndex", node.skeletonIndex);

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
            for (const auto& shapeNode : this->shapeNodes)
            {
                // write shape node
                writer->BeginNode("ShapeNode");

                WriteTransform(writer, &shapeNode, true);

                // set mesh
                writer->SetString("mesh", shapeNode.meshResource);
                writer->SetInt("mid", shapeNode.meshIndex);
                writer->SetInt("primitive", shapeNode.primitiveGroupIndex);

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

                // set position
                writer->SetVec4("position", node.transform.position);

                // set rotation
                writer->SetVec4("rotation", Math::vec4(
                    node.transform.rotation.x, 
                    node.transform.rotation.y, 
                    node.transform.rotation.z, 
                    node.transform.rotation.w));

                // set scale
                writer->SetVec4("scale", node.transform.scale);

                // Write mesh
                writer->SetString("mesh", node.mesh);
                writer->SetInt("mid", 0);
                writer->SetInt("primitive", node.primitiveGroupIndex);

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

                WriteTransform(writer, &node);

                // write primitive group index
                writer->SetInt("primitive", node.primitiveGroupIndex);

                // end shape node
                writer->EndNode();
            }

            // end shape node tag
            writer->EndNode();
        }

        if (this->skinSetNodes.Size() > 0)
        {
            writer->BeginNode("SkinSetNodes");
            for (const auto& skinSet : this->skinSetNodes)
            {
                writer->BeginNode("SkinSetNode");

                WriteTransform(writer, &skinSet);

                if (skinSet.skinFragments.Size() > 0)
                {

                    // write skins 
                    for (const auto& skin : skinSet.skinFragments)
                    {
                        // write skin
                        writer->BeginNode("SkinNode");

                        WriteTransform(writer, &skin, true);

                        // write mesh
                        writer->SetString("mesh", skin.meshResource);
                        writer->SetInt("mid", skin.meshIndex);
                        writer->SetInt("primitive", skin.primitiveGroupIndex);

                        // write tag for joints
                        writer->BeginNode("Joints");

                        // we write every joint here as content
                        IndexT l;
                        for (l = 0; l < skin.fragmentJoints.Size(); l++)
                        {
                            writer->WriteContent(String::FromInt(skin.fragmentJoints[l]));
                            if (l < skin.fragmentJoints.Size() - 1)
                            {
                                writer->WriteContent(", ");
                            }
                        }

                        // End joints
                        writer->EndNode();
                    }

                    // end skins
                    writer->EndNode();
                }

                // End skin set
                writer->EndNode();
            }
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
        this->globalBoundingBox = Math::bbox(reader->GetVec4("bboxcenter"), reader->GetVec4("bboxextents").vec);

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
                characterNode.animationResource = reader->GetString("animation");
                characterNode.animationIndex = reader->GetInt("animationIndex");

                // get skeleton
                characterNode.skeletonResource = reader->GetString("skeleton");
                characterNode.skeletonIndex = reader->GetInt("skeletonIndex");

                // add character to constants
                this->AddCharacterNode(characterNode.name, characterNode);
            } 
            while (reader->SetToNextChild("CharacterNode"));

            // jump back to parent
            reader->SetToParent();
        }

        if (reader->SetToFirstChild("SkinSetNodes"))
        {
            if (reader->SetToFirstChild("SkinSetNode")) do
            {
                ModelConstants::SkinSetNode skinSetNode;
                ReadTransform(reader, &skinSetNode);

                if (reader->SetToFirstChild("SkinNode")) do
                {
                    ModelConstants::SkinNode skin;

                    ReadTransform(reader, &skin);
                    skin.meshResource = reader->GetString("mesh");
                    skin.meshIndex = reader->GetInt("mid");

                    // get primitive index
                    skin.primitiveGroupIndex = reader->GetInt("primitive");

                    if (reader->SetToFirstChild("Joints"))
                    {
                        const Util::String& content = reader->GetContent();
                        Util::Array<String> joints = content.Tokenize(", ");
                        for (const auto& joint : joints)
                        {
                            skin.fragmentJoints.Append(joint.AsInt());
                        }
                    }
                    reader->SetToParent();

                    skinSetNode.skinFragments.Append(skin);
                } 
                while (reader->SetToNextChild("SkinNode"));

                this->AddSkinSetNode(skinSetNode);

            } 
            while (reader->SetToNextChild("SkinSetNode"));
        }

        // go through shapes
        if (reader->SetToFirstChild("ShapeNodes"))
        {
            // iterate over all shapes
            if (reader->SetToFirstChild("ShapeNode")) do
            {
                // get shape node
                ModelConstants::ShapeNode node;

                ReadTransform(reader, &node, true);

                // get primitive index
                node.primitiveGroupIndex = reader->GetInt("primitive");

                // get mesh
                node.meshResource = reader->GetString("mesh");
                node.meshIndex = reader->GetInt("mid");

                // add shape to constants
                this->AddShapeNode(node);

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
                node.transform.position = reader->GetVec4("position");

                // get rotation
                node.transform.rotation = reader->GetVec4("rotation");

                // get scale
                node.transform.scale = reader->GetVec4("scale").vec;

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

                ReadTransform(reader, &node);

                // get primitive index
                node.primitiveGroupIndex = reader->GetInt("primitive");

                // add shape to constants
                this->AddParticleNode(node.name, node);

            }
            while (reader->SetToNextChild("ParticleNode"));

            // jump back to parent
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
    this->skinSetNodes.Clear();
    this->physicsNodes.Clear();
    this->particleNodes.Clear();
}

} // namespace ToolkitUtil