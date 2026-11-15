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
ImportFBX(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::ImportFlags importFlags, float scale, ToolkitUtil::Logger* logger, bool force)
{
    Util::String fileName = file.AsString().ExtractFileName();
    Util::String fileNameNoExt = fileName;
    fileNameNoExt.StripFileExtension();
    Ptr<ToolkitUtil::FbxFileImporter> fbxImporter = ToolkitUtil::FbxFileImporter::Create();
    fbxImporter->Open();
    fbxImporter->SetForce(force);
    fbxImporter->SetFolder(destinationFolder.LocalPath());
    fbxImporter->SetFile(fileName);
    fbxImporter->SetLogger(logger);
    fbxImporter->ProcessFile(file, importFlags, scale);
    fbxImporter->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ImportGLTF(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::ImportFlags importFlags, float scale, ToolkitUtil::Logger* logger, bool force)
{
    Util::String fileName = file.AsString().ExtractFileName();
    Util::String fileNameNoExt = fileName;
    fileNameNoExt.StripFileExtension();
    Ptr<ToolkitUtil::GltfFileImporter> gltfImporter = ToolkitUtil::GltfFileImporter::Create();
    gltfImporter->Open();
    gltfImporter->SetForce(force);
    gltfImporter->SetFolder(destinationFolder.LocalPath());
    gltfImporter->SetFile(fileName);
    gltfImporter->SetLogger(logger);
    gltfImporter->ProcessFile(file, importFlags, scale);
    gltfImporter->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
static ToolkitUtil::TextureContainer
GetTextureContainerFromPath(const IO::URI& file)
{
    Util::String ext = file.LocalPath().GetFileExtension();
    ext.ToLower();
    if (ext == "jpg" || ext == "jpeg")
        return ToolkitUtil::TextureContainer_JPG;
    else if (ext == "tga")
        return ToolkitUtil::TextureContainer_TGA;
    else if (ext == "bmp")
        return ToolkitUtil::TextureContainer_BMP;
    else if (ext == "dds")
        return ToolkitUtil::TextureContainer_DDS;
    else if (ext == "png")
        return ToolkitUtil::TextureContainer_PNG;
    else if (ext == "exr")
        return ToolkitUtil::TextureContainer_EXR;
    else if (ext == "tif" || ext == "tiff")
        return ToolkitUtil::TextureContainer_TIF;
    else if (ext == "cube")
        return ToolkitUtil::TextureContainer_CUBE;
    else if (ext == "hdr")
        return ToolkitUtil::TextureContainer_HDR;
    return ToolkitUtil::TextureContainer_Unknown;
}

//------------------------------------------------------------------------------
/**
*/
ToolkitUtil::TextureResourceT 
SetupTextureImportSettingsFromPath(const IO::URI& file)
{
    ToolkitUtil::TextureResourceT texture;
    Util::String fileName = file.LocalPath().ExtractFileName();
    texture.container = GetTextureContainerFromPath(file);

    if ((Util::String::MatchPattern(fileName, "*norm.*")) ||
        (Util::String::MatchPattern(fileName, "*normal.*")) ||
        (Util::String::MatchPattern(fileName, "*Normal.*")) ||
        (Util::String::MatchPattern(fileName, "*_N.*")) ||
        (Util::String::MatchPattern(fileName, "*_n.*")) ||
        (Util::String::MatchPattern(fileName, "*bump.*")))
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_BC5;
        texture.color_space = ToolkitUtil::TextureColorSpace_Linear;
    }
    else if (Util::String::MatchPattern(fileName, "*material.*") ||
             Util::String::MatchPattern(fileName, "*orm.*") ||
             Util::String::MatchPattern(fileName, "*ORM.*") ||
             Util::String::MatchPattern(fileName, "*MetallicRoughness.*") ||
             Util::String::MatchPattern(fileName, "*Occlusion.*"))
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_BC7;
        texture.color_space = ToolkitUtil::TextureColorSpace_Linear;
    }
    else if (Util::String::MatchPattern(fileName, "*height.*"))
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_R16;
        texture.color_space = ToolkitUtil::TextureColorSpace_Linear;
    }
    else if (texture.container == ToolkitUtil::TextureContainer_HDR || texture.container == ToolkitUtil::TextureContainer_EXR)
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_BC6H;
        texture.color_space = ToolkitUtil::TextureColorSpace_Linear;
    }
    else
    {
        texture.target_format = ToolkitUtil::TexturePixelFormat_R8G8B8A8;
        texture.color_space = ToolkitUtil::TextureColorSpace_sRGB;
    }
    return texture;
}

//------------------------------------------------------------------------------
/**
    Returns the cube face (0-5 for +X, -X, +Y, -Y, +Z, -Z) an image is named after, like
    sky_px.png, sky-negx.hdr or right.jpg. cubeName receives the file name without the face,
    or the folder name if there is nothing left.
*/
IndexT
GetCubeFaceFromPath(const IO::URI& file, Util::String& cubeName)
{
    static const char* FaceNames[6][3] = {
        { "px", "posx", "right" },
        { "nx", "negx", "left" },
        { "py", "posy", "top" },
        { "ny", "negy", "bottom" },
        { "pz", "posz", "front" },
        { "nz", "negz", "back" },
    };
    Util::String path = file.LocalPath();
    Util::String name = path.ExtractFileName();
    name.StripFileExtension();
    Util::String lowerName = name;
    lowerName.ToLower();
    for (IndexT face = 0; face < 6; face++)
    {
        for (const char* faceName : FaceNames[face])
        {
            if (!lowerName.EndsWithString(faceName))
                continue;

            SizeT baseLength = lowerName.Length() - (SizeT)strlen(faceName);
            if (baseLength == 0)
            {
                cubeName = path.ExtractLastDirName();
                return face;
            }
            char separator = lowerName[baseLength - 1];
            if (separator == '_' || separator == '-' || separator == '.' || separator == ' ')
            {
                cubeName = name.ExtractRange(0, baseLength - 1);
                return face;
            }
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
static bool
ReadImportFile(const IO::URI& file, std::vector<uint8_t>& data)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        data.resize(stream->GetSize());
        memcpy(data.data(), stream->MemoryMap(), stream->GetSize());

        stream->MemoryUnmap();
        stream->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
static bool
SaveTextureResource(const ToolkitUtil::TextureResourceT& texture, const IO::URI& output)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(output);
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
ImportTexture(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::TextureResourceT& texture, ToolkitUtil::Logger* logger)
{
    if (texture.cube_layout == ToolkitUtil::TextureCubeLayout_Faces)
    {
        logger->Error("'%s' is a single image, use ImportCubeTexture to import cube faces\n", file.LocalPath().AsCharPtr());
        return false;
    }
    texture.faces.clear();
    if (!ReadImportFile(file, texture.data))
    {
        return false;
    }

    // Save nebula texture 
    Util::String fileNameNoExt = file.LocalPath().ExtractFileName();
    fileNameNoExt.StripFileExtension();
    IO::URI output = Util::String::Sprintf("%s/%s.natex", destinationFolder.LocalPath().AsCharPtr(), fileNameNoExt.AsCharPtr());
    logger->Print("%s -> %s\n", file.LocalPath().AsCharPtr(), output.LocalPath().AsCharPtr());

    return SaveTextureResource(texture, output);
}

//------------------------------------------------------------------------------
/**
    The faces are stored as they are and assembled by texassemble when batching
*/
bool
ImportCubeTexture(const Util::FixedArray<IO::URI>& faces, const IO::URI& destinationFolder, const Util::String& name, ToolkitUtil::TextureResourceT& texture, ToolkitUtil::Logger* logger)
{
    n_assert(faces.Size() == 6);
    texture.cube_layout = ToolkitUtil::TextureCubeLayout_Faces;
    texture.container = ToolkitUtil::TextureContainer_Unknown;
    texture.data.clear();
    texture.faces.clear();
    for (const IO::URI& file : faces)
    {
        auto face = std::make_unique<ToolkitUtil::TextureFaceT>();
        face->container = GetTextureContainerFromPath(file);
        if (!ReadImportFile(file, face->data))
        {
            logger->Error("Failed to read cube face '%s'\n", file.LocalPath().AsCharPtr());
            return false;
        }
        texture.faces.push_back(std::move(face));
    }

    IO::URI output = Util::String::Sprintf("%s/%s.natex", destinationFolder.LocalPath().AsCharPtr(), name.AsCharPtr());
    for (const IO::URI& file : faces)
    {
        logger->Print("%s -> %s\n", file.LocalPath().AsCharPtr(), output.LocalPath().AsCharPtr());
    }

    return SaveTextureResource(texture, output);
}

//------------------------------------------------------------------------------
/**
*/
bool
ImportAudio(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Logger* logger)
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

        stream->MemoryUnmap();
        stream->Close();
    }
    else
    {
        return false;
    }

    // Save nebula texture 
    IO::URI output = Util::String::Sprintf("%s/%s.naaud", destinationFolder.LocalPath().AsCharPtr(), file.LocalPath().ExtractFileName().AsCharPtr());

    logger->Print("%s -> %s\n", file.LocalPath().AsCharPtr(), output.LocalPath().AsCharPtr());
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