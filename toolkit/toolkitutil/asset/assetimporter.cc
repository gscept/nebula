//------------------------------------------------------------------------------
//  assetimporter.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "assetimporter.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
#include "io/assignregistry.h"
#include "toolkitutil/model/import/fbx/fbxfileimporter.h"
#include "toolkitutil/model/import/gltf/gltffileimporter.h"
#include "ufbx/ufbx.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/texture.h"
#include "flat/audio.h"
#include "flat/model.h"

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
bool
ModelSourceIsSkinned(const IO::URI& file)
{
    const Util::String path = file.LocalPath();
    Util::String ext = path.GetFileExtension();
    ext.ToLower();
    if (ext == "fbx")
    {
        // only the skin deformers are needed, they're loaded without the geometry
        ufbx_load_opts opts{ .ignore_geometry = true, .ignore_animation = true, .ignore_embedded = true };
        ufbx_error error;
        ufbx_scene* scene = ufbx_load_file_len(path.AsCharPtr(), path.Length(), &opts, &error);
        if (scene == nullptr)
            return false;

        bool skinned = false;
        for (size_t i = 0; i < scene->meshes.count && !skinned; i++)
            skinned = scene->meshes[i]->skin_deformers.count > 0;
        ufbx_free_scene(scene);
        return skinned;
    }
    else if (ext == "gltf" || ext == "glb")
    {
        Gltf::Document document;
        if (!document.Deserialize(file))
            return false;
        return !document.skins.IsEmpty();
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
GetModelSource(const IO::Path& asset, IO::URI& outSource, Util::String& outError)
{
    const IO::URI assetURI = asset.WorkURI("assets");
    Util::String source;

    // only the source is needed, so read the flatbuffer in place instead of unpacking the mesh
    Ptr<IO::Stream> stream = IO::CreateStream(assetURI);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        outError.Format("can't read '%s'", assetURI.LocalPath().AsCharPtr());
        return false;
    }
    const ToolkitUtil::ModelAsset* modelAsset = flatbuffers::GetRoot<ToolkitUtil::ModelAsset>(stream->MemoryMap());
    if (modelAsset->source() != nullptr)
    {
        source = modelAsset->source()->c_str();
    }
    stream->MemoryUnmap();
    stream->Close();

    // assets imported before the source was recorded, by convention the sources mirror the asset folders
    if (source.IsEmpty())
    {
        for (const char* ext : { "fbx", "gltf", "glb" })
        {
            Util::String candidate = Util::Format("src:source/%s/%s.%s", asset.GetFolder().AsCharPtr(), asset.GetFile().AsCharPtr(), ext);
            if (IO::FileExists(candidate))
            {
                source = candidate;
                break;
            }
        }
        if (source.IsEmpty())
        {
            outError.Format("it doesn't say what it was imported from, and there is no src:source/%s/%s.fbx, .gltf or .glb", asset.GetFolder().AsCharPtr(), asset.GetFile().AsCharPtr());
            return false;
        }
    }

    const IO::URI sourceURI(source);
    if (!IO::FileExists(sourceURI))
    {
        outError.Format("its source '%s' doesn't exist", sourceURI.LocalPath().AsCharPtr());
        return false;
    }

    // the importer names the asset after the source, so a renamed asset would be imported next to itself
    Util::String sourceName = sourceURI.LocalPath().ExtractFileName();
    sourceName.StripFileExtension();
    if (sourceName != asset.GetFile())
    {
        outError.Format("it was renamed, and importing '%s' would create '%s' instead", sourceURI.LocalPath().ExtractFileName().AsCharPtr(), sourceName.AsCharPtr());
        return false;
    }
    outSource = sourceURI;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ReimportModel(const IO::Path& asset, ToolkitUtil::Logger* logger, Util::String& outError)
{
    IO::URI sourceURI;
    if (!GetModelSource(asset, sourceURI, outError))
    {
        return false;
    }

    const IO::URI assetURI = asset.WorkURI("assets");
    uint optimizationFlags = 0;
    float scale = 1.0f;
    Ptr<IO::Stream> stream = IO::CreateStream(assetURI);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        outError.Format("can't read '%s'", assetURI.LocalPath().AsCharPtr());
        return false;
    }
    const ToolkitUtil::ModelAsset* modelAsset = flatbuffers::GetRoot<ToolkitUtil::ModelAsset>(stream->MemoryMap());
    optimizationFlags = (uint)modelAsset->flags();
    scale = modelAsset->scale();
    stream->MemoryUnmap();
    stream->Close();
    Util::String ext = sourceURI.LocalPath().GetFileExtension();
    ext.ToLower();

    // the stored optimization flags are the import flags the mesh was built with
    uint flags = ToolkitUtil::ImportFlags::ReplaceExistingMesh;
    flags |= AllBits(optimizationFlags, ToolkitUtil::OptimizationFlags_RemoveRedundantVertices) ? ToolkitUtil::ImportFlags::RemoveRedundant : 0;
    flags |= AllBits(optimizationFlags, ToolkitUtil::OptimizationFlags_CalculateNormals) ? ToolkitUtil::ImportFlags::CalcNormals : 0;
    flags |= AllBits(optimizationFlags, ToolkitUtil::OptimizationFlags_FlipUVs) ? ToolkitUtil::ImportFlags::FlipUVs : 0;
    flags |= AllBits(optimizationFlags, ToolkitUtil::OptimizationFlags_ImportVertexColors) ? ToolkitUtil::ImportFlags::ImportColors : 0;
    flags |= AllBits(optimizationFlags, ToolkitUtil::OptimizationFlags_ImportSecondaryUVs) ? ToolkitUtil::ImportFlags::ImportSecondaryUVs : 0;
    flags |= AllBits(optimizationFlags, ToolkitUtil::OptimizationFlags_CalculateTangentFrame) ? ToolkitUtil::ImportFlags::CalcTangents : 0;
    flags |= AllBits(optimizationFlags, ToolkitUtil::OptimizationFlags_CalculateRigidSkinning) ? ToolkitUtil::ImportFlags::CalcRigidSkin : 0;

    const IO::URI destination(assetURI.LocalPath().ExtractDirName());
    if (ext == "fbx")
    {
        return ImportFBX(sourceURI, destination, (ToolkitUtil::ImportFlags)flags, scale, logger, true);
    }
    else if (ext == "gltf" || ext == "glb")
    {
        return ImportGLTF(sourceURI, destination, (ToolkitUtil::ImportFlags)flags, scale, logger, true);
    }
    outError.Format("its source '%s' isn't an fbx, gltf or glb file", sourceURI.LocalPath().AsCharPtr());
    return false;
}

//------------------------------------------------------------------------------
/**
    ImportTexture names the texture after the image, a cube texture made of faces is named when importing
*/
bool
GetTextureSource(const IO::Path& asset, Util::Array<IO::URI>& outFiles, Util::String& outError)
{
    outFiles.Clear();
    const IO::URI assetURI = asset.WorkURI("assets");
    Util::String source;
    Util::Array<Util::String> faces;
    bool isCube = false;
    {
        // only the sources are needed, so read the flatbuffer in place instead of unpacking the images
        Ptr<IO::Stream> stream = IO::CreateStream(assetURI);
        stream->SetAccessMode(IO::Stream::ReadAccess);
        if (!stream->Open())
        {
            outError.Format("can't read '%s'", assetURI.LocalPath().AsCharPtr());
            return false;
        }
        const ToolkitUtil::TextureResource* texture = flatbuffers::GetRoot<ToolkitUtil::TextureResource>(stream->MemoryMap());
        isCube = texture->cube_layout() == ToolkitUtil::TextureCubeLayout_Faces;
        if (texture->source() != nullptr)
        {
            source = texture->source()->c_str();
        }
        if (isCube && texture->faces() != nullptr)
        {
            for (const ToolkitUtil::TextureFace* face : *texture->faces())
            {
                faces.Append(face->source() != nullptr ? Util::String(face->source()->c_str()) : Util::String());
            }
        }
        stream->MemoryUnmap();
        stream->Close();
    }

    if (isCube)
    {
        if (faces.Size() != 6 || faces.FindIndex("") != InvalidIndex)
        {
            outError = "it's a cube texture from before the face images were recorded, import the faces again by dropping them on the editor";
            return false;
        }
        for (const Util::String& face : faces)
        {
            const IO::URI faceURI(face);
            if (!IO::FileExists(faceURI))
            {
                outError.Format("its face '%s' doesn't exist", faceURI.LocalPath().AsCharPtr());
                return false;
            }
            outFiles.Append(faceURI);
        }
        return true;
    }

    // textures imported before the source was recorded, by convention the sources mirror the asset folders
    if (source.IsEmpty())
    {
        for (const char* ext : { "png", "jpg", "jpeg", "tga", "bmp", "dds", "exr", "tif", "tiff", "hdr" })
        {
            Util::String candidate = Util::Format("src:source/%s/%s.%s", asset.GetFolder().AsCharPtr(), asset.GetFile().AsCharPtr(), ext);
            if (IO::FileExists(candidate))
            {
                source = candidate;
                break;
            }
        }
        if (source.IsEmpty())
        {
            outError.Format("it doesn't say what it was imported from, and there is no image named src:source/%s/%s.png, .jpg or alike", asset.GetFolder().AsCharPtr(), asset.GetFile().AsCharPtr());
            return false;
        }
    }

    const IO::URI sourceURI(source);
    if (!IO::FileExists(sourceURI))
    {
        outError.Format("its source '%s' doesn't exist", sourceURI.LocalPath().AsCharPtr());
        return false;
    }
    Util::String sourceName = sourceURI.LocalPath().ExtractFileName();
    sourceName.StripFileExtension();
    if (sourceName != asset.GetFile())
    {
        outError.Format("it was renamed, and importing '%s' would create '%s' instead", sourceURI.LocalPath().ExtractFileName().AsCharPtr(), sourceName.AsCharPtr());
        return false;
    }
    outFiles.Append(sourceURI);
    return true;
}

//------------------------------------------------------------------------------
/**
    The importers name these when there's no material to use, see NFbxMeshNode and MeshPrimitiveFunc
*/
bool
AssignPlaceholderMaterials(const IO::Path& asset, const Util::String& material, SizeT& outAssigned, SizeT& outAssignedSkins, Util::String& outError)
{
    outAssigned = 0;
    outAssignedSkins = 0;
    const IO::URI assetURI = asset.WorkURI("assets");
    ToolkitUtil::ModelAssetT modelAsset;
    {
        Ptr<IO::Stream> stream = IO::CreateStream(assetURI);
        stream->SetAccessMode(IO::Stream::ReadAccess);
        if (!stream->Open())
        {
            outError.Format("can't read '%s'", assetURI.LocalPath().AsCharPtr());
            return false;
        }
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ModelAsset>(modelAsset, (const uint8_t*)stream->MemoryMap());
        stream->MemoryUnmap();
        stream->Close();
    }
    if (modelAsset.scene == nullptr)
    {
        return true;
    }

    const auto assign = [&material, &outAssigned](ToolkitUtil::ShapeNodeT* shape)
    {
        if (shape != nullptr && (shape->material == "system:placeholder:mat" || shape->material == "system:gltf_default:mat"))
        {
            shape->material = material;
            outAssigned++;
        }
    };
    for (auto& shape : modelAsset.scene->shapes)
        assign(shape.get());
    for (auto& skin : modelAsset.scene->skins)
    {
        if (skin != nullptr)
        {
            const SizeT assigned = outAssigned;
            assign(skin->shape.get());
            outAssignedSkins += outAssigned - assigned;
        }
    }
    if (outAssigned == 0)
    {
        return true;
    }

    Ptr<IO::Stream> stream = IO::CreateStream(assetURI);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        outError.Format("can't write '%s'", assetURI.LocalPath().AsCharPtr());
        return false;
    }
    Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::ModelAsset>(modelAsset);
    stream->Write(data.GetPtr(), data.Size());
    stream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
GetSourceReference(const IO::URI& file)
{
    const Util::String sourceRoot = IO::AssignRegistry::Instance()->HasAssign("src") ? IO::URI("src:").LocalPath() : Util::String();
    const Util::String sourcePath = file.LocalPath();
    if (!sourceRoot.IsEmpty() && sourcePath.BeginsWithString(sourceRoot + "/"))
    {
        return "src:" + sourcePath.ExtractRange(sourceRoot.Length() + 1, sourcePath.Length() - sourceRoot.Length() - 1);
    }
    return sourcePath;
}

//------------------------------------------------------------------------------
/**
*/
IO::URI
GetSourceFolder(const IO::Path& assetFolder)
{
    const Util::String& folder = assetFolder.GetFolder();
    return folder.IsEmpty() ? IO::URI("src:source") : IO::URI(Util::Format("src:source/%s", folder.AsCharPtr()));
}

//------------------------------------------------------------------------------
/**
*/
bool
IsInSourceFolder(const IO::URI& file)
{
    if (!IO::AssignRegistry::Instance()->HasAssign("src"))
    {
        return false;
    }
    const Util::String sourceRoot = IO::URI("src:source").LocalPath();
    return file.LocalPath().BeginsWithString(sourceRoot + "/");
}

//------------------------------------------------------------------------------
/**
    IO::CopyFile stops the application when a file can't be opened, this reports it instead
*/
static bool
CopySourceFile(const IO::URI& from, const IO::URI& to, Util::String& outError)
{
    Ptr<IO::Stream> fromStream = IO::CreateStream(from);
    fromStream->SetAccessMode(IO::Stream::ReadAccess);
    if (!fromStream->Open())
    {
        outError.Format("can't read '%s'", from.LocalPath().AsCharPtr());
        return false;
    }
    IO::CreateDirectory(IO::URI(to.LocalPath().ExtractDirName()));
    Ptr<IO::Stream> toStream = IO::CreateStream(to);
    toStream->SetAccessMode(IO::Stream::WriteAccess);
    if (!toStream->Open())
    {
        fromStream->Close();
        outError.Format("can't write '%s'", to.LocalPath().AsCharPtr());
        return false;
    }
    const IO::Stream::Size size = fromStream->GetSize();
    if (size > 0)
    {
        toStream->Write(fromStream->MemoryMap(), size);
        fromStream->MemoryUnmap();
    }
    toStream->Close();
    fromStream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
    The importer looks for the external buffers and images of a gltf relative to it, they're copied along
    keeping the folders they're in. Missing ones are left out, the importer warns about them
*/
bool
CopyImportSource(const IO::URI& file, const IO::URI& destinationFolder, IO::URI& outFile, Util::String& outError)
{
    const Util::String sourcePath = file.LocalPath();
    const Util::String sourceFolder = sourcePath.ExtractDirName();
    const Util::String fileName = sourcePath.ExtractFileName();
    Util::String targetFolder = destinationFolder.LocalPath();
    if (!targetFolder.EndsWithString("/"))
    {
        targetFolder.Append("/");
    }

    Util::Array<Util::String> files;
    files.Append(fileName);
    Util::String ext = fileName.GetFileExtension();
    ext.ToLower();
    if (ext == "gltf")
    {
        Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
        reader->SetStream(IO::CreateStream(file));
        if (!reader->Open())
        {
            outError.Format("can't read '%s'", sourcePath.AsCharPtr());
            return false;
        }
        for (const char* key : { "buffers", "images" })
        {
            if (!reader->SetToFirstChild(key))
            {
                continue;
            }
            if (reader->SetToFirstChild())
            {
                do
                {
                    if (!reader->HasAttr("uri"))
                    {
                        continue;
                    }
                    Util::String uri = reader->GetString("uri");
                    if (uri.BeginsWithString("data:"))
                    {
                        continue;
                    }
                    // a copy can only take files along that end up next to it
                    uri.ConvertBackslashes();
                    if (uri.BeginsWithString("/") || uri.FindCharIndex(':') != InvalidIndex || ("/" + uri + "/").FindStringIndex("/../") != InvalidIndex)
                    {
                        reader->Close();
                        outError.Format("it references '%s', which isn't in its folder. Copy it to the source folder by hand", uri.AsCharPtr());
                        return false;
                    }
                    if (files.FindIndex(uri) == InvalidIndex && IO::FileExists(IO::URI(sourceFolder + uri)))
                    {
                        files.Append(uri);
                    }
                } while (reader->SetToNextChild());
                reader->SetToParent();
            }
            reader->SetToParent();
        }
        reader->Close();
    }

    outFile = IO::URI(targetFolder + fileName);
    for (const Util::String& relative : files)
    {
        const IO::URI from(sourceFolder + relative);
        const IO::URI to(targetFolder + relative);
        if (from.LocalPath() == to.LocalPath())
        {
            continue;
        }
        if (!CopySourceFile(from, to, outError))
        {
            return false;
        }
    }
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
    Ptr<IO::Stream> stream = IO::CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        data.resize(stream->GetSize());
        memcpy(data.data(), stream->MemoryMap(), stream->GetSize());
        texture.data = data;
        texture.source = file.LocalPath();

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
    Ptr<IO::Stream> stream = IO::CreateStream(output);
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
    texture.source = GetSourceReference(file);

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
    texture.source.Clear();
    for (const IO::URI& file : faces)
    {
        auto face = std::make_unique<ToolkitUtil::TextureFaceT>();
        face->container = GetTextureContainerFromPath(file);
        face->source = GetSourceReference(file);
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

    Ptr<IO::Stream> stream = IO::CreateStream(file);
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
    stream = IO::CreateStream(output);
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