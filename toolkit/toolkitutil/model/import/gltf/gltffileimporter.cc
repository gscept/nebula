//------------------------------------------------------------------------------
//  gltffileimporter.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "gltffileimporter.h"
#include "io/ioserver.h"
#include "model/meshutil/meshbuildersaver.h"
#include "model/modelutil/modeldatabase.h"
#include "model/modelutil/modelattributes.h"
#include "model/animutil/animbuildersaver.h"
#include "model/skeletonutil/skeletonbuilder.h"
#include "model/skeletonutil/skeletonbuildersaver.h"
#include "util/bitfield.h"
#include "timing/timer.h"
#include "io/binarywriter.h"
#include "io/filestream.h"
#include "io/jsonreader.h"
#include "surface/surfacebuilder.h"
#include "gltffilematerialextractor.h"
#include "jobs2/jobs2.h"

#include "toolkitutil/text.h"

#include "flat/texture.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"

#define NEBULA_VALIDATE_GLTF 1

using namespace Util;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::GltfFileImporter, 'glte', ToolkitUtil::ModelImporter);

//------------------------------------------------------------------------------
/**
*/
GltfFileImporter::GltfFileImporter() 
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GltfFileImporter::~GltfFileImporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
GltfFileImporter::NeedsConversion(const Util::String& path)
{
    if (ModelImporter::NeedsConversion(path))
    {
        return true;
    }

    // glb files embed all their data
    if (path.GetFileExtension() != "gltf")
    {
        return false;
    }

    // external buffers and images are resolved relative to the gltf file
    Util::String asset = this->GetOutputAssetPath(path);
    Util::String folder = path.ExtractDirName();
    bool needsConversion = false;
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(IO::CreateStream(path));
    if (reader->Open())
    {
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
                    if (reader->HasAttr("uri"))
                    {
                        Util::String uri = reader->GetString("uri");
                        if (!uri.BeginsWithString("data:") && AssetProcessorBase::NeedsConversion(folder + "/" + uri, asset))
                        {
                            needsConversion = true;
                        }
                    }
                } while (!needsConversion && reader->SetToNextChild());
                reader->SetToParent();
            }
            reader->SetToParent();
        }
        reader->Close();
    }
    return needsConversion;
}

//------------------------------------------------------------------------------
/**
*/
bool 
GltfFileImporter::ParseScene(ToolkitUtil::ImportFlags importFlags, float scale)
{
    this->gltfScene = Gltf::Document();
    bool res = this->gltfScene.Deserialize(this->path.LocalPath().AsCharPtr());
    
    if (!res)
    {
        this->logger->Warning("Failed to import %s\n", this->file.AsCharPtr());
        return false;
    }

    auto scene = new NglTFScene();
    scene->SetName(this->file);
    scene->SetCategory(this->sourceDir);
    scene->Setup(&this->gltfScene, importFlags, scale);
    this->scene = scene;

    String fileExtension = this->path.LocalPath().GetFileExtension();
    {
        // Extract materials into .sur files
        // Always do this before exporting textures, since the texture names may be changed in the extractor.
        GltfFileMaterialExtractor extractor;
        extractor.SetLogger(this->logger);
        extractor.SetDocument(&this->gltfScene);
        extractor.SetOutputFolder(this->sourceDir);
        Util::Array<IO::URI> outputs = extractor.ExtractAll();
        this->outputFiles.AppendArray(outputs);
    }

    // textures are saved under the names the material extractor gives them in the materials' URNs
    auto textureSave = [this](ToolkitUtil::TextureResourceT& tex, const Gltf::Image& image, const Gltf::Document& scene, const Util::String& outputDir, int32_t imageIndex)
    {
        tex.container = (image.type == Gltf::Image::Type::Jpg) ? ToolkitUtil::TextureContainer_JPG : ToolkitUtil::TextureContainer_PNG;
        Util::String outputFile = Util::String::Sprintf("%s/%s.natex", outputDir.AsCharPtr(), GltfFileMaterialExtractor::TextureName(scene, imageIndex).AsCharPtr());
        if (image.embedded)
        {
            if (image.data.IsValid())
            {
                tex.data = std::vector<uint8_t>((uint8_t*)image.data.GetPtr(), (uint8_t*)image.data.GetPtr() + image.data.Size());
            }
            else
            {
                const Gltf::BufferView& view = scene.bufferViews[image.bufferView];
                const Util::Blob& data = scene.buffers[view.buffer].data;
                tex.data = std::vector<uint8_t>((uint8_t*)data.GetPtr() + view.byteOffset, (uint8_t*)data.GetPtr() + view.byteOffset + view.byteLength);
            }
        }
        else
        {
            // an external image that has been imported already, by an earlier model import or as a texture of its own,
            // keeps the settings it was imported with. Reimporting the model doesn't change the image, so this holds
            // for forced imports as well
            if (IO::FileExists(outputFile))
            {
                return;
            }

            // external images are resolved relative to the gltf file
            Util::String fullFileName = Util::String::Sprintf("%s/%s", this->path.LocalPath().ExtractDirName().AsCharPtr(), image.uri.AsCharPtr());
            Ptr<IO::Stream> fileSource = IO::CreateStream(fullFileName);
            fileSource->SetAccessMode(IO::Stream::ReadAccess);
            if (!fileSource->Open())
            {
                this->logger->Warning("Failed to read texture %s\n", fullFileName.AsCharPtr());
                return;
            }
            IO::Stream::Size size = fileSource->GetSize();
            void* data = fileSource->Map();
            tex.data = std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + size);
            fileSource->Unmap();
            fileSource->Close();
        }

        // Save asset file
        Ptr<IO::Stream> stream = IO::CreateStream(outputFile);
        stream->SetAccessMode(IO::Stream::WriteAccess);
        if (stream->Open())
        {
            Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::TextureResource>(tex);
            stream->Write(data.GetPtr(), data.Size());
            stream->Close();

            this->logger->Print("%s\n", Format("Generated texture: %s", Text(IO::URI(outputFile).LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
        }
    };

    bool hasEmbedded = false;
    for (IndexT i = 0; i < gltfScene.materials.Size(); i++)
    {
        Gltf::Material const& material = gltfScene.materials[i];
        if (material.normalTexture.index != -1)
        {
            ToolkitUtil::TextureResourceT tex;
            tex.target_format = ToolkitUtil::TexturePixelFormat_BC5;
            tex.color_space = ToolkitUtil::TextureColorSpace_Linear;
            tex.invert_green = false;
            
            int32_t imageIndex = gltfScene.textures[material.normalTexture.index].source;
            textureSave(tex, gltfScene.images[imageIndex], gltfScene, this->sourceDir, imageIndex);
        }
        if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
        {
            ToolkitUtil::TextureResourceT tex;
            tex.target_format = ToolkitUtil::TexturePixelFormat_R8G8B8A8;
            tex.color_space = ToolkitUtil::TextureColorSpace_sRGB;

            int32_t imageIndex = gltfScene.textures[material.pbrMetallicRoughness.baseColorTexture.index].source;
            textureSave(tex, gltfScene.images[imageIndex], gltfScene, this->sourceDir, imageIndex);
        }
        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
        {
            ToolkitUtil::TextureResourceT tex;
            tex.target_format = ToolkitUtil::TexturePixelFormat_BC7;
            tex.color_space = ToolkitUtil::TextureColorSpace_Linear;

            int32_t imageIndex = gltfScene.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source;
            textureSave(tex, gltfScene.images[imageIndex], gltfScene, this->sourceDir, imageIndex);
        }
        if (material.emissiveTexture.index != -1)
        {
            ToolkitUtil::TextureResourceT tex;
            tex.target_format = ToolkitUtil::TexturePixelFormat_R8G8B8A8;
            tex.color_space = ToolkitUtil::TextureColorSpace_sRGB;

            int32_t imageIndex = gltfScene.textures[material.emissiveTexture.index].source;
            textureSave(tex, gltfScene.images[imageIndex], gltfScene, this->sourceDir, imageIndex);
        }
        if (material.occlusionTexture.index != -1)
        {
            // glTF usually packs occlusion with metallic and roughness, so it's saved the same way
            ToolkitUtil::TextureResourceT tex;
            tex.target_format = ToolkitUtil::TexturePixelFormat_BC7;
            tex.color_space = ToolkitUtil::TextureColorSpace_Linear;

            int32_t imageIndex = gltfScene.textures[material.occlusionTexture.index].source;
            textureSave(tex, gltfScene.images[imageIndex], gltfScene, this->sourceDir, imageIndex);
        }
    }

    return res;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------