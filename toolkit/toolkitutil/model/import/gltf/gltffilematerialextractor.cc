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

#include "toolkit-common/text.h"

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
*/
Util::Array<IO::URI>
GltfFileMaterialExtractor::ExtractAll()
{
    Util::Array<IO::URI> outputFiles;
    String surfaceExtractPath = this->catName + "/" + this->subDir;
    surfaceExtractPath.StripFileExtension();
    this->textureDir = "tex:" + this->catName.StripSubstring(IO::URI("src:assets").LocalPath());

    if (this->doc->materials.Size() > 0)
    {
        IO::IoServer::Instance()->CreateDirectory(surfaceExtractPath);

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
                materialResource.template_name = names->alphaMask;
                ToolkitUtil::MaterialValueUnion alphaCutoffValue;
                ToolkitUtil::MaterialFloatValueT value; 
                value.value = material.alphaCutoff;
                alphaCutoffValue.Set(value);
                materialResource.value_names.push_back("alphaCutoff");
                materialResource.values.push_back(alphaCutoffValue);
            }

            this->ExtractMaterial(&materialResource, material);
            if (material.name.IsEmpty())
            {
                material.name = "unnamed_";
                material.name.AppendInt(i);
            }
            Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::MaterialResource>(materialResource);
            IO::URI output = surfaceExtractPath + "/" + material.name + ".namat";
            Ptr<IO::Stream> outStream = IO::IoServer::Instance()->CreateStream(output);
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
        int baseColorTexture = this->doc->textures[material.pbrMetallicRoughness.baseColorTexture.index].source;
        ToolkitUtil::MaterialValueUnion baseColorTextureValue;
        ToolkitUtil::MaterialStringValueT value;
        if (this->doc->images[baseColorTexture].embedded)
            value.value = this->textureDir + Util::String::FromInt(baseColorTexture);
        else
        {
            // texture is not embedded, we need to figure out the correct path to it
            Util::String texFile = this->textureDir + "/" + this->doc->images[baseColorTexture].uri;
            texFile.StripFileExtension();
            value.value = texFile;
        }
        baseColorTextureValue.Set(value);

        materialResource->value_names.push_back("baseColorTexture");
        materialResource->values.push_back(baseColorTextureValue);
    }

    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
    {
        int metallicRoughnessTexture = this->doc->textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source;
        ToolkitUtil::MaterialValueUnion metallicRoughnessTextureValue;
        ToolkitUtil::MaterialStringValueT value;
        if (this->doc->images[metallicRoughnessTexture].embedded)
            value.value = this->textureDir + Util::String::FromInt(metallicRoughnessTexture);
        else
        {
            // texture is not embedded, we need to find the correct path to it
            Util::String texFile = this->textureDir + "/" + this->doc->images[metallicRoughnessTexture].uri;
            texFile.StripFileExtension();
            value.value = texFile;
        }
        metallicRoughnessTextureValue.Set(value);
        materialResource->value_names.push_back("metallicRoughnessTexture");
        materialResource->values.push_back(metallicRoughnessTextureValue);
    }
    
    if (material.normalTexture.index != -1)
    {
        int normalTexture = this->doc->textures[material.normalTexture.index].source;
        ToolkitUtil::MaterialValueUnion normalTextureValue;
        ToolkitUtil::MaterialStringValueT value;
        n_assert(normalTexture > -1)
            if (this->doc->images[normalTexture].embedded)
                value.value = this->textureDir + Util::String::FromInt(normalTexture);
            else
            {
                Util::String texFile = this->textureDir + "/" + this->doc->images[normalTexture].uri;
                texFile.StripFileExtension();
                value.value = texFile;
            }
        normalTextureValue.Set(value);
        materialResource->value_names.push_back("normalTexture");
        materialResource->values.push_back(normalTextureValue);

        ToolkitUtil::MaterialValueUnion normalScaleValue;
        auto normalScale = ToolkitUtil::MaterialFloatValueT();
        normalScale.value = material.normalTexture.scale;
        normalScaleValue.Set(normalScale);
        materialResource->value_names.push_back("normalScale");
        materialResource->values.push_back(normalScaleValue);
    }

    if (material.emissiveTexture.index != -1)
    {
        int emissiveTexture = this->doc->textures[material.emissiveTexture.index].source;
        ToolkitUtil::MaterialValueUnion emissiveTextureValue;
        ToolkitUtil::MaterialStringValueT value;
        n_assert(emissiveTexture > -1)
            if (this->doc->images[emissiveTexture].embedded)
                value.value = this->textureDir + Util::String::FromInt(emissiveTexture);
            else
            {
                // texture is not embedded, we need to find the correct path to it
                Util::String texFile = this->textureDir + "/" + this->doc->images[emissiveTexture].uri;
                texFile.StripFileExtension();
                value.value = texFile;
            }
        emissiveTextureValue.Set(value);
        materialResource->value_names.push_back("emissiveTexture");
        materialResource->values.push_back(emissiveTextureValue);
    }

    if (material.occlusionTexture.index != -1)
    {
        int occlusionTexture = this->doc->textures[material.occlusionTexture.index].source;
        ToolkitUtil::MaterialValueUnion occlusionTextureValue;
        ToolkitUtil::MaterialStringValueT value;
        n_assert(occlusionTexture > -1)
            if (this->doc->images[occlusionTexture].embedded)
                value.value = this->textureDir + Util::String::FromInt(occlusionTexture);
            else
            {
                // texture is not embedded, we need to find the correct path to it
                Util::String texFile = this->textureDir + "/" + this->doc->images[occlusionTexture].uri;
                texFile.StripFileExtension();
                value.value = texFile;
            }
        occlusionTextureValue.Set(value);
        materialResource->value_names.push_back("occlusionTexture");
        materialResource->values.push_back(occlusionTextureValue);
    }

    ToolkitUtil::MaterialValueUnion baseColorFactorValue;
    ToolkitUtil::MaterialVec4ValueT baseColorFactor;
    baseColorFactor.value = material.pbrMetallicRoughness.baseColorFactor;
    baseColorFactorValue.Set(baseColorFactor);
    materialResource->value_names.push_back("baseColorFactor");
    materialResource->values.push_back(baseColorFactorValue);

    ToolkitUtil::MaterialValueUnion metallicFactorValue;
    ToolkitUtil::MaterialFloatValueT metallicFactor;
    metallicFactor.value = material.pbrMetallicRoughness.metallicFactor;
    metallicFactorValue.Set(metallicFactor);
    materialResource->value_names.push_back("metallicFactor");
    materialResource->values.push_back(metallicFactorValue);

    ToolkitUtil::MaterialValueUnion roughnessFactorValue;
    ToolkitUtil::MaterialFloatValueT roughnessFactor;
    roughnessFactor.value = material.pbrMetallicRoughness.roughnessFactor;
    roughnessFactorValue.Set(roughnessFactor);
    materialResource->value_names.push_back("roughnessFactor");
    materialResource->values.push_back(roughnessFactorValue);

    ToolkitUtil::MaterialValueUnion emissiveFactorValue;
    ToolkitUtil::MaterialVec4ValueT emissiveFactor;
    emissiveFactor.value = Math::vec4(material.emissiveFactor, 1.0f);
    emissiveFactorValue.Set(emissiveFactor);
    materialResource->value_names.push_back("emissiveFactor");
    materialResource->values.push_back(emissiveFactorValue);
}

} // namespace ToolkitUtil
