//------------------------------------------------------------------------------
//  assetimporter.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "assetimporter.h"
#include "io/ioserver.h"
#include "toolkitutil/model/import/fbx/fbxfileimporter.h"
#include "toolkitutil/model/import/gltf/gltffileimporter.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/texture.h"
#include "flat/audio.h"

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
bool
ImportFBX(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::ImportFlags importFlags, float scale, ToolkitUtil::Logger* logger)
{
    IO::IoServer* ioServer = IO::IoServer::Instance();
    Util::String fileName = file.AsString().ExtractFileName();
    Ptr<ToolkitUtil::FbxFileImporter> fbxImporter = ToolkitUtil::FbxFileImporter::Create();
    fbxImporter->Open();
    fbxImporter->SetForce(true);
    fbxImporter->SetFolder(destinationFolder.LocalPath());
    fbxImporter->SetFile(fileName);
    fbxImporter->SetLogger(logger);
    fbxImporter->ImportFile(file, importFlags, scale);
    fbxImporter->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ImportGLTF(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::ImportFlags importFlags, float scale, ToolkitUtil::Logger* logger)
{
    Util::String fileName = file.AsString().ExtractFileName();
    Ptr<ToolkitUtil::GltfFileImporter> gltfImporter = ToolkitUtil::GltfFileImporter::Create();
    gltfImporter->Open();
    gltfImporter->SetForce(true);
    gltfImporter->SetFolder(destinationFolder.LocalPath());
    gltfImporter->SetFile(fileName);
    gltfImporter->SetLogger(logger);
    gltfImporter->ImportFile(file, importFlags, scale);
    gltfImporter->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ImportTexture(const IO::URI& file, const IO::URI& destinationFolder)
{
    ToolkitUtil::TextureResourceT texture;
    Util::String ext = file.LocalPath().GetFileExtension();
    Util::String fileName = file.LocalPath().ExtractFileName();
    ext.ToLower();
    if (ext == "jpg" || ext == "jpeg")
        texture.container = ToolkitUtil::TextureContainer_JPG;
    else if (ext == "tga")
        texture.container = ToolkitUtil::TextureContainer_TGA;
    else if (ext == "bmp")
        texture.container = ToolkitUtil::TextureContainer_BMP;
    else if (ext == "dds")
        texture.container = ToolkitUtil::TextureContainer_DDS;
    else if (ext == "png")
        texture.container = ToolkitUtil::TextureContainer_PNG;
    else if (ext == "exr")
        texture.container = ToolkitUtil::TextureContainer_EXR;
    else if (ext == "cube")
        texture.container = ToolkitUtil::TextureContainer_CUBE;

    if ((Util::String::MatchPattern(fileName, "*norm.*")) ||
        (Util::String::MatchPattern(fileName, "*normal.*")) ||
        (Util::String::MatchPattern(fileName, "*Normal.*")) ||
        (Util::String::MatchPattern(fileName, "*_N.*")) ||
        (Util::String::MatchPattern(fileName, "*_n.*")) ||
        (Util::String::MatchPattern(fileName, "*bump.*")))
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_BC5;
    }
    else if (Util::String::MatchPattern(fileName, "*material.*") ||
             Util::String::MatchPattern(fileName, "*orm.*") ||
             Util::String::MatchPattern(fileName, "*ORM.*") ||
             Util::String::MatchPattern(fileName, "*MetallicRoughness.*") ||
             Util::String::MatchPattern(fileName, "*Occlusion.*"))
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_BC7;
    }
    else if (Util::String::MatchPattern(fileName, "*height.*"))
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_R16;
    }
    else
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_R8G8B8A8;
    }

    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        std::vector<uint8_t> data;
        data.resize(stream->GetSize());
        memcpy(data.data(), stream->MemoryMap(), stream->GetSize());
        texture.data = data;

        stream->Close();
    }
    else
    {
        return false;
    }

    // Save nebula texture 
    IO::URI output = Util::String::Sprintf("%s/%s.natex", destinationFolder.LocalPath().AsCharPtr(), file.LocalPath().ExtractFileName().AsCharPtr());
    stream = IO::IoServer::Instance()->CreateStream(output);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (stream->Open())
    {
        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::TextureResource>(texture);
        stream->Write(data.GetPtr(), data.Size());
        stream->Close();

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
ImportAudio(const IO::URI& file, const IO::URI& destinationFolder)
{
    ToolkitUtil::AudioResourceT audio;
    Util::String ext = file.LocalPath().GetFileExtension();
    ext.ToLower();
    if (ext == "mp3")
        audio.container = ToolkitUtil::AudioContainer_MP3;
    else if (ext == "wav")
        audio.container = ToolkitUtil::AudioContainer_WAV;
    else if (ext == "ogg")
        audio.container = ToolkitUtil::AudioContainer_OGG;

    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        std::vector<uint8_t> data;
        data.resize(stream->GetSize());
        memcpy(data.data(), stream->MemoryMap(), stream->GetSize());
        audio.data = data;

        stream->Close();
    }
    else
    {
        return false;
    }

    // Save nebula texture 
    IO::URI output = Util::String::Sprintf("%s/%s.naaud", destinationFolder.LocalPath().AsCharPtr(), file.LocalPath().ExtractFileName().AsCharPtr());
    stream = IO::IoServer::Instance()->CreateStream(output);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (stream->Open())
    {
        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::AudioResource>(audio);
        stream->Write(data.GetPtr(), data.Size());
        stream->Close();

        return true;
    }

    return false;
}

} // namespace ToolkitUtil