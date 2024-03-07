//------------------------------------------------------------------------------
//  ngltfmaterialextractor.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfmaterialexporter.h"
#include "io/ioserver.h"
#include "io/binarywriter.h"
#include "io/filestream.h"
#include "surface/surfacebuilder.h"

using namespace Util;
namespace ToolkitUtil
{
//------------------------------------------------------------------------------
/**
*/
NglTFMaterialExtractor::NglTFMaterialExtractor()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
NglTFMaterialExtractor::~NglTFMaterialExtractor()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<IO::URI>
NglTFMaterialExtractor::ExportAll()
{
    Util::Array<IO::URI> outputFiles;
    String surfaceExportPath = "sur:" + this->catName + "/" + this->subDir;
    this->texCatDir = "tex:" + this->catName;
    this->textureDir = texCatDir + "/" + this->subDir + "/";

    if (this->doc->materials.Size() > 0)
    {
        if (IO::IoServer::Instance()->DirectoryExists(surfaceExportPath))
        {
            // delete all previously generated surfaces
            if (!IO::IoServer::Instance()->DeleteDirectory(surfaceExportPath))
                this->logger->Warning("glTF - Could not delete old directory for gltf-specific surfaces\n");
        }

        // Generate surfaces
        for (IndexT i = 0; i < this->doc->materials.Size(); i++)
        {
            Gltf::Material& material = this->doc->materials[i];

            SurfaceBuilder builder;
            builder.SetLogger(this->logger);
            builder.SetDstDir(surfaceExportPath);

            Util::String templateName;
            if (material.alphaMode == Gltf::Material::AlphaMode::Opaque)
            {
                if (!material.doubleSided)
                    templateName = "GLTF";
                else
                    templateName = "GLTFDoubleSided";
            }
            else if (material.alphaMode == Gltf::Material::AlphaMode::Blend)
            {
                if (!material.doubleSided)
                    templateName = "GLTFAlphaBlend";
                else
                    templateName = "GLTFAlphaBlendDoubleSided";
            }
            else
            {
                if (!material.doubleSided)
                    templateName = "GLTFAlphaMask";
                else
                    templateName = "GLTFAlphaMaskDoubleSided";

                builder.AddParam("alphaCutoff", Util::String::FromFloat(material.alphaCutoff));
            }

            builder.SetMaterial(templateName);
            this->ExtractMaterial(builder, material);
            if (material.name.IsEmpty())
            {
                material.name = "unnamed_";
                material.name.AppendInt(i);
            }
            IO::URI output = surfaceExportPath + "/" + material.name + ".sur";
            builder.ExportBinary(output.AsString());
            outputFiles.Append(std::move(output));
        }
    }
    return outputFiles;
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFMaterialExtractor::ExtractMaterial(SurfaceBuilder& builder, Gltf::Material const& material)
{
    if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
    {
        int baseColorTexture = this->doc->textures[material.pbrMetallicRoughness.baseColorTexture.index].source;
        if (this->doc->images[baseColorTexture].embedded)
            builder.AddParam("baseColorTexture", this->textureDir + Util::String::FromInt(baseColorTexture));
        else
        {
            // texture is not embedded, we need to figure out the correct path to it
            Util::String texFile = this->texCatDir + "/" + this->doc->images[baseColorTexture].uri;
            texFile.StripFileExtension();
            builder.AddParam("baseColorTexture", texFile);
        }
    }
    else
    {
        builder.AddParam("baseColorTexture", "systex:white");
    }

    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
    {
        int metallicRoughnessTexture = this->doc->textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source;
        if (this->doc->images[metallicRoughnessTexture].embedded)
            builder.AddParam("metallicRoughnessTexture", this->textureDir + Util::String::FromInt(metallicRoughnessTexture));
        else
        {
            // texture is not embedded, we need to find the correct path to it
            Util::String texFile = this->texCatDir + "/" + this->doc->images[metallicRoughnessTexture].uri;
            texFile.StripFileExtension();
            builder.AddParam("metallicRoughnessTexture", texFile);
        }
    }
    else
    {
        builder.AddParam("metallicRoughnessTexture", "systex:white");
    }

    if (material.normalTexture.index != -1)
    {
        int normalTexture = this->doc->textures[material.normalTexture.index].source;
        n_assert(normalTexture > -1)
            if (this->doc->images[normalTexture].embedded)
                builder.AddParam("normalTexture", this->textureDir + Util::String::FromInt(normalTexture));
            else
            {
                Util::String texFile = this->texCatDir + "/" + this->doc->images[normalTexture].uri;
                texFile.StripFileExtension();
                builder.AddParam("normalTexture", texFile);
            }
        builder.AddParam("normalScale", Util::String::FromFloat(material.normalTexture.scale));
    }
    else
    {
        builder.AddParam("normalTexture", "systex:normal_color");
    }

    if (material.emissiveTexture.index != -1)
    {
        int emissiveTexture = this->doc->textures[material.emissiveTexture.index].source;
        n_assert(emissiveTexture > -1)
            if (this->doc->images[emissiveTexture].embedded)
                builder.AddParam("emissiveTexture", this->textureDir + Util::String::FromInt(emissiveTexture));
            else
            {
                // texture is not embedded, we need to find the correct path to it
                Util::String texFile = this->texCatDir + "/" + this->doc->images[emissiveTexture].uri;
                texFile.StripFileExtension();
                builder.AddParam("emissiveTexture", texFile);
            }
    }
    else
    {
        builder.AddParam("emissiveTexture", "systex:white");
    }

    if (material.occlusionTexture.index != -1)
    {
        int occlusionTexture = this->doc->textures[material.occlusionTexture.index].source;
        n_assert(occlusionTexture > -1)
            if (this->doc->images[occlusionTexture].embedded)
                builder.AddParam("occlusionTexture", this->textureDir + Util::String::FromInt(occlusionTexture));
            else
            {
                // texture is not embedded, we need to find the correct path to it
                Util::String texFile = this->texCatDir + "/" + this->doc->images[occlusionTexture].uri;
                texFile.StripFileExtension();
                builder.AddParam("occlusionTexture", texFile);
            }
    }
    else
    {
        builder.AddParam("occlusionTexture", "systex:white");
    }

    builder.AddParam("baseColorFactor", Util::String::FromVec4(material.pbrMetallicRoughness.baseColorFactor));
    builder.AddParam("metallicFactor", Util::String::FromFloat(material.pbrMetallicRoughness.metallicFactor));
    builder.AddParam("roughnessFactor", Util::String::FromFloat(material.pbrMetallicRoughness.roughnessFactor));
    builder.AddParam("emissiveFactor", Util::String::FromVec4(material.emissiveFactor.vec));
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
