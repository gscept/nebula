//------------------------------------------------------------------------------
//  @file scenewriter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scenewriter.h"
#include "io/memorystream.h"
#include "io/ioserver.h"
#include "model/modelutil/modeldatabase.h"
#include "model/modelutil/modelconstants.h"
#include "model/modelutil/modelbuilder.h"
#include "util/crc.h"
#include "io/xmlwriter.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"

namespace ToolkitUtil
{

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
SceneWriter::GenerateModels(
    const Util::String& basePath
    , const Scene* scene
    , const Platform::Code platform
    , const Util::Array<SceneNode*>& graphicsNodes
    , const Util::Array<SceneNode*>& physicsNodes
    , const Util::Array<SceneNode*>& characterNodes
    , const ToolkitUtil::ImportFlags& flags
)
{
    n_assert(scene != nullptr);

    // make sure category directory exists
    IO::IoServer::Instance()->CreateDirectory(basePath);

    // extract file path from export path
    String path = basePath + scene->GetName() + ".xml";
    String file = scene->GetName();
    String category = path.ExtractLastDirName();

    // merge file and category into a single name
    String fileCat = category + "/" + scene->GetName();

    // Loop through bounding boxes and get our scene bounding box
    Math::bbox globalBox;
    IndexT meshIndex;
    globalBox.begin_extend();
    for (meshIndex = 0; meshIndex < graphicsNodes.Size(); meshIndex++)
    {
        const SceneNode* mesh = graphicsNodes[meshIndex];
        globalBox.extend(mesh->base.boundingBox);
    }
    globalBox.end_extend();

    // format animation resource
    String animRes;
    animRes.Format("ani:%s/%s.nax", category.AsCharPtr(), file.AsCharPtr());

    // format skeleton resource
    String skeletonRes;
    skeletonRes.Format("ske:%s/%s.nsk", category.AsCharPtr(), file.AsCharPtr());

    // create mesh name
    String physicsMeshRes;
    physicsMeshRes.Format("phymsh:%s/%s_ph.nvx", category.AsCharPtr(), scene->GetName().AsCharPtr());

    // create mesh name
    String meshResource;
    meshResource.Format("msh:%s/%s.nvx", category.AsCharPtr(), scene->GetName().AsCharPtr());

    ToolkitUtil::ModelResourceT model;
    model.name = scene->GetName();
    IndexT i;
    for (i = 0; i < graphicsNodes.Size(); i++)
    {
        // get mesh node
        const SceneNode* mesh = graphicsNodes[i];

        // create full node path
        String nodePath;
        nodePath.Format("root/%s", mesh->base.name.AsCharPtr());

        // create and add a shape node
        if (mesh->base.isSkin)
        {
            // Every skin has a set of fragments which should be treated as a series of draw calls
            // as opposed to a set of individual model nodes, which is what the SkinSetNode is for
            ModelConstants::SkinSetNode skinSetNode;
            skinSetNode.name = mesh->base.name;
            skinSetNode.path = nodePath;

            auto skin = std::make_unique<ToolkitUtil::SkinNodeT>();

            for (IndexT j = 0; j < mesh->skin.skinFragments.Size(); j++)
            {
                auto transform = std::make_unique<ToolkitUtil::TransformNodeInfoT>();
                transform->name = mesh->base.name;
                transform->translation = mesh->base.translation;
                transform->rotation = mesh->base.rotation;
                transform->scale = mesh->base.scale;

                auto shape = std::make_unique<ToolkitUtil::ShapeNodeT>();
                shape->use_lod = mesh->mesh.lodIndex != InvalidIndex;
                shape->lod_min = mesh->mesh.minLodDistance;
                shape->lod_max = mesh->mesh.maxLodDistance;
                shape->bbox_min = xyz(mesh->base.boundingBox.pmin);
                shape->bbox_max = xyz(mesh->base.boundingBox.pmax);
                shape->prim_group = mesh->skin.skinFragments[j];
                shape->mesh_resource = meshResource;
                shape->mesh_index = mesh->mesh.meshIndex;
                shape->material = mesh->mesh.material;
                shape->transform = std::move(transform);

                auto skinFragment = std::make_unique<ToolkitUtil::SkinFragmentNodeT>();
                const Util::Array<IndexT>& indices = mesh->skin.jointLookup[j].KeysAsArray();
                skinFragment->joints = std::vector<int32_t>(indices.Begin(), indices.End());
                skinFragment->shape = std::move(shape);
                skin->fragments.push_back(std::move(skinFragment));

                /*
                ModelConstants::SkinNode skinNode;
                skinNode.meshResource = meshResource;
                skinNode.meshIndex = mesh->mesh.meshIndex;
                String skinPath;
                skinNode.path.Format("%s/fragment_%d", nodePath.AsCharPtr(), j);
                skinNode.useLOD = mesh->mesh.lodIndex != InvalidIndex;
                if (skinNode.useLOD)
                {
                    skinNode.LODMax = mesh->mesh.maxLodDistance;
                    skinNode.LODMin = mesh->mesh.minLodDistance;
                }
                skinNode.transform.position = mesh->base.translation;
                skinNode.transform.rotation = mesh->base.rotation;
                skinNode.transform.scale = mesh->base.scale;
                skinNode.boundingBox = mesh->base.boundingBox;
                skinNode.name = mesh->base.name;
                skinNode.primitiveGroupIndex = mesh->skin.skinFragments[j];
                skinNode.fragmentJoints = mesh->skin.jointLookup[j].KeysAsArray();

                skinSetNode.skinFragments.Append(skinNode);

                //SetupDefaultState(skinNode.path, Util::String::Sprintf("%s/%s/%s", category.AsCharPtr(), file.AsCharPtr(), mesh->mesh.material.AsCharPtr()), attributes, true);
                */
            }

            //skinSets.Append(skinSetNode);
            model.skins.push_back(std::move(skin));
        }
        else
        {
            /*
            ModelConstants::ShapeNode shapeNode;
            shapeNode.meshResource = meshResource;
            shapeNode.meshIndex = mesh->mesh.meshIndex;
            shapeNode.path = nodePath;
            shapeNode.useLOD = mesh->mesh.lodIndex != InvalidIndex;
            if (shapeNode.useLOD)
            {
                shapeNode.LODMax = mesh->mesh.maxLodDistance;
                shapeNode.LODMin = mesh->mesh.minLodDistance;
            }
            shapeNode.transform.position = mesh->base.translation;
            shapeNode.transform.rotation = mesh->base.rotation;
            shapeNode.transform.scale = mesh->base.scale;
            shapeNode.boundingBox = mesh->base.boundingBox;
            shapeNode.name = mesh->base.name;
            shapeNode.primitiveGroupIndex = mesh->mesh.groupId;


            // add to constants
            shapes.Append(shapeNode);

            Util::String nodeMaterial = mesh->mesh.material;

            // If the material is a .sur material, we pass it as is.
            if ((nodeMaterial.Length() <= 4) || (!nodeMaterial.IsEmpty() && !nodeMaterial.EndsWithString(".sur")))
            {
                nodeMaterial = Util::String::Sprintf("%s/%s/%s", category.AsCharPtr(), file.AsCharPtr(), nodeMaterial.AsCharPtr());
            }

            //SetupDefaultState(shapeNode.path, nodeMaterial, attributes);
            */

            auto transform = std::make_unique<ToolkitUtil::TransformNodeInfoT>();
            transform->name = mesh->base.name;
            transform->translation = mesh->base.translation;
            transform->rotation = mesh->base.rotation;
            transform->scale = mesh->base.scale;

            auto shape = std::make_unique<ToolkitUtil::ShapeNodeT>();
            shape->use_lod = mesh->mesh.lodIndex != InvalidIndex;
            shape->lod_min = mesh->mesh.minLodDistance;
            shape->lod_max = mesh->mesh.maxLodDistance;
            shape->bbox_min = xyz(mesh->base.boundingBox.pmin);
            shape->bbox_max = xyz(mesh->base.boundingBox.pmax);
            shape->prim_group = mesh->mesh.groupId;
            shape->mesh_resource = meshResource;
            shape->mesh_index = mesh->mesh.meshIndex;
            shape->material = mesh->mesh.material;
            shape->transform = std::move(transform);
            model.shapes.push_back(std::move(shape));

        }
    }

    for (i = 0; i < characterNodes.Size(); i++)
    {
        // create character node
        /*
        ModelConstants::CharacterNode characterNode;
        characterNode.skeletonResource = skeletonRes;
        characterNode.skeletonIndex = characterNodes[i]->skeleton.skeletonIndex;
        characterNode.animationResource = animRes;
        characterNode.animationIndex = characterNodes[i]->anim.animIndex;
        characterNode.name = characterNodes[i]->base.name;
        // add character node to constants
        characters.Append(characterNode);
        */

        auto characterNodeInfo = std::make_unique<ToolkitUtil::CharacterNodeT>();
        characterNodeInfo->name = characterNodes[i]->base.name;
        characterNodeInfo->skeleton_resource = skeletonRes;
        characterNodeInfo->skeleton_index = characterNodes[i]->skeleton.skeletonIndex;
        characterNodeInfo->animation_resource = animRes;
        characterNodeInfo->animation_index = characterNodes[i]->anim.animIndex;
        model.characters.push_back(std::move(characterNodeInfo));
    }

    for (i = 0; i < physicsNodes.Size(); i++)
    {
        // get mesh
        const SceneNode* mesh = physicsNodes[i];

        // create full node path
        String nodePath;
        nodePath.Format("root/%s", mesh->base.name.AsCharPtr());

        auto transform = std::make_unique<ToolkitUtil::TransformNodeInfoT>();
        transform->name = mesh->base.name;
        transform->translation = mesh->base.translation;
        transform->rotation = mesh->base.rotation;
        transform->scale = mesh->base.scale;

        auto phys = std::make_unique<ToolkitUtil::PhysicsNodeT>();
        phys->transform = std::move(transform);
        phys->mesh_resource = physicsMeshRes;
        phys->material = "";
        model.physics.push_back(std::move(phys));

        // create physics node
        /*
        ModelConstants::PhysicsNode node;
        node.mesh = physicsMeshRes;
        node.path = nodePath;
        node.transform.position = mesh->base.translation;
        node.transform.rotation = mesh->base.rotation;
        node.transform.scale = mesh->base.scale;
        node.primitiveGroupIndex = mesh->mesh.groupId;
        node.name = mesh->base.name;

        // add to constants
        physics.Append(node);
        */
    }

    String modelFile;
    modelFile.Format("src:assets/%s/%s.namdl", category.AsCharPtr(), file.AsCharPtr());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(modelFile);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::ModelResource>(model);
        stream->Write(data.GetPtr(), data.Size());

        stream->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SetupDefaultState(
    const Util::String& nodePath
    , const Util::String& nodeMaterial
    , const Ptr<ModelAttributes>& attributes    
    , bool skinned = false
)
{
    // get state from attributes
    State state;

    // retrieve state if exists
    if (attributes->HasState(nodePath))
    {
        state = attributes->GetState(nodePath);
    }

    // If the node has a material that is already a surface, use that instead of creating a default one
    Util::String surfacePath = nodeMaterial;
    IO::URI uri = surfacePath;
    if (!surfacePath.EndsWithString(".sur"))
    {
        surfacePath = Util::String::Sprintf("sur:%s.sur", nodeMaterial.AsCharPtr());
    }
    if (IO::IoServer::Instance()->FileExists(surfacePath) || surfacePath.BeginsWithString("syssur:"))
    {
        state.material = surfacePath;
    }
    else if (!state.material.IsValid())
    {
        state.material = skinned ? "syssur:placeholder_skinned.sur" : "syssur:placeholder.sur";
    }
    else if (!state.material.EndsWithString(".sur")) // Temporary to fix all bad material assignments
    {
        state.material = Util::String::Sprintf("%s.sur", state.material.AsCharPtr());
    }
    
    // set state for attributes
    attributes->SetState(nodePath, state);
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
    writer->SetVec4("rotation", node->transform.rotation.vec);

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
SceneWriter::CreateModel(
    const Util::String& file
    , const Math::bbox globalBoundingBox
    , const Scene* scene
    , const Util::String& category
    , const Util::Array<SceneNode*>& graphicsNodes
    , const Util::Array<SceneNode*>& physicsNodes
    , const Util::Array<SceneNode*>& characterNodes
)
{
    // format animation resource
    String animRes;
    animRes.Format("ani:%s/%s.nax", category.AsCharPtr(), file.AsCharPtr());

    // format skeleton resource
    String skeletonRes;
    skeletonRes.Format("ske:%s/%s.nsk", category.AsCharPtr(), file.AsCharPtr());

    // create mesh name
    String physicsMeshRes;
    physicsMeshRes.Format("phymsh:%s/%s_ph.nvx", category.AsCharPtr(), scene->GetName().AsCharPtr());

    // create mesh name
    String meshResource;
    meshResource.Format("msh:%s/%s.nvx", category.AsCharPtr(), scene->GetName().AsCharPtr());

    ToolkitUtil::ModelResourceT model;

    Util::Array<ModelConstants::SkinSetNode> skinSets;
    Util::Array<ModelConstants::ShapeNode> shapes;
    Util::Array<ModelConstants::CharacterNode> characters;
    Util::Array<ModelConstants::PhysicsNode> physics;

    IndexT i;
    for (i = 0; i < graphicsNodes.Size(); i++)
    {
        // get mesh node
        const SceneNode* mesh = graphicsNodes[i];

        // create full node path
        String nodePath;
        nodePath.Format("root/%s", mesh->base.name.AsCharPtr());

        // create and add a shape node
        if (mesh->base.isSkin)
        {
            // Every skin has a set of fragments which should be treated as a series of draw calls
            // as opposed to a set of individual model nodes, which is what the SkinSetNode is for
            ModelConstants::SkinSetNode skinSetNode;
            skinSetNode.name = mesh->base.name;
            skinSetNode.path = nodePath;

            auto skin = std::make_unique<ToolkitUtil::SkinNodeT>();

            for (IndexT j = 0; j < mesh->skin.skinFragments.Size(); j++)
            {
                auto transform = std::make_unique<ToolkitUtil::TransformNodeInfoT>();
                transform->name = mesh->base.name;
                transform->translation = mesh->base.translation;
                transform->rotation = mesh->base.rotation;
                transform->scale = mesh->base.scale;

                auto shape = std::make_unique<ToolkitUtil::ShapeNodeT>();
                shape->use_lod = mesh->mesh.lodIndex != InvalidIndex;
                shape->lod_min = mesh->mesh.minLodDistance;
                shape->lod_max = mesh->mesh.maxLodDistance;
                shape->bbox_min = xyz(mesh->base.boundingBox.pmin);
                shape->bbox_max = xyz(mesh->base.boundingBox.pmax);
                shape->prim_group = mesh->skin.skinFragments[j];
                shape->mesh_resource = meshResource;
                shape->mesh_index = mesh->mesh.meshIndex;
                shape->material = mesh->mesh.material;
                shape->transform = std::move(transform);

                auto skinFragment = std::make_unique<ToolkitUtil::SkinFragmentNodeT>();
                const Util::Array<IndexT>& indices = mesh->skin.jointLookup[j].KeysAsArray();
                skinFragment->joints = std::vector<int32_t>(indices.Begin(), indices.End());
                skinFragment->shape = std::move(shape);
                skin->fragments.push_back(std::move(skinFragment));

                /*
                ModelConstants::SkinNode skinNode;
                skinNode.meshResource = meshResource;
                skinNode.meshIndex = mesh->mesh.meshIndex;
                String skinPath;
                skinNode.path.Format("%s/fragment_%d", nodePath.AsCharPtr(), j);
                skinNode.useLOD = mesh->mesh.lodIndex != InvalidIndex;
                if (skinNode.useLOD)
                {
                    skinNode.LODMax = mesh->mesh.maxLodDistance;
                    skinNode.LODMin = mesh->mesh.minLodDistance;
                }
                skinNode.transform.position = mesh->base.translation;
                skinNode.transform.rotation = mesh->base.rotation;
                skinNode.transform.scale = mesh->base.scale;
                skinNode.boundingBox = mesh->base.boundingBox;
                skinNode.name = mesh->base.name;
                skinNode.primitiveGroupIndex = mesh->skin.skinFragments[j];
                skinNode.fragmentJoints = mesh->skin.jointLookup[j].KeysAsArray();

                skinSetNode.skinFragments.Append(skinNode);

                //SetupDefaultState(skinNode.path, Util::String::Sprintf("%s/%s/%s", category.AsCharPtr(), file.AsCharPtr(), mesh->mesh.material.AsCharPtr()), attributes, true);
                */
            }

            //skinSets.Append(skinSetNode);
            model.skins.push_back(std::move(skin));
        }
        else
        {
            /*
            ModelConstants::ShapeNode shapeNode;
            shapeNode.meshResource = meshResource;
            shapeNode.meshIndex = mesh->mesh.meshIndex;
            shapeNode.path = nodePath;
            shapeNode.useLOD = mesh->mesh.lodIndex != InvalidIndex;
            if (shapeNode.useLOD)
            {
                shapeNode.LODMax = mesh->mesh.maxLodDistance;
                shapeNode.LODMin = mesh->mesh.minLodDistance;
            }
            shapeNode.transform.position = mesh->base.translation;
            shapeNode.transform.rotation = mesh->base.rotation;
            shapeNode.transform.scale = mesh->base.scale;
            shapeNode.boundingBox = mesh->base.boundingBox;
            shapeNode.name = mesh->base.name;
            shapeNode.primitiveGroupIndex = mesh->mesh.groupId;


            // add to constants
            shapes.Append(shapeNode);

            Util::String nodeMaterial = mesh->mesh.material;

            // If the material is a .sur material, we pass it as is.
            if ((nodeMaterial.Length() <= 4) || (!nodeMaterial.IsEmpty() && !nodeMaterial.EndsWithString(".sur")))
            {
                nodeMaterial = Util::String::Sprintf("%s/%s/%s", category.AsCharPtr(), file.AsCharPtr(), nodeMaterial.AsCharPtr());
            }

            //SetupDefaultState(shapeNode.path, nodeMaterial, attributes);
            */

            auto transform = std::make_unique<ToolkitUtil::TransformNodeInfoT>();
            transform->name = mesh->base.name;
            transform->translation = mesh->base.translation;
            transform->rotation = mesh->base.rotation;
            transform->scale = mesh->base.scale;

            auto shape = std::make_unique<ToolkitUtil::ShapeNodeT>();
            shape->use_lod = mesh->mesh.lodIndex != InvalidIndex;
            shape->lod_min = mesh->mesh.minLodDistance;
            shape->lod_max = mesh->mesh.maxLodDistance;
            shape->bbox_min = xyz(mesh->base.boundingBox.pmin);
            shape->bbox_max = xyz(mesh->base.boundingBox.pmax);
            shape->prim_group = mesh->mesh.groupId;
            shape->mesh_resource = meshResource;
            shape->mesh_index = mesh->mesh.meshIndex;
            shape->material = mesh->mesh.material;
            shape->transform = std::move(transform);
            model.shapes.push_back(std::move(shape));

        }
    }

    for (i = 0; i < characterNodes.Size(); i++)
    {
        // create character node
        /*
        ModelConstants::CharacterNode characterNode;
        characterNode.skeletonResource = skeletonRes;
        characterNode.skeletonIndex = characterNodes[i]->skeleton.skeletonIndex;
        characterNode.animationResource = animRes;
        characterNode.animationIndex = characterNodes[i]->anim.animIndex;
        characterNode.name = characterNodes[i]->base.name;
        // add character node to constants
        characters.Append(characterNode);
        */

        auto characterNodeInfo = std::make_unique<ToolkitUtil::CharacterNodeT>();
        characterNodeInfo->name = characterNodes[i]->base.name;
        characterNodeInfo->skeleton_resource = skeletonRes;
        characterNodeInfo->skeleton_index = characterNodes[i]->skeleton.skeletonIndex;
        characterNodeInfo->animation_resource = animRes;
        characterNodeInfo->animation_index = characterNodes[i]->anim.animIndex;
        model.characters.push_back(std::move(characterNodeInfo));
    }

    for (i = 0; i < physicsNodes.Size(); i++)
    {
        // get mesh
        const SceneNode* mesh = physicsNodes[i];

        // create full node path
        String nodePath;
        nodePath.Format("root/%s", mesh->base.name.AsCharPtr());

        auto transform = std::make_unique<ToolkitUtil::TransformNodeInfoT>();
        transform->name = mesh->base.name;
        transform->translation = mesh->base.translation;
        transform->rotation = mesh->base.rotation;
        transform->scale = mesh->base.scale;

        auto phys = std::make_unique<ToolkitUtil::PhysicsNodeT>();
        phys->transform = std::move(transform);
        phys->mesh_resource = physicsMeshRes;
        phys->material = "";
        model.physics.push_back(std::move(phys));

        // create physics node
        /*
        ModelConstants::PhysicsNode node;
        node.mesh = physicsMeshRes;
        node.path = nodePath;
        node.transform.position = mesh->base.translation;
        node.transform.rotation = mesh->base.rotation;
        node.transform.scale = mesh->base.scale;
        node.primitiveGroupIndex = mesh->mesh.groupId;
        node.name = mesh->base.name;

        // add to constants
        physics.Append(node);
        */
    }

    String modelFile;
    modelFile.Format("src:assets/%s/%s.namdl", category.AsCharPtr(), file.AsCharPtr());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(modelFile);
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::ModelResource>(model);
        stream->Write(data.GetPtr(), data.Size());

        stream->Close();
    }

    /*
    String modelFile;
    modelFile.Format("src:assets/%s/%s.model", category.AsCharPtr(), file.AsCharPtr());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(modelFile);
    stream->SetAccessMode(Stream::ReadAccess);
    stream->Open();

    if (stream->IsOpen())
    {
        // create memory stream
        Ptr<Stream> memStream = MemoryStream::Create();

        Ptr<IO::XmlWriter> writer = XmlWriter::Create();
        writer->SetStream(stream);
        writer->Open();

        // start with nebula tag
        writer->BeginNode("Nebula");

        // start model node
        writer->BeginNode("Model");

        // set name of model
        writer->SetString("name", scene->GetName());

        // set bounding box
        writer->SetVec4("bboxcenter", globalBoundingBox.center());
        writer->SetVec4("bboxextents", globalBoundingBox.extents());

        if (characters.Size() > 0)
        {
            // write content, start with character nodes
            writer->BeginNode("CharacterNodes");

            // go through shapes and write nodes
            IndexT i;
            for (i = 0; i < characters.Size(); i++)
            {

                // get shape node
                const ModelConstants::CharacterNode& node = characters[i];

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

        if (shapes.Size() > 0)
        {
            // write shape nodes
            writer->BeginNode("ShapeNodes");

            // go through shapes and write nodes
            for (const auto& shapeNode : shapes)
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

        if (physics.Size() > 0)
        {
            // begin physics nodes
            writer->BeginNode("PhysicsNodes");

            // go through and write out physics nodes
            IndexT i;
            for (i = 0; i < physicsNodes.Size(); i++)
            {
                // get physics node
                const ModelConstants::PhysicsNode& node = physics[i];

                // write physics node
                writer->BeginNode("PhysicsNode");

                // write data
                writer->SetString("name", node.name);
                writer->SetString("path", node.path);

                // set position
                writer->SetVec4("position", node.transform.position);

                // set rotation
                writer->SetVec4("rotation", node.transform.rotation.vec);

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

        // close model node
        writer->EndNode();

        // close nebula3 node
        writer->EndNode();

        // closes writer and file
        writer->Close();
        stream->Close();
    }
    */
}

//------------------------------------------------------------------------------
/**
*/
void 
SceneWriter::UpdateConstants(const Util::String & file, const Ptr<ToolkitUtil::ModelConstants>& constants)
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
SceneWriter::UpdateAttributes(const Util::String & file, const Ptr<ToolkitUtil::ModelAttributes>&attributes)
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
        attributes->Save(stream);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SceneWriter::UpdatePhysics(const Util::String & file, const Ptr<ToolkitUtil::ModelPhysics>&physics)
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
