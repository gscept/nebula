//------------------------------------------------------------------------------
//  assetpackager.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "assetpackager.h"
#include "io/ioserver.h"
#include "io/xmlwriter.h"
#include "toolkitutil/converters/binaryxmlconverter.h"
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

#include "toolkitutil/text.h"
#include "toolkitutil/logger.h"
#include "system/process.h"

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
        writer->BeginTag("JointMasks", 'JOMS');
        writer->WriteInt((uint)model->joint_masks.size());

        // write joint mask
        for (const auto& jointMask : model->joint_masks)
        {
            writer->WriteString(jointMask->name);
            writer->WriteInt((uint)jointMask->weights.size());
            for (const auto weight : jointMask->weights)
            {
                writer->WriteFloat(weight);
            }
        }

        writer->EndTag();

        writer->BeginTag("Takes", 'TAKE');
        writer->WriteInt((uint)model->takes.size());

        // write take info
        for (const auto& take : model->takes)
        {
            writer->WriteInt((uint)take->clips.size());

            for (const auto& clip : take->clips)
            {
                writer->WriteString(clip->name);
                writer->WriteFloat(clip->start);
                writer->WriteFloat(clip->end);
                writer->WriteInt(clip->pre_infinity);
                writer->WriteInt(clip->post_infinity);

                // write events
                writer->WriteInt((uint)clip->events.size());

                for (const auto& event : clip->events)
                {
                    writer->WriteString(event->name);
                    writer->WriteFloat(event->time);
                }
            }
        }
        writer->EndTag();

        writer->BeginModelNode("TransformNode", 'TRFN', "root");

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
    output.SetScheme("safefile");
    bool res = ToolkitUtil::AnimBuilderSaver::SaveBinary(output, anim, Platform::Win32);

    if (res)
        logger->Print("%s\n", Util::Format("Packaged animation: %s", Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    else
        logger->Print("%s\n", Util::Format("%s %s", Text("Failed to package animation").Color(TextColor::Red).AsCharPtr(), Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    return res;
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
    output.SetScheme("safefile");
    bool res = ToolkitUtil::SkeletonBuilderSaver::SaveBinary(output, skel, Platform::Win32);
    if (res)
        logger->Print("%s\n", Util::Format("Packaged skeleton: %s", Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    else
        logger->Print("%s\n", Util::Format("%s %s", Text("Failed to package skeleton").Color(TextColor::Red).AsCharPtr(), Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    return res;
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
    output.SetScheme("safefile");
    bool res = ToolkitUtil::MeshBuilderSaver::SaveBinary(output, mesh, Platform::Win32);
    if (res)
        logger->Print("%s\n", Util::Format("Packaged mesh: %s", Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    else
        logger->Print("%s\n", Util::Format("%s %s", Text("Failed to package mesh").Color(TextColor::Red).AsCharPtr(), Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    return res;
}

// converters and texassemble pick the image loader from the file extension
static const char* ContainerExtension[] = {
    "",    // Unknown
    "jpg", // JPG
    "tga", // TGA
    "bmp", // BMP
    "dds", // DDS
    "png", // PNG
    "exr", // EXR
    "tif", // TIF
    "dds", // CUBE
    "hdr"  // HDR
};

//------------------------------------------------------------------------------
/**
*/
static bool
WriteTextureFile(const IO::URI& path, const std::vector<uint8_t>& data)
{
    Ptr<IO::BinaryWriter> writer = IO::BinaryWriter::Create();
    writer->SetStream(IO::IoServer::Instance()->CreateStream(path));
    if (writer->Open())
    {
        writer->WriteRawData(data.data(), (uint)data.size());
        writer->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    texassemble loads PNG and JPG faces with sRGB formats on Linux (and on Windows when the
    file says so), which makes the converters linearize textures with a linear color space.
    Relabels the assembled DDS as UNORM, the texture's color space decides like for every
    other texture.
*/
static bool
StripSRGBFormat(const IO::URI& dds)
{
    static const uint32_t SRGBFormats[][2] = {
        { 29, 28 }, // R8G8B8A8_UNORM_SRGB
        { 72, 71 }, // BC1_UNORM_SRGB
        { 75, 74 }, // BC2_UNORM_SRGB
        { 78, 77 }, // BC3_UNORM_SRGB
        { 91, 87 }, // B8G8R8A8_UNORM_SRGB
        { 93, 88 }, // B8G8R8X8_UNORM_SRGB
        { 99, 98 }, // BC7_UNORM_SRGB
    };
    // magic and DDS_HEADER, followed by DDS_HEADER_DXT10 if the pixel format's fourcc is DX10
    const SizeT FourCCOffset = 84, DXGIFormatOffset = 128;

    std::vector<uint8_t> data;
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(dds);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        return false;
    }
    data.resize(stream->GetSize());
    memcpy(data.data(), stream->MemoryMap(), stream->GetSize());
    stream->MemoryUnmap();
    stream->Close();

    if (data.size() < DXGIFormatOffset + 4 || memcmp(&data[FourCCOffset], "DX10", 4) != 0)
    {
        return true;
    }
    uint32_t format;
    memcpy(&format, &data[DXGIFormatOffset], 4);
    for (const auto& [srgb, unorm] : SRGBFormats)
    {
        if (format == srgb)
        {
            memcpy(&data[DXGIFormatOffset], &unorm, 4);
            return WriteTextureFile(dds, data);
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Assembles the source images of a cube texture into an uncompressed DDS cube
    with texassemble, which the converters then compress like any other DDS cube.
    The face images have to be readable by texassemble on the batching platform,
    PNG, JPG, TGA, HDR and DDS work everywhere, EXR only on Linux, BMP and TIF only on Windows.
*/
static bool
AssembleCubeTexture(
    const ToolkitUtil::TextureResourceT* tex,
    const Util::String& fileName,
    const Util::String& output,
    ToolkitUtil::Logger* logger
)
{
    static const char* LayoutCommand[] = {
        "",                 // None
        "cube",             // Faces
        "cube-from-hc",     // HorizontalCross
        "cube-from-vc",     // VerticalCross
        "cube-from-vc-fnz", // VerticalCrossFlipNegZ
        "cube-from-ht",     // HorizontalTee
        "cube-from-hs",     // HorizontalStrip
        "cube-from-vs"      // VerticalStrip
    };

    Util::Array<Util::String> inputs;
    bool written = true;
    if (tex->cube_layout == ToolkitUtil::TextureCubeLayout_Faces)
    {
        if (tex->faces.size() != 6)
        {
            logger->Error("Cube texture '%s' needs 6 faces, but has %d\n", fileName.AsCharPtr(), (int)tex->faces.size());
            return false;
        }
        for (IndexT i = 0; i < 6; i++)
        {
            inputs.Append(Util::Format("temp:texturepackager/%s_face%d.%s", fileName.AsCharPtr(), i, ContainerExtension[tex->faces[i]->container]));
            written &= WriteTextureFile(inputs.Back(), tex->faces[i]->data);
        }
    }
    else
    {
        // single cross, tee or strip image
        inputs.Append(Util::Format("temp:texturepackager/%s_cube.%s", fileName.AsCharPtr(), ContainerExtension[tex->container]));
        written &= WriteTextureFile(inputs.Back(), tex->data);
    }

    bool res = false;
    if (written)
    {
        // '--' ends the options, texassemble would take absolute linux paths for options otherwise
        Util::String args = Util::Format(" %s -y -nologo -o \"%s\" --", LayoutCommand[tex->cube_layout], IO::IoServer::NativePath(IO::URI(output).LocalPath()).AsCharPtr());
        for (const Util::String& input : inputs)
        {
            args.Append(" \"");
            args.Append(IO::IoServer::NativePath(IO::URI(input).LocalPath()));
            args.Append("\"");
        }

        System::ProcessStartInfo startInfo;
        startInfo.args = args;
        startInfo.workingDir = "temp:texturepackager";
        startInfo.exePath = Util::Format("toolkit:bin/%s/texassemble", System::PlatformTypeAsString(System::Platform));
        startInfo.consoleWindow = false;

        System::ProcessId process = System::StartProcess(startInfo);
        if (process == System::InvalidProcessId)
        {
            logger->Warning("Failed to launch texassemble '%s'!\n", startInfo.exePath.LocalPath().AsCharPtr());
        }
        else
        {
            uint exitCode = System::WaitForProcess(process);
            res = exitCode == 0;
            if (!res)
                logger->Error("texassemble failed to assemble cube texture '%s', exit code: %d\n", fileName.AsCharPtr(), exitCode);
            else
                res = StripSRGBFormat(output);
        }
    }

    for (const Util::String& input : inputs)
    {
        IO::IoServer::Instance()->DeleteFile(input);
    }
    return res;
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
        false, // CUBE (assumed to be in DDS format)
        false  // HDR
    };
    const bool assembleCube = tex->cube_layout != ToolkitUtil::TextureCubeLayout_None;

    // If already in an 'exported' format, just copy over
    if (IsExportedFormat[tex->container] && !assembleCube)
    {
        IO::CreateDirectory(destinationFolder);
        IO::URI destFile = Util::Format("%s/%s.dds", destinationFolder.LocalPath().AsCharPtr(), fileName.AsCharPtr());
        logger->Print("%s", destFile.LocalPath().AsCharPtr());
        return WriteTextureFile(destFile, tex->data);
    }
    else
    {
        IO::CreateDirectory("temp:texturepackager");
        const Util::String tmpFile = Util::Format("temp:texturepackager/%s.%s", fileName.AsCharPtr(), assembleCube ? "dds" : ContainerExtension[tex->container]);

        // Create intermediate file for texture conversion
        bool written = false;
        if (assembleCube)
        {
            written = AssembleCubeTexture(tex, fileName, tmpFile, logger);
        }
        else
        {
            written = WriteTextureFile(tmpFile, tex->data);
        }
        if (!written)
        {
            IO::IoServer::Instance()->DeleteFile(tmpFile);
            logger->Print("%s\n", Util::Format("%s %s", Text("Failed to package texture:").Color(TextColor::Red).AsCharPtr(), Text(fileName).Color(TextColor::Blue).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
            return false;
        }

        Util::String platformExtension;
        if (platform == ToolkitUtil::Platform::Win32 || platform == ToolkitUtil::Platform::Linux)
        {
            platformExtension = "dds";
        }
        IO::URI output = Util::String::Sprintf("%s/%s.%s", destinationFolder.LocalPath().AsCharPtr(), fileName.AsCharPtr(), platformExtension.AsCharPtr());
        output.SetScheme("safefile");

        // cube maps (CUBE and assembled ones) are DDS cube textures, both converters keep them as cubes
        TextureConversionInfo info;
        info.logger = logger;
        info.destPath = destinationFolder.LocalPath();
        info.sourcePath = tmpFile;
        info.tmpDir = "temp:texturepackager";
        info.texture = tex;
        bool res = ConvertTexture(info);
        IO::IoServer::Instance()->DeleteFile(tmpFile);
        if (res)
            logger->Print("%s\n", Util::Format("Packaged texture: %s", Text(output.LocalPath()).Color(TextColor::Blue).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
        else
            logger->Print("%s\n", Util::Format("%s %s", Text("Failed to package texture:").Color(TextColor::Red).AsCharPtr(), Text(output.LocalPath()).Color(TextColor::Blue).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
        return res;
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
    IO::URI output = Util::String::Sprintf("%s/%s.mat", destinationFolder.LocalPath().AsCharPtr(), fileNameNoExt.AsCharPtr());
    output.SetScheme("safefile");
    bool res = converter.ConvertStream(stream, output.LocalPath(), *logger);
    if (res)
        logger->Print("%s\n", Util::Format("Packaged material: %s", Text(output.LocalPath()).Color(TextColor::Blue).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    else
        logger->Print("%s\n", Util::Format("%s %s", Text("Failed to package material:").Color(TextColor::Red).AsCharPtr(), Text(output.LocalPath()).Color(TextColor::Blue).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    return res;
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

        stream->MemoryUnmap();
        stream->Close();

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

        stream->MemoryUnmap();
        stream->Close();

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

        stream->MemoryUnmap();
        stream->Close();

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

        stream->MemoryUnmap();
        stream->Close();

        return true;
    }
    return false;
}

} // namespace ToolkitUtil