//------------------------------------------------------------------------------
//  ngltfscenewriter.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfscenewriter.h"
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
__ImplementClass(ToolkitUtil::NglTFSceneWriter, 'gltw', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
NglTFSceneWriter::NglTFSceneWriter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
NglTFSceneWriter::~NglTFSceneWriter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Generates model files, the path provided should be the path to the folder
*/
void
NglTFSceneWriter::GenerateModels(const String& basePath, const ExportFlags& flags, const ExportMode& mode)
{
    // create N3 model writer and set it up
    Ptr<N3Writer> n3Writer = N3Writer::Create();
    Ptr<XmlModelWriter> modelWriter = XmlModelWriter::Create();
    n3Writer->SetModelWriter(modelWriter.upcast<ModelWriter>());

    // make sure category directory exists
    IoServer::Instance()->CreateDirectory(basePath);

    const Array<Ptr<NglTFMesh> >& meshes = this->scene->GetMeshNodes();
    if (mode == Static)
    {
        // create merged model, will basically go through every node and add them into ONE .xml file
        this->CreateStaticModel(n3Writer, meshes, basePath + this->scene->GetName() + ".xml");
    }
    else
    {
        n_warning("ERROR: Animated and skinned GLTF models are not yet supported!\n");
        // create skeletal model
        //this->CreateSkeletalModel(n3Writer, meshes, basePath + this->scene->GetName() + ".xml");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFSceneWriter::CreateStaticModel(const Ptr<ToolkitUtil::N3Writer>& modelWriter, const Util::Array<Ptr<NglTFMesh>>& meshes, const Util::String& path)
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

#if PHYSEXPORT
    // get physics
    Ptr<ModelPhysics> physics = ModelDatabase::Instance()->LookupPhysics(fileCat);

    if (scene->GetPhysicsMesh()->GetNumTriangles() > 0)
    {
        Util::String phname;
        phname.Format("phymsh:%s_ph.nvx2", fileCat.AsCharPtr());
        physics->SetPhysicsMesh(phname);
}
#endif
    // clear constants
    constants->Clear();

    // loop through bounding boxes and get our scene bounding box
    Math::bbox globalBox;
    const Util::Array<Ptr<NglTFMesh> >& meshNodes = meshes;
    IndexT meshIndex;
    globalBox.begin_extend();
    for (meshIndex = 0; meshIndex < meshNodes.Size(); meshIndex++)
    {
        Ptr<NglTFMesh> mesh = meshNodes[meshIndex];
        globalBox.extend(mesh->GetBoundingBox());
    }
    globalBox.end_extend();

    // set global box for constants
    constants->SetGlobalBoundingBox(globalBox);

    IndexT i;
    for (i = 0; i < meshes.Size(); i++)
    {
        // get mesh node
        const Ptr<NglTFMesh>& mesh = meshes[i];

        // create mesh name
        String meshPath;
        meshPath.Format("msh:%s/%s.nvx2", category.AsCharPtr(), scene->GetName().AsCharPtr());

        for (IndexT gid = 0; gid < mesh->Primitives().Size(); gid++)
        {
            MeshPrimitive const& primitive = mesh->Primitives()[gid];

            // create full node path
            String nodePath;
            nodePath.Format("root/%s", primitive.name.AsCharPtr());

            // create and add a shape node
            ModelConstants::ShapeNode shapeNode;
            shapeNode.mesh = meshPath;
            shapeNode.path = nodePath;
            shapeNode.transform.position = mesh->GetInitialPosition();
            shapeNode.transform.rotation = mesh->GetInitialRotation();
            shapeNode.transform.scale = mesh->GetInitialScale().vec;
            shapeNode.boundingBox = primitive.boundingBox;
            shapeNode.name = primitive.name;
            shapeNode.type = scene->GetSceneFeatureString();
            shapeNode.primitiveGroupIndex = primitive.group.GetGroupId();

            // add to constants
            constants->AddShapeNode(primitive.name, shapeNode);

            // get state from attributes
            State state;

            // retrieve state if exists
            if (!this->force && attributes->HasState(nodePath))
            {
                state = attributes->GetState(nodePath);
            }

            // set material of state
            if (force || !state.material.IsValid())
            {
                state.material = this->surExportPath + "/" + primitive.material;
            }

            //// set state for attributes
            attributes->SetState(nodePath, state);
        }
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

#if PHYSEXPORT
    // update
    this->UpdatePhysics(physicsFile, physics);
#endif

    // create model builder
    Ptr<ModelBuilder> modelBuilder = ModelBuilder::Create();

    // set constants and attributes
    modelBuilder->SetConstants(constants);
    modelBuilder->SetAttributes(attributes);
#if PHYSEXPORT  
    modelBuilder->SetPhysics(physics);
#endif
    // format name of model
    String destination;
    destination.Format("mdl:%s/%s.n3", category.AsCharPtr(), file.AsCharPtr());

    // save file
    modelBuilder->SaveN3(destination, this->platform);

#if PHYSEXPORT
    // save physics
    destination.Format("phys:%s/%s.actor", category.AsCharPtr(), file.AsCharPtr());
    modelBuilder->SaveN3Physics(destination, this->platform);
#endif  
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFSceneWriter::UpdateConstants(const Util::String& file, const Ptr<ModelConstants>& constants)
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
NglTFSceneWriter::UpdateAttributes(const Util::String& file, const Ptr<ModelAttributes>& attributes)
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

#if PHYSEXPORT
//------------------------------------------------------------------------------
/**
*/
void
NFbxSceneWriter::UpdatePhysics(const Util::String& file, const Ptr<ModelPhysics>& physics)
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
#endif
} // namespace ToolkitUtil
