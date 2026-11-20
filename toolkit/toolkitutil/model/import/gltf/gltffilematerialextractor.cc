//------------------------------------------------------------------------------
//  ngltfmaterialextractor.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "gltffilematerialextractor.h"
#include "io/ioserver.h"
#include "io/binarywriter.h"
#include "io/filestream.h"
#include "surface/surfacebuilder.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/material.h"

#include "toolkitutil/text.h"

using namespace Util;
namespace ToolkitUtil
{
//------------------------------------------------------------------------------
/**
*/
GltfFileMaterialExtractor::GltfFileMaterialExtractor()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GltfFileMaterialExtractor::~GltfFileMaterialExtractor()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    URNs name an asset's folder relative to the work assets folder, which the output folder lies in
*/
static Util::String
AssetFolder(const Util::String& outputFolder)
{
    Util::String folder = IO::URI(outputFolder).LocalPath();
    folder.TrimRight("/\\");
    if (IO::Path::WorkRoot.IsValid())
    {
        Util::String root = IO::URI(Util::Format("%s/assets", IO::Path::WorkRoot.Value())).LocalPath();
        root.TrimRight("/\\");
        if (folder == root)
            return Util::String();
        if (folder.Length() > root.Length() && folder.BeginsWithString(root) && (folder[root.Length()] == '/' || folder[root.Length()] == '\\'))
            return folder.ExtractToEnd(root.Length() + 1);
    }
    n_warning("GltfFileMaterialExtractor: '%s' is not in the work assets folder, materials refer to their textures by the folder name only\n", folder.AsCharPtr());
    return folder.ExtractFileName();
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<IO::URI>
GltfFileMaterialExtractor::ExtractAll()
{
    Util::Array<IO::URI> outputFiles;
    this->textureDir = AssetFolder(this->outputFolder);

    if (this->doc->materials.Size() > 0)
    {
        IO::CreateDirectory(this->outputFolder);

        // Generate surfaces
        for (IndexT i = 0; i < this->doc->materials.Size(); i++)
        {
            Gltf::Material& material = this->doc->materials[i];

            ToolkitUtil::MaterialResourceT materialResource;
            struct MatTemplateNames
            {
                const char* standardOpaque;
                const char* alphaBlend;
                const char* alphaMask;
            };

            constexpr MatTemplateNames staticNames = {
                .standardOpaque = "GLTF",
                .alphaBlend = "GLTFAlphaBlend",
                .alphaMask = "GLTFAlphaMask",
            };
            constexpr MatTemplateNames doubleSidedNames = {
                .standardOpaque = "GLTFDoubleSided",
                .alphaBlend = "GLTFAlphaBlendDoubleSided",
                .alphaMask = "GLTFAlphaMaskDoubleSided",
            };
            constexpr MatTemplateNames skinnedNames = {
                .standardOpaque = "GLTFSkinned",
                .alphaBlend = "GLTFSkinnedAlphaBlend",
                .alphaMask = "GLTFSkinnedAlphaMask",
            };
            constexpr MatTemplateNames skinnedDoubleSidedNames = {
                .standardOpaque = "GLTFSkinnedDoubleSided",
                .alphaBlend = "GLTFSkinnedAlphaBlendDoubleSided",
                .alphaMask = "GLTFSkinnedAlphaMaskDoubleSided",
            };

            bool const isSkinned = (material.extras == "n_skinned");

            // Categorize into either using face culling or being doublesided, and choose based on if it's skinned or not
            MatTemplateNames const* const cullingCategory[2] = { 
                isSkinned ? &skinnedNames : &staticNames,
                isSkinned ? &skinnedDoubleSidedNames : &doubleSidedNames
            };

            // choose between doublesided or not
            MatTemplateNames const* const names = !material.doubleSided ? cullingCategory[0] : cullingCategory[1];

            // choose between opaque, blend or masked materials
            if (material.alphaMode == Gltf::Material::AlphaMode::Opaque)
            {
                materialResource.template_name = names->standardOpaque;
            }
            else if (material.alphaMode == Gltf::Material::AlphaMode::Blend)
            {
                materialResource.template_name = names->alphaBlend;
            }
            else
            {
                materialResource.value_names.push_back("alphaCutoff");
                materialResource.values.push_back(Util::String::FromFloat(material.alphaCutoff));
            }

            this->ExtractMaterial(&materialResource, material);
            if (material.name.IsEmpty())
            {
                material.name = "unnamed_";
                material.name.AppendInt(i);
            }
            Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::MaterialResource>(materialResource);
            IO::URI output = this->outputFolder + "/" + material.name + ".namat";
            Ptr<IO::Stream> outStream = IO::CreateStream(output);
            outStream->SetAccessMode(IO::Stream::WriteAccess);
            if (outStream->Open())
            {
                outStream->Write(data.GetPtr(), data.Size());
                outStream->Close();

                this->logger->Print("%s\n", Format("Generated material: %s", Text(output.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
            }
            outputFiles.Append(std::move(output));
        }
    }
    return outputFiles;
}

//------------------------------------------------------------------------------
/**
*/
void
GltfFileMaterialExtractor::ExtractMaterial(ToolkitUtil::MaterialResourceT* materialResource, Gltf::Material const& material)
{
    if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
    {
        materialResource->value_names.push_back("baseColorTexture");
        materialResource->values.push_back(this->TextureURN(material.pbrMetallicRoughness.baseColorTexture.index));
    }

    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
    {
        materialResource->value_names.push_back("metallicRoughnessTexture");
        materialResource->values.push_back(this->TextureURN(material.pbrMetallicRoughness.metallicRoughnessTexture.index));
    }

    if (material.normalTexture.index != -1)
    {
        materialResource->value_names.push_back("normalTexture");
        materialResource->values.push_back(this->TextureURN(material.normalTexture.index));

        materialResource->value_names.push_back("normalScale");
        materialResource->values.push_back(Util::String::FromFloat(material.normalTexture.scale));
    }

    if (material.emissiveTexture.index != -1)
    {
        materialResource->value_names.push_back("emissiveTexture");
        materialResource->values.push_back(this->TextureURN(material.emissiveTexture.index));
    }

    if (material.occlusionTexture.index != -1)
    {
        materialResource->value_names.push_back("occlusionTexture");
        materialResource->values.push_back(this->TextureURN(material.occlusionTexture.index));
    }

    materialResource->value_names.push_back("baseColorFactor"); 
    materialResource->values.push_back(Util::String::FromVec4(material.pbrMetallicRoughness.baseColorFactor));

    materialResource->value_names.push_back("metallicFactor");
    materialResource->values.push_back(Util::String::FromFloat(material.pbrMetallicRoughness.metallicFactor));

    materialResource->value_names.push_back("roughnessFactor");
    materialResource->values.push_back(Util::String::FromFloat(material.pbrMetallicRoughness.roughnessFactor));

    materialResource->value_names.push_back("emissiveFactor");
    materialResource->values.push_back(Util::String::FromVec3(material.emissiveFactor));
}

//------------------------------------------------------------------------------
/**
    Embedded images have no name of their own and are named after their index, external images after their file.
    The importer saves the textures under these names, so it has to agree with the URNs the materials use
*/
Util::String
GltfFileMaterialExtractor::TextureName(Gltf::Document const& document, int32_t imageIndex)
{
    Gltf::Image const& image = document.images[imageIndex];
    if (image.embedded)
        return Util::String::FromInt(imageIndex);
    Util::String name = image.uri.ExtractFileName();
    name.StripFileExtension();
    return name;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
GltfFileMaterialExtractor::TextureURN(int32_t textureIndex) const
{
    int32_t const imageIndex = this->doc->textures[textureIndex].source;
    n_assert(imageIndex > -1);
    return IO::Path::File(this->textureDir, TextureName(*this->doc, imageIndex), "tex").AsString();
}

} // namespace ToolkitUtil
