//------------------------------------------------------------------------------
//  ngltfexporter.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfexporter.h"
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
#include "ngltfmaterialexporter.h"
#include "jobs2/jobs2.h"

#define NEBULA_VALIDATE_GLTF 1

using namespace Util;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::NglTFExporter, 'glte', Base::ExporterBase);

//------------------------------------------------------------------------------
/**
*/
NglTFExporter::NglTFExporter() 
    : texConverter(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
NglTFExporter::~NglTFExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
NglTFExporter::ParseScene()
{
    this->gltfScene = Gltf::Document();
    this->exportedMeshes.Clear();
    bool res = this->gltfScene.Deserialize(this->path.LocalPath().AsCharPtr());

    if (!res)
    {
        n_warning("WARNING: Failed to import '%s'\n\n", this->file.AsCharPtr());
        return false;
    }

    auto scene = new NglTFScene();
    scene->SetName(this->file);
    scene->SetCategory(this->category);
    scene->Setup(&this->gltfScene, this->exportFlags, this->sceneScale);
    this->scene = scene;

    String fileExtension = this->path.LocalPath().GetFileExtension();
    {
        // Extract materials into .sur files
        // Always do this before exporting textures, since the texture names may be changed in the extractor.
        NglTFMaterialExtractor extractor;
        extractor.SetCategoryName(this->category);
        extractor.SetDocument(&this->gltfScene);
        extractor.SetExportSubDirectory(this->file);
        extractor.ExportAll();
    }

    for (IndexT i = 0; i < gltfScene.materials.Size(); i++)
    {
        Gltf::Material const& material = gltfScene.materials[i];
        if (material.normalTexture.index != -1)
        {
            // Set texture attrs for normal textures
            TextureAttrs attrs;
            attrs.SetPixelFormat(TextureAttrs::BC5);
            attrs.SetFlipNormalY(true);

            
            Gltf::Image const& image = gltfScene.images[gltfScene.textures[material.normalTexture.index].source];
            if (image.embedded)
            {
                Util::String intermediateDir = Util::String::Sprintf("%s_%s", this->file.AsCharPtr(), fileExtension.AsCharPtr());
                Util::String intermediateFile = Util::String::Sprintf("%s/%d", intermediateDir.AsCharPtr(), Util::String::FromInt(material.normalTexture.index).AsCharPtr());
                this->texConverter->AddAttributeEntry(intermediateFile, attrs);
            }
            else
            {
                Util::String fileName = image.uri;
                fileName.StripFileExtension();
                Util::String fullFileName = Util::String::Sprintf("%s/%s", this->category.AsCharPtr(), fileName.AsCharPtr());
                this->texConverter->AddAttributeEntry(fullFileName, attrs);
            }
        }
        if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
        {
            // Set texture attrs for normal textures
            TextureAttrs attrs;
            attrs.SetMipMapFilter(TextureAttrs::Filter::Kaiser);
            attrs.SetScaleFilter(TextureAttrs::Filter::Kaiser);
            attrs.SetColorSpace(TextureAttrs::sRGB);

            Gltf::Image const& image = gltfScene.images[gltfScene.textures[material.pbrMetallicRoughness.baseColorTexture.index].source];
            if (image.embedded)
            {
                Util::String intermediateDir = Util::String::Sprintf("%s_%s", this->file.AsCharPtr(), fileExtension.AsCharPtr());
                Util::String intermediateFile = Util::String::Sprintf("%s/%d", intermediateDir.AsCharPtr(), Util::String::FromInt(material.pbrMetallicRoughness.baseColorTexture.index).AsCharPtr());
                this->texConverter->AddAttributeEntry(intermediateFile, attrs);
            }
            else
            {
                Util::String fileName = image.uri;
                fileName.StripFileExtension();
                Util::String fullFileName = Util::String::Sprintf("%s/%s", this->category.AsCharPtr(), fileName.AsCharPtr());
                this->texConverter->AddAttributeEntry(fullFileName, attrs);
            }
        }
        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
        {
            // Set texture attrs for normal textures
            TextureAttrs attrs;
            attrs.SetScaleFilter(TextureAttrs::Kaiser);
            attrs.SetMipMapFilter(TextureAttrs::Kaiser);

            Gltf::Image const& image = gltfScene.images[gltfScene.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source];
            if (image.embedded)
            {
                Util::String intermediateDir = Util::String::Sprintf("%s_%s", this->file.AsCharPtr(), fileExtension.AsCharPtr());
                Util::String intermediateFile = Util::String::Sprintf("%s/%d", intermediateDir.AsCharPtr(), Util::String::FromInt(material.normalTexture.index).AsCharPtr());
                this->texConverter->AddAttributeEntry(intermediateFile, attrs);
            }
            else
            {
                Util::String fileName = image.uri;
                fileName.StripFileExtension();
                Util::String fullFileName = Util::String::Sprintf("%s/%s", this->category.AsCharPtr(), fileName.AsCharPtr());
                this->texConverter->AddAttributeEntry(fullFileName, attrs);
            }
        }
    }

    if (gltfScene.images.Size() > 0)
    {
        // Export embedded textures to file-specific directory
        bool hasEmbedded = false;
        String embeddedPath = "tex:" + this->category + "/" + this->file;
        if (IO::IoServer::Instance()->DirectoryExists(embeddedPath))
        {
            // delete all previously generated images
            if (!IO::IoServer::Instance()->DeleteDirectory(embeddedPath))
                n_warning("    [glTF Warning - Could not delete old directory for embedded gltf images]\n");
        }

        for (IndexT i = 0; i < gltfScene.images.Size(); i++)
        {
            if (gltfScene.images[i].embedded)
            {
                hasEmbedded = true;
                break;
            }
        }

        if (hasEmbedded)
        {
            IO::IoServer::Instance()->CreateDirectory(embeddedPath);

            static const Util::String tmpDir = "temp:textureconverter";

            TextureAttrTable texAttrTable;
            texAttrTable.Setup(tmpDir);

            Util::String intermediateDir =
                Util::String::Sprintf(
                    "%s/%s/%s"
                    , tmpDir.AsCharPtr()
                    , this->category.AsCharPtr()
                    , this->file.AsCharPtr()
                );

            // Create intermediate directory
            IO::IoServer::Instance()->CreateDirectory(intermediateDir);

            struct ImageJob
            {
                Gltf::Image* const* image;
                const Gltf::Document* scene;
                const Util::String* category;
                const Util::String* intermediateDir;
                TextureConverter* converter;
            } imageJob;

            Util::Array<Gltf::Image*> images;
            for (IndexT i = 0; i < gltfScene.images.Size(); i++)
            {
                if (gltfScene.images[i].embedded)
                {
                    images.Append(&gltfScene.images[i]);
                }
            }

            imageJob.image = images.ConstBegin();
            imageJob.scene = &gltfScene;
            imageJob.category = &this->category;
            imageJob.converter = this->texConverter;
            imageJob.intermediateDir = &intermediateDir;

            auto job = [](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
            {
                ImageJob* jobCtx = static_cast<ImageJob*>(ctx);

                for (int i = 0; i < groupSize; i++)
                {
                    int index = invocationOffset + i;
                    if (index >= totalJobs)
                        continue;

                    Gltf::Image const& image = jobCtx->scene->images[index];

                    void const* data;
                    size_t dataSize = 0;

                    if (!image.uri.IsEmpty())
                    {
                        data = image.data.GetPtr();
                        dataSize = image.data.Size();
                    }
                    else
                    {
                        auto const& bufferView = jobCtx->scene->bufferViews[image.bufferView];
                        data = (const char*)jobCtx->scene->buffers[bufferView.buffer].data.GetPtr() + bufferView.byteOffset;
                        dataSize = bufferView.byteLength;
                    }

                    Util::String format = (image.type == Gltf::Image::Type::Jpg) ? ".jpg" : ".png";

                    // export the content of blob to a temporary file
                    Ptr<IO::BinaryWriter> writer = IO::BinaryWriter::Create();
                    Util::String intermediateFile = Util::String::Sprintf("%s/%d%s", jobCtx->intermediateDir->AsCharPtr(), index, format.AsCharPtr());
                    writer->SetStream(IO::IoServer::Instance()->CreateStream(intermediateFile));
                    Logger logger;
                    if (!writer->Open())
                    {
                        logger.Warning("    [glTF Warning - Could not open filestream to write intermediate image format]\n");
                        return;
                    }
                    writer->GetStream()->Write(data, dataSize);
                    writer->Close();

                    auto dstDir = Util::String::Sprintf("tex:%s", jobCtx->category->AsCharPtr());

                    // content is base 64 encoded in uri
                    if (!jobCtx->converter->ConvertTexture(intermediateFile, dstDir, tmpDir))
                    {
                        logger.Error("    [glTF Error - failed to convert texture]\n");
                    }
                }
            };
            Threading::Event event;
            Jobs2::JobDispatch(job, gltfScene.images.Size(), 1, imageJob, nullptr, nullptr, &event);

            event.Wait();

            // Delete temporary directory
            if (IO::IoServer::Instance()->DirectoryExists(tmpDir))
            {
                if (!IO::IoServer::Instance()->DeleteDirectory(tmpDir))
                    n_printf("    [glTF Warning - Could not delete temporary texconverter directory]\n");
            }

            // Reset scratch memory
            Jobs2::JobNewFrame();
        }
    }

    return res;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------