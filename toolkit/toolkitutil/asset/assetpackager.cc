//------------------------------------------------------------------------------
//  assetpackager.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "assetpackager.h"
#include "io/ioserver.h"
#include "toolkitutil/model/binarymodelwriter.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/mesh.h"
#include "flat/model.h"
#include "flat/skeleton.h"
#include "flat/anim.h"
#include "flat/texture.h"

#include "model/meshutil/meshbuildersaver.h"
#include "model/skeletonutil/skeletonbuildersaver.h"
#include "model/animutil/animbuildersaver.h"
#include "texutil/textureconverter.h"

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
bool 
PackageModel(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::ModelResourceT model;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ModelResource>(model, (const uint8_t*)data);

        auto writeTransform = [](Ptr<BinaryModelWriter>& writer, const std::unique_ptr<ToolkitUtil::TransformNodeInfoT>& transform)
        {
            writer->BeginTag("Transform Position", 'POSI');
            writer->WriteVec4(Math::vec4(transform->translation, 1));
            writer->EndTag();

            writer->BeginTag("Transform Rotation", 'ROTN');
            writer->WriteVec4(transform->rotation.vec);
            writer->EndTag();

            writer->BeginTag("Transform Scale", 'SCAL');
            writer->WriteVec4(Math::vec4(transform->scale, 1));
            writer->EndTag();
        };

        auto writeShape = [](Ptr<BinaryModelWriter>& writer, const std::unique_ptr<ToolkitUtil::ShapeNodeT>& shape)
        {
            writer->BeginTag("Mesh Resource", 'MESH');
            writer->WriteString(shape->mesh_resource);
            writer->EndTag();

            writer->BeginTag("Mesh Index", 'MSHI');
            writer->WriteInt(shape->mesh_index);
            writer->EndTag();

            // write primitive group index
            writer->BeginTag("Primitive Group Index", 'PGRI');
            writer->WriteInt(shape->prim_group);
            writer->EndTag();

            // write material
            writer->BeginTag("Material", 'MATE');
            writer->WriteString(shape->material);
            writer->EndTag();

            // write bounding box
            writer->BeginTag("Bounding Box", 'LBOX');
            writer->WriteVec4(Math::vec4(shape->bbox_min, 1));
            writer->WriteVec4(Math::vec4(shape->bbox_max, 1));
            writer->EndTag();
        };

        IO::URI output = Util::String::Sprintf("%s/%s.n3", destinationFolder.LocalPath().AsCharPtr(), file.LocalPath().ExtractFileName().AsCharPtr());
        Ptr<IO::Stream> modelStream = IO::IoServer::Instance()->CreateStream(output);
        modelStream->SetAccessMode(IO::Stream::WriteAccess);
        if (modelStream->Open())
        {
            // create binary writer
            Ptr<BinaryModelWriter> writer = BinaryModelWriter::Create();
            writer->SetPlatform(platform);
            writer->SetStream(modelStream);

            writer->Open();

            // begin model
            writer->BeginModel("Model", 'MODL', model.name);

            writer->BeginModelNode("TransformNode", 'TRFN', "root");

                writer->BeginTag("Scene Bounding Box", 'LBOX');

                writer->WriteVec4(Math::vec4(model.bbox_min, 1));
                writer->WriteVec4(Math::vec4(model.bbox_max, 1));

                writer->EndTag();

                // Write characters
                if (model.characters.size() > 0)
                {
                    const auto& jointMasks = model.joint_masks;
                    if (jointMasks.size() > 0)
                    {
                        // write number of masks
                        writer->BeginTag("Number of masks", 'NJMS');
                        writer->WriteInt(jointMasks.size());
                        writer->EndTag();

                        // write joint mask
                        for (const auto& jointMask : jointMasks)
                        {
                            writer->BeginTag("Joint mask", 'JOMS');
                            writer->WriteString(jointMask->name);
                            writer->WriteInt(jointMask->weights.size());
                            for (const auto weight : jointMask->weights)
                            {
                                writer->WriteFloat(weight);
                            }
                            writer->EndTag();
                        }
                    }
                }

                // Write skins
                for (const auto& skinSet : model.skins)
                {
                    // get name of skin
                    const Util::String& name = skinSet->shape->transform->name;

                    writer->BeginModelNode("CharacterSkinNode", 'CHSN', name);
                    writer->BeginTag("Number of skin fragments", 'NSKF');
                    writer->WriteInt(skinSet->fragments.size());
                    writer->EndTag();

                    for (const auto& skinFragment : skinSet->fragments)
                    {
                        // write the used skin fragments
                        writer->BeginTag("Used skin fragments", 'SFRG');
                        writer->WriteInt(skinFragment->shape->prim_group);

                        // write the used joints for the fragment
                        writer->WriteInt(skinFragment->joints.size());

                        IndexT j;
                        for (j = 0; j < skinFragment->joints.size(); j++)
                        {
                            writer->WriteInt(skinFragment->joints[j]);
                        }
                        writer->EndTag();

                        writeTransform(writer, skinFragment->shape->transform);
                        writeShape(writer, skinFragment->shape);
                    }

                    // end skin node
                    writer->EndModelNode();
                }

                // Write shapes
                for (const auto& shape : model.shapes)
                {
                    // then create actual model with shape node
                    writer->BeginModelNode("ShapeNode", 'SPND', shape->transform->name);

                    writeTransform(writer, shape->transform);
                    writeShape(writer, shape);

                    if (shape->use_lod)
                    {
                        writer->BeginTag("LODMinDistance", 'SMID');
                        writer->WriteFloat(shape->lod_min);
                        writer->EndTag();

                        writer->BeginTag("LODMaxDistance", 'SMAD');
                        writer->WriteFloat(shape->lod_max);
                        writer->EndTag();
                    }

                    writer->EndModelNode();
                }

            // End root node
            writer->EndModelNode(); 

            // end name
            writer->EndModel();

            writer->Close();

            modelStream->Close();
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageAnimation(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::AnimResourceT anim;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::AnimResource>(anim, (const uint8_t*)data);

        IO::URI output = Util::String::Sprintf("%s/%s.n3", destinationFolder.LocalPath().AsCharPtr(), file.LocalPath().ExtractFileName().AsCharPtr());
        return ToolkitUtil::AnimBuilderSaver::SaveBinary(output, &anim, Platform::Win32);
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageSkeleton(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::SkeletonResourceT skeleton;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::SkeletonResource>(skeleton, (const uint8_t*)data);

        IO::URI output = Util::String::Sprintf("%s/%s.n3", destinationFolder.LocalPath().AsCharPtr(), file.LocalPath().ExtractFileName().AsCharPtr());
        return ToolkitUtil::SkeletonBuilderSaver::SaveBinary(output, &skeleton, Platform::Win32);
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageMesh(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::MeshResourceT mesh;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::MeshResource>(mesh, (const uint8_t*)data);

        IO::URI output = Util::String::Sprintf("%s/%s.n3", destinationFolder.LocalPath().AsCharPtr(), file.LocalPath().ExtractFileName().AsCharPtr());
        return ToolkitUtil::MeshBuilderSaver::SaveBinary(output, &mesh, Platform::Win32);
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageTexture(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::TextureResourceT tex;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::TextureResource>(tex, (const uint8_t*)data);

        IO::IoServer::Instance()->CreateDirectory("temp:texturepackager");
        const Util::String tmpFile = Util::String::Sprintf("temp:texturepackager/%d", file.LocalPath().HashCode());

        // Create intermediate file for texture conversion
        Ptr<IO::BinaryWriter> writer = IO::BinaryWriter::Create();
        writer->SetStream(IO::IoServer::Instance()->CreateStream(tmpFile));
        if (writer->Open())
        {
            writer->WriteRawData(tex.data.data(), tex.data.size());
            writer->Close();
        }
        else
        {
            return false;
        }
        ToolkitUtil::TextureConverter textureExporter;
        Util::String platformExtension;
        if (platform == ToolkitUtil::Platform::Win32 || platform == ToolkitUtil::Platform::Linux)
        {
            platformExtension = "dds";
        }
        IO::URI output = Util::String::Sprintf("%s/%s.%s", destinationFolder.LocalPath().AsCharPtr(), file.LocalPath().ExtractFileName().AsCharPtr(), platformExtension.AsCharPtr());
        if (tex.container == ToolkitUtil::TextureContainer_CUBE)
        {
            return textureExporter.ConvertCubemap(tmpFile, output.LocalPath(), "tmp:", &tex);
        }
        else
        {
            return textureExporter.ConvertTexture(tmpFile, output.LocalPath(), "tmp:", &tex);
        }
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageAudio(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageMaterial(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageParticle(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackagePhysics(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    return false;
}

} // namespace ToolkitUtil