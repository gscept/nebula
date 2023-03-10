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
    String subDir = this->file + "_" + fileExtension;
    {
        // Extract materials into .sur files
        // Always do this before exporting textures, since the texture names may be changed in the extractor.
        NglTFMaterialExtractor extractor;
        extractor.SetCategoryName(this->category);
        extractor.SetDocument(&this->gltfScene);
        extractor.SetExportSubDirectory(subDir);
        extractor.ExportAll();
    }

    if (gltfScene.images.Size() > 0)
    {
        // Export embedded textures to file-specific directory
        bool hasEmbedded = false;
        String embeddedPath = "tex:" + this->category + "/" + this->file + "_" + fileExtension;
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

            TextureAttrTable texAttrTable;
            texAttrTable.Setup("temp:texconverter");

            for (IndexT i = 0; i < gltfScene.materials.Size(); i++)
            {
                Gltf::Material const& material = gltfScene.materials[i];
                if (material.normalTexture.index != -1)
                {
                    // Set texture attrs for normal textures
                    TextureAttrs attrs;
                    attrs.SetPixelFormat(TextureAttrs::BC5);
                    attrs.SetFlipNormalY(true);

                    Gltf::Image const& image = gltfScene.images[material.normalTexture.index];
                    Util::String format = (image.type == Gltf::Image::Type::Jpg) ? ".jpg" : ".png";
                    Util::String intermediateDir = this->file + "_" + fileExtension;
                    Util::String intermediateFile = intermediateDir + "/" + Util::String::FromInt(material.normalTexture.index);
                    texAttrTable.SetEntry(intermediateFile, attrs);
                }
                else if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
                {
                    // Set texture attrs for normal textures
                    TextureAttrs attrs;
                    attrs.SetPixelFormat(TextureAttrs::BC7);
                    attrs.SetColorSpace(TextureAttrs::ColorSpace::sRGB);

                    Gltf::Image const& image = gltfScene.images[material.pbrMetallicRoughness.baseColorTexture.index];
                    Util::String format = (image.type == Gltf::Image::Type::Jpg) ? ".jpg" : ".png";
                    Util::String intermediateDir = this->file + "_" + fileExtension;
                    Util::String intermediateFile = intermediateDir + "/" + Util::String::FromInt(material.pbrMetallicRoughness.baseColorTexture.index);
                    texAttrTable.SetEntry(intermediateFile, attrs);
                }
            }

            texConverter->SetExternalTextureAttrTable(&texAttrTable);

            // export all embedded images
            for (IndexT i = 0; i < gltfScene.images.Size(); i++)
            {
                Gltf::Image const& image = gltfScene.images[i];
                if (!image.embedded)
                    continue;

                void const* data;
                size_t dataSize = 0;

                if (!image.uri.IsEmpty())
                {
                    data = image.data.GetPtr();
                    dataSize = image.data.Size();
                }
                else
                {
                    auto const& bufferView = gltfScene.bufferViews[image.bufferView];
                    data = (const char*)gltfScene.buffers[bufferView.buffer].data.GetPtr() + bufferView.byteOffset;
                    dataSize = bufferView.byteLength;
                }

                // create temp directory from guid. so that other jobs won't interfere
                //Guid guid;
                //guid.Generate();
                //String tmpDir;
                //tmpDir.Format("%s", "temp:textureconverter", guid.AsString().AsCharPtr());

                String tmpDir = "temp:textureconverter";

                Util::String format = (image.type == Gltf::Image::Type::Jpg) ? ".jpg" : ".png";
                // export the content of blob to a temporary file
                Ptr<IO::BinaryWriter> writer = IO::BinaryWriter::Create();
                Util::String intermediateDir = tmpDir + "/" + this->category + "/" + this->file + "_" + fileExtension;
                Util::String intermediateFile = intermediateDir + "/" + Util::String::FromInt(i) + format;
                IO::IoServer::Instance()->CreateDirectory(intermediateDir);
                writer->SetStream(IO::IoServer::Instance()->CreateStream(intermediateFile));
                if (!writer->Open())
                {
                    n_warning("    [glTF Warning - Could not open filestream to write intermediate image format]\n");
                    return false;
                }
                writer->GetStream()->Write(data, dataSize);
                writer->Close();

                this->texConverter->SetDstDir("tex:" + this->category + "/");
                // content is base 64 encoded in uri
                if (!this->texConverter->ConvertTexture(intermediateFile, tmpDir))
                {
                    n_error("    [glTF Error - failed to convert texture]\n");
                }
                if (IO::IoServer::Instance()->DirectoryExists(tmpDir))
                {
                    if (!IO::IoServer::Instance()->DeleteDirectory(tmpDir))
                        n_warning("    [glTF Warning - Could not delete temporary texconverter directory]\n");
                }
            }
        }
    }

    return res;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------