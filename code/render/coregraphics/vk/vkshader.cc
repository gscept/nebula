//------------------------------------------------------------------------------
// vkshader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkshader.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/sampler.h"
#include "coregraphics/resourcetable.h"
#include "vksampler.h"
#include "vkgraphicsdevice.h"

namespace Vulkan
{


Util::Dictionary<Util::StringAtom, VkDescriptorSetLayout> VkShaderLayoutCache;
Util::Dictionary<Util::StringAtom, VkPipelineLayout> VkShaderPipelineCache;
Util::Dictionary<Util::StringAtom, VkDescriptorSet> VkShaderDescriptorSetCache;
ShaderAllocator shaderAlloc;

enum ShaderStages
{
    VertexShader,
    HullShader,
    DomainShader,
    GeometryShader,
    PixelShader,
    ComputeShader,
    TaskShader,
    MeshShader,
    RayGenerationShader,
    RayAnyHitShader,
    RayClosestHitShader,
    RayMissShader,
    RayIntersectionShader,
    CallableShader,

    NumShaders
};

//------------------------------------------------------------------------------
/**
*/
void
UpdateOccupancy(uint32_t* occupancyList, bool& slotUsed, const CoreGraphics::ShaderVisibility vis)
{
    if (AllBits(vis, CoreGraphics::VertexShaderVisibility))             { occupancyList[VertexShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::HullShaderVisibility))               { occupancyList[HullShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::DomainShaderVisibility))             { occupancyList[DomainShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::GeometryShaderVisibility))           { occupancyList[GeometryShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::PixelShaderVisibility))              { occupancyList[PixelShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::TaskShaderVisibility))               { occupancyList[TaskShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::MeshShaderVisibility))               { occupancyList[MeshShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::ComputeShaderVisibility))            { occupancyList[ComputeShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::RayGenerationShaderVisibility))      { occupancyList[RayGenerationShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::RayAnyHitShaderVisibility))          { occupancyList[RayAnyHitShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::RayClosestHitShaderVisibility))      { occupancyList[RayClosestHitShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::RayMissShaderVisibility))            { occupancyList[RayMissShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::RayIntersectionShaderVisibility))    { occupancyList[RayIntersectionShader]++; slotUsed = true; }
    if (AllBits(vis, CoreGraphics::CallableShaderVisibility))           { occupancyList[CallableShader]++; slotUsed = true; }
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateValueDescriptorSet(bool& dynamic, uint32_t& num, uint32_t& numDyn, const unsigned set)
{
    if (set == NEBULA_DYNAMIC_OFFSET_GROUP || set == NEBULA_INSTANCE_GROUP)
    {
        dynamic = true;
        numDyn++;
    }
    num++;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SamplerFilter
SamplerFilterFromGPULang(GPULang::Serialization::Filter filter)
{
    switch (filter)
    {
        case GPULang::Serialization::Filter::PointFilter:
            return CoreGraphics::SamplerFilter::NearestFilter;
        case GPULang::Serialization::Filter::LinearFilter:
            return CoreGraphics::SamplerFilter::LinearFilter;
    }
    n_error("Invalid GPULang filter mode");
    return CoreGraphics::SamplerFilter::LinearFilter;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SamplerMipMode
SamplerMipFilterFromGPULang(GPULang::Serialization::Filter filter)
{
    switch (filter)
    {
        case GPULang::Serialization::Filter::PointFilter:
            return CoreGraphics::SamplerMipMode::NearestMipMode;
        case GPULang::Serialization::Filter::LinearFilter:
            return CoreGraphics::SamplerMipMode::LinearMipMode;
    }
    n_error("Invalid GPULang filter mode");
    return CoreGraphics::SamplerMipMode::LinearMipMode;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SamplerCompareOperation
SamplerCompareOpFromGPULang(GPULang::Serialization::CompareMode mode)
{
    switch (mode)
    {
        case GPULang::Serialization::CompareMode::NeverCompare:
            return CoreGraphics::SamplerCompareOperation::NeverCompare;
        case GPULang::Serialization::CompareMode::LessCompare:
            return CoreGraphics::SamplerCompareOperation::LessCompare;
        case GPULang::Serialization::CompareMode::EqualCompare:
            return CoreGraphics::SamplerCompareOperation::EqualCompare;
        case GPULang::Serialization::CompareMode::LessEqualCompare:
            return CoreGraphics::SamplerCompareOperation::LessOrEqualCompare;
        case GPULang::Serialization::CompareMode::GreaterCompare:
            return CoreGraphics::SamplerCompareOperation::GreaterCompare;
        case GPULang::Serialization::CompareMode::NotEqualCompare:
            return CoreGraphics::SamplerCompareOperation::NotEqualCompare;
        case GPULang::Serialization::CompareMode::GreaterEqualCompare:
            return CoreGraphics::SamplerCompareOperation::GreaterOrEqualCompare;
        case GPULang::Serialization::CompareMode::AlwaysCompare:
            return CoreGraphics::SamplerCompareOperation::AlwaysCompare;
    }
    n_error("Invalid GPULang compare mode");
    return CoreGraphics::SamplerCompareOperation::AlwaysCompare;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SamplerAddressMode
SamplerAddressModeFromGPULang(GPULang::Serialization::AddressMode address)
{
    switch (address)
    {
        case GPULang::Serialization::AddressMode::RepeatAddressMode:
            return CoreGraphics::SamplerAddressMode::RepeatAddressMode;
        case GPULang::Serialization::AddressMode::MirrorAddressMode:
            return CoreGraphics::SamplerAddressMode::MirroredRepeatAddressMode;
        case GPULang::Serialization::AddressMode::ClampAddressMode:
            return CoreGraphics::SamplerAddressMode::ClampToEdgeAddressMode;
        case GPULang::Serialization::AddressMode::BorderAddressMode:
            return CoreGraphics::SamplerAddressMode::ClampToBorderAddressMode;
    }
    n_error("Invalid GPULang address mode");
    return CoreGraphics::SamplerAddressMode::RepeatAddressMode;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::SamplerBorderMode
SamplerBorderModeFromGPULang(GPULang::Serialization::BorderColor color)
{
    switch (color)
    {
    case GPULang::Serialization::BorderColor::TransparentBorder:
        return CoreGraphics::SamplerBorderMode::FloatTransparentBlackBorder;
    case GPULang::Serialization::BorderColor::BlackBorder:
        return CoreGraphics::SamplerBorderMode::FloatOpaqueBlackBorder;
    case GPULang::Serialization::BorderColor::WhiteBorder:
        return CoreGraphics::SamplerBorderMode::FloatOpaqueWhiteBorder;
    }
    n_error("Invalid GPULang border color");
    return CoreGraphics::SamplerBorderMode::FloatOpaqueBlackBorder;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ShaderVisibility
ShaderVisibilityFromGPULang(const GPULang::ShaderUsage usage)
{
    CoreGraphics::ShaderVisibility ret = CoreGraphics::InvalidVisibility;
    ret |= usage.flags.vertexShader ? CoreGraphics::VertexShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.hullShader ? CoreGraphics::HullShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.domainShader ? CoreGraphics::DomainShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.geometryShader ? CoreGraphics::GeometryShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.pixelShader ? CoreGraphics::PixelShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.computeShader ? CoreGraphics::ComputeShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.taskShader ? CoreGraphics::TaskShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.meshShader ? CoreGraphics::MeshShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.rayGenerationShader ? CoreGraphics::RayGenerationShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.rayAnyHitShader ? CoreGraphics::RayAnyHitShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.rayClosestHitShader ? CoreGraphics::RayClosestHitShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.rayMissShader ? CoreGraphics::RayMissShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.rayIntersectionShader ? CoreGraphics::RayIntersectionShaderVisibility : CoreGraphics::InvalidVisibility;
    ret |= usage.flags.rayCallableShader ? CoreGraphics::CallableShaderVisibility : CoreGraphics::InvalidVisibility;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderSetup(
    VkDevice dev,
    const Util::StringAtom& name,
    GPULang::Loader* loader,
    Util::FixedArray<CoreGraphics::ResourcePipelinePushConstantRange>& constantRange,
    Util::Set<CoreGraphics::SamplerId>& immutableSamplers,
    Util::FixedArray<Util::Pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
    Util::Dictionary<uint32_t, uint32_t>& setLayoutMap,
    CoreGraphics::ResourcePipelineId& pipelineLayout,
    Util::Dictionary<Util::StringAtom, uint32_t>& resourceSlotMapping,
    Util::Dictionary<Util::StringAtom, IndexT>& constantBindings
)
{
    using namespace CoreGraphics;

    // construct layout create info
    ResourceTableLayoutCreateInfo layoutInfo;
    Util::Dictionary<uint32_t, ResourceTableLayoutCreateInfo> layoutCreateInfos;
    uint32_t numsets = 0;

        // always create push constant range in layout, making all shaders using push constants compatible
    uint32_t maxPushConstantBytes = CoreGraphics::MaxPushConstantSize;
    uint32_t pushRangeOffset = 0; // we must append previous push range size to offset
    constantRange.Resize(NumShaders); // one per shader stage
    uint i;
    for (i = 0; i < NumShaders; i++)
        constantRange[i] = CoreGraphics::ResourcePipelinePushConstantRange{ 0,0,InvalidVisibility };

    // assert we are not over-stepping any uniform buffer limit we are using, perStage is used for ALL_STAGES
    uint32_t numUniformDyn = 0;
    uint32_t numUniform = 0;
    uint32_t numStorageDyn = 0;
    uint32_t numStorage = 0;

    uint32_t numPerStageUniformBuffers[NumShaders] = {0};
    uint32_t numPerStageStorageBuffers[NumShaders] = {0};
    uint32_t numPerStageSamplers[NumShaders] = {0};
    uint32_t numPerStageSampledImages[NumShaders] = {0};
    uint32_t numPerStageStorageImages[NumShaders] = {0};
    uint32_t numPerStageInputAttachments[NumShaders] = {0};
    uint32_t numPerStageAccelerationStructures[NumShaders] = {0};
    uint32_t numInputAttachments = 0;
    bool bindingTable[128] = { false };

    enum
    {
        SamplerSlots,
        ConstantBufferSlots,
        RWBufferSlots,
        ImageSlots,
        RWImageSlots,
        PixelCacheSlots,
        AccelerationStructureSlots,

        NumSlots
    };
    uint32_t slotsUsed[NumSlots] = {0};

    uint32_t maxTextures = CoreGraphics::MaxResourceTableSampledImages;
    uint32_t remainingTextures = maxTextures;

    auto it = loader->nameToObject.begin();
    while (it != loader->nameToObject.end())
    {
        auto& [name, object] = *it;
        if (object->type == GPULang::Serialize::Type::SamplerStateType)
        {
            auto sampler = (GPULang::Deserialize::SamplerState*)object;
            SamplerCreateInfo info;
            info.magFilter = SamplerFilterFromGPULang(sampler->magFilter);
            info.minFilter = SamplerFilterFromGPULang(sampler->minFilter);
            info.mipmapMode = SamplerMipFilterFromGPULang(sampler->mipFilter);
            info.addressModeU = SamplerAddressModeFromGPULang(sampler->addressU);
            info.addressModeV = SamplerAddressModeFromGPULang(sampler->addressV);
            info.addressModeW = SamplerAddressModeFromGPULang(sampler->addressW);
            info.mipLodBias = sampler->mipLodBias;
            info.anisotropyEnable = sampler->anisotropicEnabled;
            info.maxAnisotropy = sampler->maxAnisotropy;
            info.compareEnable = sampler->compareSamplerEnabled;
            info.compareOp = SamplerCompareOpFromGPULang(sampler->compareMode);
            info.minLod = sampler->minLod;
            info.maxLod = sampler->maxLod;
            info.borderColor = SamplerBorderModeFromGPULang(sampler->borderColor);
            info.unnormalizedCoordinates = sampler->unnormalizedSamplingEnabled;
            SamplerId samp = CreateSampler(info);
            immutableSamplers.Add(samp);

            uint32_t annotationBits = ShaderVisibility::AllVisibility;
            for (size_t i = 0; i < sampler->annotationCount; i++)
            {
                const auto& annotation = sampler->annotations[i];
                if (Util::String(annotation.name, annotation.nameLength) == "Visibility")
                {
                    annotationBits = ShaderVisibilityFromString(Util::String(annotation.data.s.string, annotation.data.s.length));
                }
            }

            ResourceTableLayoutSampler sampBinding;
            sampBinding.slot = sampler->binding;
            sampBinding.visibility = ShaderVisibilityFromGPULang(sampler->visibility) | CoreGraphics::ShaderVisibility(annotationBits);
            sampBinding.sampler = samp;

            if (sampler->binding != 0xFFFFFFFF)
            {
                bool occupiesNewBinding = !bindingTable[sampler->binding];
                bindingTable[sampler->binding] = true;
                bool slotUsed = false;

                if (occupiesNewBinding)
                {
                    UpdateOccupancy(numPerStageUniformBuffers, slotUsed, sampBinding.visibility);
                }
                if (slotUsed)
                {
                    slotsUsed[SamplerSlots] += slotUsed ? 1 : 0;
                }
            }
            resourceSlotMapping.Add(sampler->name, sampler->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(sampler->group);
            numsets = Math::max(numsets, sampler->group + 1);
            rinfo.samplers.Append(sampBinding);

        }
        it++;
    }

    for (size_t i = 0; i < loader->variables.size(); i++)
    {
        const GPULang::Deserialize::Variable* variable = loader->variables[i];

        // Skip variables without any visibility
        //if (variable->visibility.bits == 0x0)
            //continue;

        uint32_t annotationBits = ShaderVisibility::AllVisibility;
        for (size_t i = 0; i < variable->annotationCount; i++)
        {
            const auto& annotation = variable->annotations[i];
            if (Util::String(annotation.name, annotation.nameLength) == "Visibility")
            {
                annotationBits = ShaderVisibilityFromString(Util::String(annotation.data.s.string, annotation.data.s.length));
            }
        }

        if (variable->bindingType == GPULang::Serialization::BindingType::Buffer)
        {
            ResourceTableLayoutConstantBuffer cbo;
            cbo.slot = variable->binding;
            cbo.num = 1;
            if (variable->arraySizeCount > 0)
                cbo.num = variable->arraySizes[0];
            cbo.visibility = ShaderVisibilityFromGPULang(variable->visibility) | CoreGraphics::ShaderVisibility(annotationBits);
            bool slotUsed = false;
            if (variable->binding != 0xFFFFFFFF)
            {
                bool occupiesNewBinding = !bindingTable[variable->binding];
                bindingTable[variable->binding] = true;

                if (occupiesNewBinding)
                {
                    UpdateOccupancy(numPerStageUniformBuffers, slotUsed, cbo.visibility);
                }
            }
            resourceSlotMapping.Add(variable->name, variable->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(variable->group);
            numsets = Math::max(numsets, variable->group + 1);

            if (variable->group == NEBULA_DYNAMIC_OFFSET_GROUP || variable->group == NEBULA_INSTANCE_GROUP)
            {
                cbo.dynamicOffset = true; numUniformDyn += slotUsed ? 1 : 0;
            }
            else
            {
                cbo.dynamicOffset = false; numUniform += slotUsed ? 1 : 0;
            }

            rinfo.constantBuffers.Append(cbo);
            n_assert(variable->byteSize <= CoreGraphics::MaxConstantBufferSize);
        }
        else if (variable->bindingType == GPULang::Serialization::BindingType::Inline)
        {
            n_assert(variable->byteSize <= maxPushConstantBytes);
            maxPushConstantBytes -= variable->byteSize;
            CoreGraphics::ResourcePipelinePushConstantRange range;
            range.offset = pushRangeOffset;
            range.size = variable->byteSize;
            range.vis = AllGraphicsVisibility;// ShaderVisibilityFromGPULang(variable->visibility) | annotationBits;
            constantRange[0] = range; // okay, this is hacky
            pushRangeOffset += variable->byteSize;
        }
        else if (variable->bindingType == GPULang::Serialization::BindingType::MutableBuffer)
        {
            ResourceTableLayoutShaderRWBuffer rwbo;
            rwbo.slot = variable->binding;
            rwbo.num = 1;
            if (variable->arraySizeCount > 0)
                rwbo.num = variable->arraySizes[0];
            rwbo.visibility = ShaderVisibilityFromGPULang(variable->visibility) | CoreGraphics::ShaderVisibility(annotationBits);
            bool slotUsed = false;
            if (variable->binding != 0xFFFFFFFF)
            {
                bool occupiesNewBinding = !bindingTable[variable->binding];
                bindingTable[variable->binding] = true;

                if (occupiesNewBinding)
                {
                    UpdateOccupancy(numPerStageStorageBuffers, slotUsed, rwbo.visibility);
                }
            }
            resourceSlotMapping.Add(variable->name, variable->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(variable->group);
            numsets = Math::max(numsets, variable->group + 1);

            if (variable->group == NEBULA_DYNAMIC_OFFSET_GROUP || variable->group == NEBULA_INSTANCE_GROUP)
            {
                rwbo.dynamicOffset = true; numStorageDyn += slotUsed ? 1 : 0;
            }
            else
            {
                rwbo.dynamicOffset = false; numStorage += slotUsed ? 1 : 0;
            }

            rinfo.rwBuffers.Append(rwbo);
        }
        else if (variable->bindingType == GPULang::Serialization::BindingType::Sampler)
        {
            ResourceTableLayoutSampler samp;
            samp.slot = variable->binding;
            samp.visibility = ShaderVisibilityFromGPULang(variable->visibility) | CoreGraphics::ShaderVisibility(annotationBits);
            samp.sampler = CoreGraphics::InvalidSamplerId;
            bool slotUsed = false;
            if (variable->binding != 0xFFFFFFFF)
            {
                bool occupiesNewBinding = !bindingTable[variable->binding];
                bindingTable[variable->binding] = true;

                if (occupiesNewBinding)
                {
                    UpdateOccupancy(numPerStageSamplers, slotUsed, samp.visibility);
                }
            }
            resourceSlotMapping.Add(variable->name, variable->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(variable->group);
            numsets = Math::max(numsets, variable->group + 1);
            rinfo.samplers.Append(samp);
        }
        else if (variable->bindingType == GPULang::Serialization::BindingType::Image)
        {
            ResourceTableLayoutTexture tex;
            tex.slot = variable->binding;
            tex.visibility = ShaderVisibilityFromGPULang(variable->visibility) | CoreGraphics::ShaderVisibility(annotationBits);
            tex.num = 1;
            if (variable->arraySizeCount > 0)
                tex.num = variable->arraySizes[0];
            tex.immutableSampler = CoreGraphics::InvalidSamplerId;
            bool slotUsed = false;


            if (variable->binding != 0xFFFFFFFF)
            {
                bool occupiesNewBinding = !bindingTable[variable->binding];
                bindingTable[variable->binding] = true;

                if (occupiesNewBinding)
                {
                    UpdateOccupancy(numPerStageSampledImages, slotUsed, tex.visibility);
                }
            }
            resourceSlotMapping.Add(variable->name, variable->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(variable->group);
            numsets = Math::max(numsets, variable->group + 1);
            rinfo.textures.Append(tex);
        }
        else if (variable->bindingType == GPULang::Serialization::BindingType::MutableImage)
        {
            ResourceTableLayoutTexture tex;
            tex.slot = variable->binding;
            tex.visibility = ShaderVisibilityFromGPULang(variable->visibility) | CoreGraphics::ShaderVisibility(annotationBits);
            tex.num = 1;
            if (variable->arraySizeCount > 0)
                tex.num = variable->arraySizes[0];
            tex.immutableSampler = CoreGraphics::InvalidSamplerId;
            bool slotUsed = false;
            if (variable->binding != 0xFFFFFFFF)
            {
                bool occupiesNewBinding = !bindingTable[variable->binding];
                bindingTable[variable->binding] = true;

                if (occupiesNewBinding)
                {
                    UpdateOccupancy(numPerStageStorageImages, slotUsed, tex.visibility);
                }
            }
            resourceSlotMapping.Add(variable->name, variable->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(variable->group);
            numsets = Math::max(numsets, variable->group + 1);
            rinfo.rwTextures.Append(tex);
        }
        else if (variable->bindingType == GPULang::Serialization::BindingType::PixelCache)
        {
            ResourceTableLayoutInputAttachment tex;
            tex.slot = variable->binding;
            tex.visibility = ShaderVisibility::PixelShaderVisibility;
            tex.num = 1;
            bool slotUsed = false;

            if (variable->arraySizeCount > 0)
                tex.num = variable->arraySizes[0];

            if (variable->binding != 0xFFFFFFFF)
            {
                bool occupiesNewBinding = !bindingTable[variable->binding];
                bindingTable[variable->binding] = true;

                if (occupiesNewBinding)
                {
                    UpdateOccupancy(numPerStageInputAttachments, slotUsed, tex.visibility);
                }
            }
            resourceSlotMapping.Add(variable->name, variable->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(variable->group);
            numsets = Math::max(numsets, variable->group + 1);
            rinfo.inputAttachments.Append(tex);
        }
        else if (variable->bindingType == GPULang::Serialization::BindingType::AccelerationStructure)
        {
            ResourceTableLayoutAccelerationStructure bvh;
            bvh.slot = variable->binding;
            bvh.visibility = ShaderVisibilityFromGPULang(variable->visibility) | CoreGraphics::ShaderVisibility(annotationBits);
            bvh.num = 1;
            bool slotUsed = false;

            if (variable->binding != 0xFFFFFFFF)
            {
                bool occupiesNewBinding = !bindingTable[variable->binding];
                bindingTable[variable->binding] = true;

                if (occupiesNewBinding)
                {
                    UpdateOccupancy(numPerStageAccelerationStructures, slotUsed, bvh.visibility);
                }
            }
            resourceSlotMapping.Add(variable->name, variable->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(variable->group);
            numsets = Math::max(numsets, variable->group + 1);
            rinfo.accelerationStructures.Append(bvh);
        }
    }
    n_assert(CoreGraphics::MaxResourceTableDynamicOffsetConstantBuffers >= numUniformDyn);
    n_assert(CoreGraphics::MaxResourceTableConstantBuffers >= numUniform);
        for (uint i = 0; i < NumShaders; i++)
        n_assert(CoreGraphics::MaxPerStageConstantBuffers >= numPerStageUniformBuffers[i]);

    n_assert(CoreGraphics::MaxResourceTableDynamicOffsetReadWriteBuffers >= numStorageDyn);
    n_assert(CoreGraphics::MaxResourceTableReadWriteBuffers >= numStorage);
        for (uint i = 0; i < NumShaders; i++)
        n_assert(CoreGraphics::MaxPerStageReadWriteBuffers >= numPerStageStorageBuffers[i]);


    // skip the rest if we don't have any descriptor sets
    if (!layoutCreateInfos.IsEmpty())
    {
        setLayouts.Resize(numsets);
        for (IndexT i = 0; i < layoutCreateInfos.Size(); i++)
        {
            const ResourceTableLayoutCreateInfo& info = layoutCreateInfos.ValueAtIndex(i);
            ResourceTableLayoutId layout = CreateResourceTableLayout(info);
            setLayouts[i] = Util::MakePair(layoutCreateInfos.KeyAtIndex(i), layout);
            setLayoutMap.Add(layoutCreateInfos.KeyAtIndex(i), i);

#if NEBULA_GRAPHICS_DEBUG
            ObjectSetName(layout, Util::String::Sprintf("%s - Resource Table Layout %d", name.Value(), i).AsCharPtr());
#endif
        }
    }

    Util::Array<ResourceTableLayoutId> layoutList;
    Util::Array<uint32_t> layoutIndices;
    for (IndexT i = 0; i < setLayouts.Size(); i++)
    {
        const ResourceTableLayoutId& a = Util::Get<1>(setLayouts[i]);
        if (a != ResourceTableLayoutId::Invalid())
        {
            layoutList.Append(a);
            layoutIndices.Append(Util::Get<0>(setLayouts[i]));
        }
    }

    ResourcePipelineCreateInfo piInfo =
    {
        layoutList, layoutIndices, constantRange[0]
    };
    pipelineLayout = CreateResourcePipeline(piInfo);

#if NEBULA_GRAPHICS_DEBUG
    ObjectSetName(pipelineLayout, Util::String::Sprintf("%s - Resource Pipeline", name.Value()).AsCharPtr());
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderCleanup(
    VkDevice dev,
    Util::Set<CoreGraphics::SamplerId>& immutableSamplers,
    Util::FixedArray<Util::Pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
    Util::Dictionary<Util::StringAtom, CoreGraphics::BufferId>& buffers,
    CoreGraphics::ResourcePipelineId& pipelineLayout
)
{
    IndexT i;
    for (i = 0; i < immutableSamplers.Size(); i++)
    {
        CoreGraphics::DestroySampler(immutableSamplers.KeyAtIndex(i));
    }
    immutableSamplers.Clear();

    for (i = 0; i < setLayouts.Size(); i++)
    {
        if (Util::Get<1>(setLayouts[i]) != CoreGraphics::ResourceTableLayoutId::Invalid())
            CoreGraphics::DestroyResourceTableLayout(Util::Get<1>(setLayouts[i]));
    }
    setLayouts.Clear();

    for (i = 0; i < buffers.Size(); i++)
    {
        CoreGraphics::DestroyBuffer(buffers.ValueAtIndex(i));
    }
    buffers.Clear();

    CoreGraphics::DestroyResourcePipeline(pipelineLayout);
}

//------------------------------------------------------------------------------
/**
*/
Util::String
VkShaderCreateSignature(const VkDescriptorSetLayoutBinding& bind)
{
    return Util::String::Sprintf("%d:%d:%d:%d:%p;", bind.binding, bind.descriptorCount, bind.descriptorType, bind.stageFlags, bind.pImmutableSamplers);
}

//------------------------------------------------------------------------------
/**
    Use direct resource ids, not the State, Shader or Variable type ids
*/
const VkProgramReflectionInfo&
ShaderGetProgramReflection(const CoreGraphics::ShaderProgramId shaderProgramId)
{
    return shaderProgramAlloc.Get<ShaderProgram_ReflectionInfo>(shaderProgramId.programId);
}

} // namespace Vulkan

namespace CoreGraphics
{
using namespace Vulkan;


//------------------------------------------------------------------------------
/**
*/
const ShaderId
CreateShader(const GPULangShaderCreateInfo& info)
{
    Ids::Id32 id = shaderAlloc.Alloc();
    VkReflectionInfo& reflectionInfo = shaderAlloc.Get<Shader_ReflectionInfo>(id);
    VkShaderSetupInfo& setupInfo = shaderAlloc.Get<Shader_SetupInfo>(id);
    VkShaderRuntimeInfo& runtimeInfo = shaderAlloc.Get<Shader_RuntimeInfo>(id);

    ShaderId ret = id;

    setupInfo.id = ShaderIdentifier::FromName(info.name);
    setupInfo.name = info.name;
    setupInfo.dev = Vulkan::GetCurrentDevice();

    // the setup code is massive, so just let it be in VkShader...
    ShaderSetup(
        setupInfo.dev,
        info.name,
        info.loader,
        setupInfo.constantRangeLayout,
        setupInfo.immutableSamplers,
        setupInfo.descriptorSetLayouts,
        setupInfo.descriptorSetLayoutMap,
        setupInfo.pipelineLayout,
        setupInfo.resourceIndexMap,
        setupInfo.constantBindings
    );

   
    for (auto& [name, object] : info.loader->nameToObject)
    {
        if (object->type == GPULang::Serialize::Type::VariableType)
        {
            auto variable = (GPULang::Deserialize::Variable*)object;

            // If variable is a uniform buffer, store it in the reflection data
            if (variable->bindingType == GPULang::Serialization::BindingType::Buffer)
            {
                n_assert(variable->structType != nullptr);
                VkReflectionInfo::UniformBuffer refl;
                refl.binding = variable->binding;
                refl.set = variable->group;
                refl.name = Util::String(variable->name, variable->nameLength);
                refl.byteSize = variable->structType->size;

                if (variable->binding != 0xFFFFFFFF)
                {
                    n_assert(variable->binding < 64);
                    reflectionInfo.uniformBuffersMask.Resize(Math::max(variable->group + 1, (uint)reflectionInfo.uniformBuffersMask.Size()), 0);
                    reflectionInfo.uniformBuffersMask[variable->group] |= (1ull << (uint64_t)(variable->binding));
                }

                reflectionInfo.uniformBuffers.Append(refl);
                reflectionInfo.uniformBuffersByName.Add(refl.name, refl);
                reflectionInfo.uniformBuffersPerSet.Resize(Math::max(variable->group + 1, (uint)reflectionInfo.uniformBuffersPerSet.Size()), nullptr);
                reflectionInfo.uniformBuffersPerSet[variable->group].Append(refl);
            }
        }
        if (object->type == GPULang::Serialize::Type::ProgramType)
        {
            auto program = (GPULang::Deserialize::Program*)object;

            // allocate new program object and set it up
            Ids::Id32 programId = shaderProgramAlloc.Alloc();
            VkShaderProgramSetup(programId, info.name, program, setupInfo.pipelineLayout);

            // make an ID which is the shader id and program id
            ShaderProgramId shaderProgramId;
            shaderProgramId.shader = ret.id;
            shaderProgramId.program = programId;
            runtimeInfo.programMap.Add(shaderProgramAlloc.Get<ShaderProgram_SetupInfo>(programId).mask, shaderProgramId);
        }
    }

    for (auto& set : reflectionInfo.uniformBuffersPerSet)
    {
        set.SortWithFunc(
            [](const VkReflectionInfo::UniformBuffer& lhs, const VkReflectionInfo::UniformBuffer& rhs) -> bool
            {
                return lhs.binding < rhs.binding;
            }
        );
    }

    delete info.loader;

#if __NEBULA_HTTP__
    //res->debugState = res->CreateState();
#endif
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DeleteShader(const ShaderId id)
{
    VkShaderSetupInfo& setup = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    VkShaderRuntimeInfo& runtime = shaderAlloc.Get<Shader_RuntimeInfo>(id.id);
    ShaderCleanup(setup.dev, setup.immutableSamplers, setup.descriptorSetLayouts, setup.uniformBufferMap, setup.pipelineLayout);

    for (IndexT i = 0; i < runtime.programMap.Size(); i++)
    {
        VkShaderProgramSetupInfo& progSetup = shaderProgramAlloc.Get<ShaderProgram_SetupInfo>(runtime.programMap.ValueAtIndex(i).programId);
        VkShaderProgramRuntimeInfo& progRuntime = shaderProgramAlloc.Get<ShaderProgram_RuntimeInfo>(runtime.programMap.ValueAtIndex(i).programId);
        VkShaderProgramDiscard(progSetup, progRuntime, progRuntime.pipeline);
    }
    runtime.programMap.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyShader(const ShaderId id)
{
    DeleteShader(id);
    shaderAlloc.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderProgramId
ShaderGetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask)
{
    VkShaderRuntimeInfo& runtime = shaderAlloc.Get<Shader_RuntimeInfo>(shaderId.id);
    IndexT i = runtime.programMap.FindIndex(mask);
    if (i == InvalidIndex)  return CoreGraphics::InvalidShaderProgramId;
    else                    return runtime.programMap.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
ShaderCreateResourceTable(const CoreGraphics::ShaderId id, const IndexT group, const uint overallocationSize, const char* name)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    IndexT idx = info.descriptorSetLayoutMap.FindIndex(group);
    if (idx == InvalidIndex) return CoreGraphics::InvalidResourceTableId;
    else
    {
        ResourceTableCreateInfo crInfo =
        {
            .name = name,
            .layout = Util::Get<1>(info.descriptorSetLayouts[info.descriptorSetLayoutMap.ValueAtIndex(idx)]),
            .overallocationSize = overallocationSize
        };
        return CoreGraphics::CreateResourceTable(crInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
ResourceTableSet
ShaderCreateResourceTableSet(const ShaderId id, const IndexT group, const uint overallocationSize, const char* name)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    IndexT idx = info.descriptorSetLayoutMap.FindIndex(group);
    if (idx == InvalidIndex) return CoreGraphics::ResourceTableSet();
    else
    {
        ResourceTableCreateInfo crInfo =
        {
            .name = name,
            .layout = Util::Get<1>(info.descriptorSetLayouts[info.descriptorSetLayoutMap.ValueAtIndex(idx)]),
            .overallocationSize = overallocationSize
        };
        CoreGraphics::ResourceTableSet ret;
        ret.Create(crInfo);
        return ret;
    }
}

//------------------------------------------------------------------------------
/**
*/
const bool
ShaderHasResourceTable(const ShaderId id, const IndexT group)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    return info.descriptorSetLayoutMap.FindIndex(group) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
ShaderCreateConstantBuffer(const CoreGraphics::ShaderId id, const Util::StringAtom& name, CoreGraphics::BufferAccessMode mode)
{
    const auto& uniformBuffers = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffersByName;
    IndexT i = uniformBuffers.FindIndex(name);
    if (i != InvalidIndex)
    {
        const VkReflectionInfo::UniformBuffer& buffer = uniformBuffers.ValueAtIndex(i);
        if (buffer.byteSize == 0)
            return CoreGraphics::InvalidBufferId;

        BufferCreateInfo info;
        info.byteSize = buffer.byteSize;
        info.name = name;
        info.mode = mode;
        info.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
        info.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;

        // Initialize data to zeroes
        Util::FixedArray<byte> data(buffer.byteSize, 0x0);
        info.data = data.Begin();
        info.dataSize = data.Size();

        return CoreGraphics::CreateBuffer(info);
    }
    else
        return CoreGraphics::InvalidBufferId;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
ShaderCreateConstantBuffer(const CoreGraphics::ShaderId id, const IndexT cbIndex, CoreGraphics::BufferAccessMode mode)
{
    const auto& uniformBuffers = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffers;
    const VkReflectionInfo::UniformBuffer& buffer = uniformBuffers[cbIndex];
    if (buffer.byteSize > 0)
    {
        BufferCreateInfo info;
        info.byteSize = buffer.byteSize;
        info.name = buffer.name;
        info.mode = mode;
        info.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;

        // Initialize data to zeroes
        Util::FixedArray<byte> data(buffer.byteSize, 0x0);
        info.data = data.Begin();
        info.dataSize = data.Size();
        return CoreGraphics::CreateBuffer(info);
    }
    else
        return CoreGraphics::InvalidBufferId;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
ShaderCreateConstantBuffer(const ShaderId id, const IndexT group, const IndexT cbIndex, BufferAccessMode mode)
{
    const auto& uniformBuffers = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffersPerSet;
    const auto& buffer = uniformBuffers[group][cbIndex];
    if (buffer.byteSize > 0)
    {
        BufferCreateInfo info;
        info.byteSize = buffer.byteSize;
        info.name = buffer.name;
        info.mode = mode;
        info.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;

        // Initialize data to zeroes
        Util::FixedArray<byte> data(buffer.byteSize, 0x0);
        info.data = data.Begin();
        info.dataSize = data.Size();
        return CoreGraphics::CreateBuffer(info);
    }
    return CoreGraphics::InvalidBufferId;
}

//------------------------------------------------------------------------------
/**
*/
const uint
ShaderCalculateConstantBufferIndex(const uint64_t bindingMask, const IndexT slot)
{
    if ((bindingMask & (1ull << slot)) == 0)
        return 0xFFFFFFFF;
    uint mask = (1 << slot) - 1;
	uint survivingBits = bindingMask & mask;
	return Util::PopCnt(survivingBits);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableLayoutId
ShaderGetResourceTableLayout(const CoreGraphics::ShaderId id, const IndexT group)
{
    const VkShaderSetupInfo& setupInfo = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    uint layout = setupInfo.descriptorSetLayoutMap[group];
    return Util::Get<1>(setupInfo.descriptorSetLayouts[layout]);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourcePipelineId
ShaderGetResourcePipeline(const CoreGraphics::ShaderId id)
{
    return shaderAlloc.Get<Shader_SetupInfo>(id.id).pipelineLayout;
}

//------------------------------------------------------------------------------
/**
*/
const Resources::ResourceName
ShaderGetName(const ShaderId id)
{
    return shaderAlloc.Get<Shader_SetupInfo>(id.id).name;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantCount(const CoreGraphics::ShaderId id)
{
    return shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variables.Size();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantType
ShaderGetConstantType(const CoreGraphics::ShaderId id, const IndexT i)
{
    return ConstantBufferVariableType;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantType
ShaderGetConstantType(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    return ConstantBufferVariableType;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantBlockName(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variablesByName[name];
    return var.blockName;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantBlockName(const CoreGraphics::ShaderId id, const IndexT cIndex)
{
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variables[cIndex];
    return var.blockName;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantName(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variables[i];
    return var.name;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantGroup(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    IndexT idx = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variablesByName.FindIndex(name);
    if (idx != InvalidIndex)
    {
        const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variablesByName.ValueAtIndex(idx);
        return var.blockSet;
    }
    else
        return -1;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    IndexT idx = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variablesByName.FindIndex(name);
    if (idx != InvalidIndex)
    {
        const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variablesByName.ValueAtIndex(idx);
        return var.blockBinding;
    }
    else
        return -1;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantBufferCount(const CoreGraphics::ShaderId id)
{
    return shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffers.Size();
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffers[i];
    return var.byteSize;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffers[i];
    return var.name;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantBufferResourceSlot(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffers[i];
    return var.binding;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantBufferResourceGroup(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffers[i];
    return var.set;
}

//------------------------------------------------------------------------------
/**
*/
const uint64_t
ShaderGetConstantBufferBindingMask(const ShaderId id, const IndexT group)
{
    const auto& masks = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffersMask;
    if (masks.Size() > group)
        return masks[group];
    else
        return 0x0;
}

//------------------------------------------------------------------------------
/**
*/
const uint64_t
ShaderGetConstantBufferSize(const ShaderId id, const IndexT group, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).uniformBuffersPerSet[group][i];
    return var.byteSize;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetResourceSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    IndexT index = info.resourceIndexMap.FindIndex(name);
    if (index == InvalidIndex)  return index;
    else                        return info.resourceIndexMap.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>&
ShaderGetPrograms(const CoreGraphics::ShaderId id)
{
    return shaderAlloc.Get<Shader_RuntimeInfo>(id.id).programMap;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
ShaderProgramGetName(const ShaderProgramId id)
{
    return shaderProgramAlloc.Get<ShaderProgram_SetupInfo>(id.programId).name;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderProgramId
ShaderGetProgram(const ShaderId id, const CoreGraphics::ShaderFeature::Mask mask)
{
    VkShaderRuntimeInfo& runtime = shaderAlloc.Get<Shader_RuntimeInfo>(id.id);
    IndexT i = runtime.programMap.FindIndex(mask);
    if (i == InvalidIndex)  return CoreGraphics::InvalidShaderProgramId;
    else                    return runtime.programMap.ValueAtIndex(i);
}

} // namespace CoreGraphics
