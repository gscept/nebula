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
    , const Util::String& physicsMeshResource
    , const ToolkitUtil::ExportFlags& flags
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

    // get constant model set
    Ptr<ModelConstants> constants = ModelDatabase::Instance()->LookupConstants(fileCat);

    // get model attributes
    Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(fileCat);

    // get physics
    Ptr<ModelPhysics> physics = ModelDatabase::Instance()->LookupPhysics(fileCat);

    if (!physicsMeshResource.IsEmpty())
    {
        physics->SetPhysicsMesh(physicsMeshResource);
    }

    // clear constants
    constants->Clear();

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

    // set global box for constants
    constants->SetGlobalBoundingBox(globalBox);

    CreateModel(
        file
        , scene
        , category
        , constants
        , attributes
        , physics
        , graphicsNodes
        , physicsNodes
        , characterNodes
    );

    String constantsFile;
    constantsFile.Format("src:assets/%s/%s.constants", category.AsCharPtr(), file.AsCharPtr());

    // update
    UpdateConstants(constantsFile, constants);

    // format attributes file
    String attributesFile;
    attributesFile.Format("src:assets/%s/%s.attributes", category.AsCharPtr(), file.AsCharPtr());

    // update
    UpdateAttributes(attributesFile, attributes);

    // format attributes file
    String physicsFile;
    physicsFile.Format("src:assets/%s/%s.physics", category.AsCharPtr(), file.AsCharPtr());

    // update
    UpdatePhysics(physicsFile, physics);

    // Once we're done updating the XML intermediates, we should export to our binary N3
    Ptr<ModelBuilder> modelBuilder = ModelBuilder::Create();

    // set constants and attributes
    modelBuilder->SetConstants(constants);
    modelBuilder->SetAttributes(attributes);
    modelBuilder->SetPhysics(physics);

    // format name of model
    String destination;
    destination.Format("mdl:%s/%s.n3", category.AsCharPtr(), file.AsCharPtr());

    // save file
    modelBuilder->SaveN3(destination, platform);

    // save physics
    destination.Format("phys:%s/%s.actor", category.AsCharPtr(), file.AsCharPtr());
    modelBuilder->SaveN3Physics(destination, platform);
}

//------------------------------------------------------------------------------
/**
*/
void 
SetupDefaultState(
    const Util::String& nodePath
    , const Ptr<ModelAttributes>& attributes    
)
{
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

    // set state for attributes
    attributes->SetState(nodePath, state);
}

//------------------------------------------------------------------------------
/**
*/
void 
SceneWriter::CreateModel(
    const Util::String& file
    , const Scene* scene
    , const Util::String& category
    , const Ptr<ModelConstants>& constants
    , const Ptr<ModelAttributes>& attributes
    , const Ptr<ModelPhysics>& physics
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

            for (IndexT j = 0; j < mesh->skin.skinFragments.Size(); j++)
            {
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

                SetupDefaultState(skinNode.path, attributes);
            }

            constants->AddSkinSetNode(skinSetNode);
        }
        else
        {
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
            constants->AddShapeNode(shapeNode);

            SetupDefaultState(shapeNode.path, attributes);
        }
    }

    for (i = 0; i < characterNodes.Size(); i++)
    {
        // create character node
        ModelConstants::CharacterNode characterNode;
        characterNode.skeletonResource = skeletonRes;
        characterNode.skeletonIndex = characterNodes[i]->skeleton.skeletonIndex;
        characterNode.animationResource = animRes;
        characterNode.animationIndex = characterNodes[i]->anim.animIndex;
        characterNode.name = characterNodes[i]->base.name;

        // add character node to constants
        constants->AddCharacterNode(characterNode.name, characterNode);
    }

    for (i = 0; i < physicsNodes.Size(); i++)
    {
        // get mesh
        const SceneNode* mesh = physicsNodes[i];

        // create full node path
        String nodePath;
        nodePath.Format("root/%s", mesh->base.name.AsCharPtr());

        // create physics node
        ModelConstants::PhysicsNode node;
        node.mesh = physicsMeshRes;
        node.path = nodePath;
        node.transform.position = mesh->base.translation;
        node.transform.rotation = mesh->base.rotation;
        node.transform.scale = mesh->base.scale;
        node.primitiveGroupIndex = mesh->mesh.groupId;
        node.name = mesh->base.name;

        // add to constants
        constants->AddPhysicsNode(mesh->base.name, node);
    }
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
