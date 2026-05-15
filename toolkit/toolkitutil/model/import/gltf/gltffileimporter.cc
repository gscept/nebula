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
#include "surface/surfacebuilder.h"
#include "gltffilematerialextractor.h"
#include "jobs2/jobs2.h"

#include "flat/texture.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"

#define NEBULA_VALIDATE_GLTF 1

using namespace Util;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::GltfFileImporter, 'glte', Base::ImporterBase);

//------------------------------------------------------------------------------
/**
*/
GltfFileImporter::GltfFileImporter() 
    : texConverter(nullptr)
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
GltfFileImporter::ParseScene(ToolkitUtil::ImportFlags importFlags, float scale)
{
    this->gltfScene = Gltf::Document();
    bool res = this->gltfScene.Deserialize(this->path.LocalPath().AsCharPtr());
    
    if (!res)
    {
        this->logger->Warning("Failed to import '%s'\n\n", this->file.AsCharPtr());
        return false;
    }

    auto scene = new NglTFScene();
    scene->SetName(this->file);
    scene->SetCategory(this->folder);
    scene->Setup(&this->gltfScene, importFlags, scale);
    this->texConverter->SetLogger(this->logger);
    this->scene = scene;

    String fileExtension = this->path.LocalPath().GetFileExtension();
    {
        // Extract materials into .sur files
        // Always do this before exporting textures, since the texture names may be changed in the extractor.
        GltfFileMaterialExtractor extractor;
        extractor.SetLogger(this->logger);
        extractor.SetCategoryName(this->folder);
        extractor.SetDocument(&this->gltfScene);
        extractor.SetExportSubDirectory(this->file);
        Util::Array<IO::URI> outputs = extractor.ExportAll();
        this->outputFiles.AppendArray(outputs);
    }

    const Util::String tmpDir = Util::String::Sprintf("temp:textureconverter/%s", this->folder.AsCharPtr());
    Util::String intermediateDir = Util::String::Sprintf("%s/%s/%s/temp", tmpDir.AsCharPtr(), this->folder.AsCharPtr(), this->file.AsCharPtr());

    auto textureSave = [this](ToolkitUtil::TextureResourceT& tex, const Gltf::Image& image, const Util::String& intermediateDir, int32_t imageIndex)
    {
        if (image.embedded)
        {
            tex.container = (image.type == Gltf::Image::Type::Jpg) ? ToolkitUtil::TextureContainer_JPG : ToolkitUtil::TextureContainer_PNG;
            tex.data = std::vector<uint8_t>((uint8_t*)image.data.GetPtr(), (uint8_t*)image.data.GetPtr() + image.data.Size());
            Util::String intermediateFile = Util::String::Sprintf("%s/%d.natex", intermediateDir.AsCharPtr(), imageIndex);
            Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(intermediateFile);
            stream->SetAccessMode(IO::Stream::WriteAccess);
            if (stream->Open())
            {
                Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::TextureResource>(tex);
                stream->Write(data.GetPtr(), data.Size());
                stream->Close();
            }
        }
        else
        {
            Util::String fileName = image.uri;
            fileName.StripFileExtension();
            Util::String fullFileName = Util::String::Sprintf("%s/%s", this->folder.AsCharPtr(), fileName.AsCharPtr());

            // Source is not embedded, so load source data
            Ptr<IO::Stream> fileSource = IO::IoServer::Instance()->CreateStream(fullFileName);
            fileSource->SetAccessMode(IO::Stream::ReadAccess);
            if (fileSource->Open())
            {
                IO::Stream::Size size = fileSource->GetSize();
                void* data = fileSource->Map();

                tex.data = std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + size);

                // Save asset file
                Util::String intermediateFile = Util::String::Sprintf("%s/%d.natex", intermediateDir.AsCharPtr(), imageIndex);
                Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(intermediateFile);
                stream->SetAccessMode(IO::Stream::WriteAccess);
                if (stream->Open())
                {
                    Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::TextureResource>(tex);
                    stream->Write(data.GetPtr(), data.Size());
                    stream->Close();
                }
            }
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
            tex.invert_green = false;
            
            int32_t imageIndex = gltfScene.textures[material.normalTexture.index].source;
            textureSave(tex, gltfScene.images[imageIndex], intermediateDir, imageIndex);
        }
        if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
        {
            ToolkitUtil::TextureResourceT tex;
            tex.target_format = ToolkitUtil::TexturePixelFormat_R8G8B8A8;
            tex.color_space = ToolkitUtil::TextureColorSpace_sRGB;

            int32_t imageIndex = gltfScene.textures[material.pbrMetallicRoughness.baseColorTexture.index].source;
            textureSave(tex, gltfScene.images[imageIndex], intermediateDir, imageIndex);
        }
        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
        {
            ToolkitUtil::TextureResourceT tex;
            tex.target_format = ToolkitUtil::TexturePixelFormat_BC7;

            int32_t imageIndex = gltfScene.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source;
            textureSave(tex, gltfScene.images[imageIndex], intermediateDir, imageIndex);
        }
    }

    return res;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------