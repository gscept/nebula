//------------------------------------------------------------------------------
//  assetpackager.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "assetpackager.h"
#include "io/ioserver.h"
#include "io/xmlwriter.h"
#include "toolkit-common/converters/binaryxmlconverter.h"
#include "toolkitutil/model/binarymodelwriter.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/audio.h"
#include "flat/mesh.h"
#include "flat/model.h"
#include "flat/material.h"
#include "flat/skeleton.h"
#include "flat/particle.h"
#include "flat/anim.h"
#include "flat/texture.h"
#include "flat/physics/actor.h"

#include "model/meshutil/meshbuildersaver.h"
#include "model/skeletonutil/skeletonbuildersaver.h"
#include "model/animutil/animbuildersaver.h"
#include "texutil/textureconverter.h"

#include "toolkit-common/text.h"
#include "toolkit-common/logger.h"

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
bool
PackageModel(
    const ToolkitUtil::SceneResourceT* model,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
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

    IO::URI output = Util::String::Sprintf("%s/%s.n3", destinationFolder.LocalPath().AsCharPtr(), fileName.AsCharPtr());
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
        writer->BeginModel("Model", 'MODL', model->name);

        // write number of masks
        writer->BeginTag("Number of masks", 'NJMS');
        writer->WriteInt((uint)model->joint_masks.size());
        writer->EndTag();

        // write joint mask
        for (const auto& jointMask : model->joint_masks)
        {
            writer->BeginTag("Joint mask", 'JOMS');
            writer->WriteString(jointMask->name);
            writer->WriteInt((uint)jointMask->weights.size());
            for (const auto weight : jointMask->weights)
            {
                writer->WriteFloat(weight);
            }
            writer->EndTag();
        }

        writer->BeginTag("Number of takes", 'NTKS');
        writer->WriteInt((uint)model->takes.size());
        writer->EndTag();

        // write take info
        for (const auto& take : model->takes)
        {
            writer->BeginTag("Number of clips", 'NTCL');
            writer->WriteInt((uint)take->clips.size());
            writer->EndTag();

            for (const auto& clip : take->clips)
            {
                writer->BeginTag("Clip info", 'CLIP');
                writer->WriteString(clip->name);
                writer->WriteFloat(clip->start);
                writer->WriteFloat(clip->end);
                writer->WriteInt(clip->pre_infinity);
                writer->WriteInt(clip->post_infinity);

                // write events
                writer->BeginTag("Number of clip events", 'NCEV');
                writer->WriteInt((uint)clip->events.size());
                writer->EndTag();

                for (const auto& event : clip->events)
                {
                    writer->BeginTag("Clip event", 'CEVT');
                    writer->WriteString(event->name);
                    writer->WriteFloat(event->time);
                    writer->EndTag();
                }
                writer->EndTag();
            }
        }

        writer->BeginModelNode("TransformNode", 'TRFN', "root");

            writer->BeginTag("Scene Bounding Box", 'LBOX');

            writer->WriteVec4(Math::vec4(model->bbox_min, 1));
            writer->WriteVec4(Math::vec4(model->bbox_max, 1));

            writer->EndTag();

            if (model->skins.size() > 0)
            {
                writer->BeginModelNode("CharacterSkinNode", 'CHSN', "character");
                writer->BeginTag("Number of skin fragments", 'NSKF');
                writer->WriteInt((uint)model->skins.size());
                writer->EndTag();

                // Write skins
                for (const auto& skinFragment : model->skins)
                {
                    // write the used skin fragments
                    writer->BeginTag("Used skin fragments", 'SFRG');
                    writer->WriteInt(skinFragment->shape->prim_group);

                    // write the used joints for the fragment
                    writer->WriteInt((uint)skinFragment->joints.size());

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
            for (const auto& shape : model->shapes)
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

        logger->Print("%s\n", Util::Format("Packaged model: %s", Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
PackageAnimation(
    const ToolkitUtil::AnimResourceT* anim,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
    IO::URI output = Util::String::Sprintf("%s/%s.nax", destinationFolder.LocalPath().AsCharPtr(), fileName.AsCharPtr());
    logger->Print("%s\n", Util::Format("Packaged animation: %s", Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    return ToolkitUtil::AnimBuilderSaver::SaveBinary(output, anim, Platform::Win32);
}

//------------------------------------------------------------------------------
/**
*/
bool
PackageSkeleton(
    const ToolkitUtil::SkeletonResourceT* skel,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
    IO::URI output = Util::String::Sprintf("%s/%s.nsk", destinationFolder.LocalPath().AsCharPtr(), fileName.AsCharPtr());
    logger->Print("%s\n", Util::Format("Packaged skeleton: %s", Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    return ToolkitUtil::SkeletonBuilderSaver::SaveBinary(output, skel, Platform::Win32);
}

//------------------------------------------------------------------------------
/**
*/
bool
PackageMesh(
    const ToolkitUtil::MeshResourceT* mesh,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
    IO::URI output = Util::String::Sprintf("%s/%s.nvx", destinationFolder.LocalPath().AsCharPtr(), fileName.AsCharPtr());
    logger->Print("%s\n", Util::Format("Packaged mesh: %s", Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    return ToolkitUtil::MeshBuilderSaver::SaveBinary(output, mesh, Platform::Win32);
}

//------------------------------------------------------------------------------
/**
*/
bool
PackageTexture(
    const ToolkitUtil::TextureResourceT* tex,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
    static const bool IsExportedFormat[] = {
        false, // Unknown
        false, // JPG
        false, // TGA
        false, // BMP
        true,  // DDS
        false, // PNG
        false, // EXR
        false, // TIF
        false   // CUBE (assumed to be in DDS format)
    };

    // If already in an 'exported' format, just copy over
    if (IsExportedFormat[tex->container])
    {
        IO::CreateDirectory(destinationFolder);
        IO::URI destFile = Util::Format("%s/%s.dds", destinationFolder.LocalPath().AsCharPtr(), fileName.AsCharPtr());
        Ptr<IO::BinaryWriter> writer = IO::BinaryWriter::Create();
        writer->SetStream(IO::IoServer::Instance()->CreateStream(destFile));
        if (writer->Open())
        {
            writer->WriteRawData(tex->data.data(), (uint)tex->data.size());
            writer->Close();
        }
        else
        {
            return false;
        }
    }
    else
    {
        IO::CreateDirectory("temp:texturepackager");
        const Util::String tmpFile = Util::Format("temp:texturepackager/%s", fileName.AsCharPtr());

        // Create intermediate file for texture conversion
        Ptr<IO::BinaryWriter> writer = IO::BinaryWriter::Create();
        writer->SetStream(IO::IoServer::Instance()->CreateStream(tmpFile));
        if (writer->Open())
        {
            writer->WriteRawData(tex->data.data(), (uint)tex->data.size());
            writer->Close();
        }
        else
        {
            return false;
        }
        Util::String platformExtension;
        if (platform == ToolkitUtil::Platform::Win32 || platform == ToolkitUtil::Platform::Linux)
        {
            platformExtension = "dds";
        }
        IO::URI output = Util::String::Sprintf("%s/%s.%s", destinationFolder.LocalPath().AsCharPtr(), fileName.AsCharPtr(), platformExtension.AsCharPtr());
        logger->Print("%s\n",
                        Util::Format("Packaged texture: %s",
                                     Text(output.LocalPath()).Color(TextColor::Blue).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());

        TextureConversionInfo info;
        info.cube = tex->container == ToolkitUtil::TextureContainer_CUBE;
        info.logger = logger;
        info.destPath = destinationFolder.LocalPath();
        info.sourcePath = tmpFile;
        info.tmpDir = "temp:texturepackager";
        info.texture = tex;
        return ConvertTexture(info);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
PackageAudio(
    const ToolkitUtil::AudioResourceT* audio,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
PackageMaterial(
    const ToolkitUtil::MaterialResourceT* mat,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
    Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);

    Ptr<IO::XmlWriter> writer = IO::XmlWriter::Create();
    writer->SetStream(stream);
    writer->Open();

    writer->BeginNode("Nebula");
    {
        writer->BeginNode("Surface");
        writer->SetString("template", mat->template_name);
        writer->BeginNode("Params");
        for (IndexT i = 0; i < mat->value_names.size(); i++)
        {
            writer->BeginNode(mat->value_names[i]);
            writer->SetString("value", mat->values[i]);
            writer->EndNode();
        }
        writer->EndNode();
        writer->EndNode();
    }
    writer->EndNode();
    writer->Close();

    stream->SetAccessMode(IO::Stream::AccessMode::ReadAccess);

    ToolkitUtil::BinaryXmlConverter converter;
    Util::String fileNameNoExt = fileName;
    fileNameNoExt.StripFileExtension();
    IO::URI output = Util::String::Sprintf("%s/%s.sur", destinationFolder.LocalPath().AsCharPtr(), fileNameNoExt.AsCharPtr());
    logger->Print("%s\n",
                Util::Format("Packaged material: %s",
                             Text(output.LocalPath()).Color(TextColor::Blue).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    return converter.ConvertStream(stream, output.LocalPath(), *logger);
}

//------------------------------------------------------------------------------
/**
*/
bool
PackageParticle(
    const ToolkitUtil::ParticleResourceT* par,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
PackagePhysics(
    const ToolkitUtil::PhysicsResourceT* phy,
    const Util::String& fileName,
    const IO::URI& destinationFolder,
    ToolkitUtil::Platform::Code platform,
    ToolkitUtil::Logger* logger
)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageTextureFile(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::CreateDirectory(destinationFolder);
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::TextureResourceT tex;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::TextureResource>(tex, (const uint8_t*)data);

        Util::String fileNameNoExt = file.LocalPath().ExtractFileName();
        fileNameNoExt.StripFileExtension();

        return PackageTexture(&tex, fileNameNoExt, destinationFolder, platform, logger);
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageAudioFile(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::CreateDirectory(destinationFolder);
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::AudioResourceT aud;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::AudioResource>(aud, (const uint8_t*)data);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageMaterialFile(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::CreateDirectory(destinationFolder);
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::MaterialResourceT mat;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::MaterialResource>(mat, (const uint8_t*)data);
        return PackageMaterial(&mat, file.LocalPath().ExtractFileName(), destinationFolder, platform, logger);
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PackageParticleFile(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        IO::CreateDirectory(destinationFolder);
        IO::Stream::Size size = stream->GetSize();
        const void* data = stream->MemoryMap();

        ToolkitUtil::ParticleResourceT par;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ParticleResource>(par, (const uint8_t*)data);
        return true;
    }
    return false;
}

} // namespace ToolkitUtil