//------------------------------------------------------------------------------
// vkshader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkshader.h"
#include "coregraphics/shaderserver.h"
#include "lowlevel/vk/vksampler.h"
#include "lowlevel/vk/vkvarblock.h"
#include "lowlevel/vk/vkvarbuffer.h"
#include "lowlevel/vk/vkvariable.h"
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
void
ShaderSetup(
    VkDevice dev,
    const Util::StringAtom& name,
    AnyFX::ShaderEffect* effect,
    Util::FixedArray<CoreGraphics::ResourcePipelinePushConstantRange>& constantRange,
    Util::Set<CoreGraphics::SamplerId>& immutableSamplers,
    Util::FixedArray<Util::Pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
    Util::Dictionary<uint32_t, uint32_t>& setLayoutMap,
    CoreGraphics::ResourcePipelineId& pipelineLayout,
    Util::Dictionary<Util::StringAtom, uint32_t>& resourceSlotMapping,
    Util::Dictionary<Util::StringAtom, IndexT>& constantBindings
    )
{
    const std::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks();
    const std::vector<AnyFX::VarbufferBase*>& varbuffers = effect->GetVarbuffers();
    const std::vector<AnyFX::VariableBase*>& variables = effect->GetVariables();
    const std::vector<AnyFX::SamplerBase*>& samplers = effect->GetSamplers();

    using namespace CoreGraphics;

    // construct layout create info
    ResourceTableLayoutCreateInfo layoutInfo;
    Util::Dictionary<uint32_t, ResourceTableLayoutCreateInfo> layoutCreateInfos;
    uint32_t numsets = 0;

    // always create push constant range in layout, making all shaders using push constants compatible
    uint32_t pushRangeOffset = 0; // we must append previous push range size to offset
    constantRange.Resize(NumShaders); // one per shader stage
    uint i;
    for (i = 0; i < NumShaders; i++)
        constantRange[i] = CoreGraphics::ResourcePipelinePushConstantRange{ 0,0,InvalidVisibility };

    // assert we are not over-stepping any uniform buffer limit we are using, perStage is used for ALL_STAGES
    uint32_t numUniformDyn = 0;
    uint32_t numUniform = 0;

    uint32_t numPerStageUniformBuffers[NumShaders] = {0};
    uint32_t numPerStageStorageBuffers[NumShaders] = {0};
    uint32_t numPerStageSamplers[NumShaders] = {0};
    uint32_t numPerStageSampledImages[NumShaders] = {0};
    uint32_t numPerStageStorageImages[NumShaders] = {0};
    uint32_t numPerStageAccelerationStructures[NumShaders] = {0};
    uint32_t numInputAttachments = 0;
    bool bindingTable[128] = { false };
    for (i = 0; i < varblocks.size(); i++)
    {
        AnyFX::VkVarblock* block = static_cast<AnyFX::VkVarblock*>(varblocks[i]);
        resourceSlotMapping.Add(block->name.c_str(), block->binding);
        if (block->variables.empty()) continue;

        if (AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
        {
            CoreGraphics::ResourcePipelinePushConstantRange range;
            range.offset = pushRangeOffset;
            range.size = block->alignedSize;
            range.vis = AllGraphicsVisibility; // only allow for fragment bit...
            constantRange[0] = range; // okay, this is hacky
            pushRangeOffset += block->alignedSize;
            n_assert(CoreGraphics::MaxPushConstantSize >= pushRangeOffset); // test if the next offset would still be in range
            goto skipbuffer; // if push-constant block, do not add to resource table, but add constant bindings!
        }
        {
            ResourceTableLayoutConstantBuffer cbo;
            cbo.slot = block->binding;
            cbo.num = block->bindingLayout.descriptorCount;
            cbo.visibility = AllVisibility;
            cbo.dynamicOffset = false;
            bool slotUsed = false;
            if (block->HasAnnotation("Visibility"))
            {
                cbo.visibility = ShaderVisibilityFromString(block->GetAnnotationString("Visibility").c_str());
            }

            if (block->binding != 0xFFFFFFFF && !bindingTable[block->binding])
            {
                bindingTable[block->binding] = true;
                UpdateOccupancy(numPerStageUniformBuffers, slotUsed, cbo.visibility);
            }
            UpdateValueDescriptorSet(cbo.dynamicOffset, numUniform, numUniformDyn, block->set);

            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(block->set);
            numsets = Math::max(numsets, block->set + 1);
            rinfo.constantBuffers.Append(cbo);

            n_assert(block->alignedSize <= CoreGraphics::MaxConstantBufferSize);
        }
        skipbuffer:

        const std::vector<AnyFX::VariableBase*>& vars = block->variables;
        uint j;
        for (j = 0; j < vars.size(); j++)
        {
            const AnyFX::VariableBase* var = vars[j];
#if NEBULA_DEBUG
            n_assert(!constantBindings.Contains(var->name.c_str()));
#endif
            constantBindings.Add(var->name.c_str(), { (IndexT)block->offsetsByName[var->name] });
        }
    }
    n_assert(CoreGraphics::MaxResourceTableDynamicOffsetConstantBuffers >= numUniformDyn);
    n_assert(CoreGraphics::MaxResourceTableConstantBuffers >= numUniform);
        for (uint i = 0; i < NumShaders; i++)
        n_assert(CoreGraphics::MaxPerStageConstantBuffers >= numPerStageUniformBuffers[i]);

    // do the same for storage buffers
    uint32_t numStorageDyn = 0;
    uint32_t numStorage = 0;
    memset(bindingTable, 0x0, sizeof(bindingTable));
    for (i = 0; i < varbuffers.size(); i++)
    {
        AnyFX::VkVarbuffer* buffer = static_cast<AnyFX::VkVarbuffer*>(varbuffers[i]);
        resourceSlotMapping.Add(buffer->name.c_str(), buffer->binding);
        if (buffer->alignedSize == 0) continue;

        VkDescriptorSetLayoutBinding& binding = buffer->bindingLayout;
        ResourceTableLayoutShaderRWBuffer rwbo;
        rwbo.slot = buffer->binding;
        rwbo.num = binding.descriptorCount;
        rwbo.visibility = AllVisibility;
        rwbo.dynamicOffset = false;
        bool slotUsed = false;

        if (buffer->HasAnnotation("Visibility"))
        {
            rwbo.visibility = ShaderVisibilityFromString(buffer->GetAnnotationString("Visibility").c_str());
        }

        if (!bindingTable[buffer->binding])
        {
            bindingTable[buffer->binding] = true;
            UpdateOccupancy(numPerStageStorageBuffers, slotUsed, rwbo.visibility);
        }
        UpdateValueDescriptorSet(rwbo.dynamicOffset, numStorage, numStorageDyn, buffer->set);

        ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(buffer->set);
        numsets = Math::max(numsets, buffer->set + 1);
        rinfo.rwBuffers.Append(rwbo);
    }
    n_assert(CoreGraphics::MaxResourceTableDynamicOffsetReadWriteBuffers >= numStorageDyn);
    n_assert(CoreGraphics::MaxResourceTableReadWriteBuffers >= numStorage);
    for (uint i = 0; i < NumShaders; i++)
        n_assert(CoreGraphics::MaxPerStageReadWriteBuffers >= numPerStageStorageBuffers[i]);

    uint32_t maxTextures = CoreGraphics::MaxResourceTableSampledImages;
    uint32_t remainingTextures = maxTextures;

    // setup samplers as immutable coupled with the texture input, before we setup the variables so that it's part of their layout
    immutableSamplers.Reserve((SizeT)samplers.size() + 1);
    Util::Dictionary<Util::StringAtom, SamplerId> samplerLookup;
    for (i = 0; i < samplers.size(); i++)
    {
        AnyFX::VkSampler* sampler = static_cast<AnyFX::VkSampler*>(samplers[i]);
        if (!sampler->textureVariables.empty())
        {
            // okay, a bit ugly since we actually just need a Vulkan sampler...
            const VkSamplerCreateInfo& inf = sampler->samplerInfo;
            SamplerCreateInfo info = ToNebulaSamplerCreateInfo(inf);
            SamplerId samp = CreateSampler(info);

            // add to list so we can remove it later
            immutableSamplers.Add(samp);

            uint j;
            for (j = 0; j < sampler->textureVariables.size(); j++)
            {
                AnyFX::VkVariable* var = static_cast<AnyFX::VkVariable*>(sampler->textureVariables[j]);
                n_assert(var->type >= AnyFX::Sampler1D && var->type <= AnyFX::SamplerCubeArray);
                n_assert(!samplerLookup.Contains(sampler->textureVariables[j]->name.c_str()));
                samplerLookup.Add(sampler->textureVariables[j]->name.c_str(), samp);
            }
        }
        else
        {
            // okay, a bit ugly since we actually just need a Vulkan sampler...
            const VkSamplerCreateInfo& inf = sampler->samplerInfo;
            SamplerCreateInfo info = ToNebulaSamplerCreateInfo(inf);
            SamplerId samp = CreateSampler(info);

            ResourceTableLayoutSampler smla;
            smla.visibility = AllVisibility;

            // add to list so we can remove it later
            immutableSamplers.Add(samp);

            if (sampler->HasAnnotation("Visibility"))
            {
                smla.visibility = ShaderVisibilityFromString(sampler->GetAnnotationString("Visibility").c_str());
                bool slotUsed = false;
                UpdateOccupancy(numPerStageSamplers, slotUsed, smla.visibility);
            }
            else
            {
                for (IndexT i = 0; i < NumShaders; i++)
                    numPerStageSamplers[i]++;
            }
            smla.slot = sampler->bindingLayout.binding;
            smla.sampler = samp;

            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(sampler->set);
            rinfo.samplers.Append(smla);

            numsets = Math::max(numsets, sampler->set + 1);
        }
    }

    // make sure we don't use too many samplers
    for (uint i = 0; i < NumShaders; i++)
        n_assert(CoreGraphics::MaxPerStageSamplers >= numPerStageSamplers[i]);

    SamplerCreateInfo placeholderSamplerInfo =
    {
        LinearFilter, LinearFilter,
        LinearMipMode,
        RepeatAddressMode, RepeatAddressMode, RepeatAddressMode,
        0,
        false,
        16,
        0,
        NeverCompare,
        -FLT_MAX,
        FLT_MAX,
        FloatOpaqueBlackBorder,
        false
    };
    SamplerId placeholderSampler = CreateSampler(placeholderSamplerInfo);
    immutableSamplers.Add(placeholderSampler);

    // setup variables
    for (i = 0; i < variables.size(); i++)
    {
        AnyFX::VkVariable* variable = static_cast<AnyFX::VkVariable*>(variables[i]);

        // handle samplers, images and textures
        if (variable->type >= AnyFX::Sampler1D && variable->type <= AnyFX::TextureCubeArray)
        {
            // add to mapping
            resourceSlotMapping.Add(variable->name.c_str(), variable->binding);

            ResourceTableLayoutTexture tex;
            tex.slot = variable->bindingLayout.binding;
            tex.num = variable->bindingLayout.descriptorCount;
            tex.immutableSampler = InvalidSamplerId;
            tex.visibility = AllVisibility;

            bool storageImage = variable->bindingLayout.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

            if (variable->HasAnnotation("Visibility"))
            {
                tex.visibility = ShaderVisibilityFromString(variable->GetAnnotationString("Visibility").c_str());
                bool slotUsed = false;
                UpdateOccupancy(storageImage ? numPerStageStorageImages : numPerStageSampledImages, slotUsed, tex.visibility);
            }

            if (remainingTextures < (uint32_t)variable->arraySizes[0])
                n_error("Too many textures in shader!");
            else
            {
                remainingTextures -= variable->arraySizes[0];
            }
            if (variable->bindingLayout.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            {
                // if we have a combined image sampler, we must find a sampler
                IndexT i = samplerLookup.FindIndex(variable->name.c_str());
                if (i != InvalidIndex)
                    tex.immutableSampler = samplerLookup.ValueAtIndex(i);
                else
                    tex.immutableSampler = placeholderSampler;
            }

            ResourceTableLayoutCreateInfo& info = layoutCreateInfos.Emplace(variable->set);

            // if storage, store in rwTextures, otherwise use ordinary textures and figure out if we are to use sampler
            if (storageImage) info.rwTextures.Append(tex);
            else              info.textures.Append(tex);

            numsets = Math::max(numsets, variable->set + 1);
        }
        else if (variable->type >= AnyFX::InputAttachment && variable->type <= AnyFX::InputAttachmentUIntegerMS)
        {
            resourceSlotMapping.Add(variable->name.c_str(), variable->binding);
            ResourceTableLayoutInputAttachment ia;
            ia.slot = variable->bindingLayout.binding;
            ia.num = variable->bindingLayout.descriptorCount;
            numInputAttachments += variable->bindingLayout.descriptorCount;
            ia.visibility = PixelShaderVisibility;              // only visible from pixel shaders anyways...

            ResourceTableLayoutCreateInfo& info = layoutCreateInfos.Emplace(variable->set);
            info.inputAttachments.Append(ia);

            numsets = Math::max(numsets, variable->set + 1);
        }
        else if (CoreGraphics::RayTracingSupported && variable->type == AnyFX::AccelerationStructure)
        {
            resourceSlotMapping.Add(variable->name.c_str(), variable->binding);

            ResourceTableLayoutAccelerationStructure tlas;
            tlas.slot = variable->bindingLayout.binding;
            tlas.num = variable->bindingLayout.descriptorCount;
            tlas.visibility = AllVisibility;

            if (variable->HasAnnotation("Visibility"))
                tlas.visibility = ShaderVisibilityFromString(variable->GetAnnotationString("Visibility").c_str());



            ResourceTableLayoutCreateInfo& info = layoutCreateInfos.Emplace(variable->set);
            info.accelerationStructures.Append(tlas);

            numsets = Math::max(numsets, variable->set + 1);
        }
    }
    for (uint i = 0; i < NumShaders; i++)
        n_assert(CoreGraphics::MaxPerStageReadWriteImages >= numPerStageStorageImages[i]);

    for (uint i = 0; i < NumShaders; i++)
        n_assert(CoreGraphics::MaxPerStageSampledImages >= numPerStageSampledImages[i]);

    n_assert(numInputAttachments <= CoreGraphics::MaxResourceTableInputAttachments);

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
            sampBinding.visibility = ShaderVisibilityFromGPULang(sampler->visibility) | annotationBits;
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
            cbo.visibility = ShaderVisibilityFromGPULang(variable->visibility) | annotationBits;
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
            range.vis = ShaderVisibilityFromGPULang(variable->visibility) | annotationBits;
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
            rwbo.visibility = ShaderVisibilityFromGPULang(variable->visibility) | annotationBits;
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
            samp.visibility = ShaderVisibilityFromGPULang(variable->visibility) | annotationBits;
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
            tex.visibility = ShaderVisibilityFromGPULang(variable->visibility) | annotationBits;
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
            tex.visibility = ShaderVisibilityFromGPULang(variable->visibility) | annotationBits;
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
            bvh.visibility = ShaderVisibilityFromGPULang(variable->visibility) | annotationBits;
            bvh.num = variable->arraySizes[0];
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
CreateShader(const ShaderCreateInfo& info)
{
    // load effect from memory
    AnyFX::ShaderEffect* effect = info.effect;

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
        effect,
        setupInfo.constantRangeLayout,
        setupInfo.immutableSamplers,
        setupInfo.descriptorSetLayouts,
        setupInfo.descriptorSetLayoutMap,
        setupInfo.pipelineLayout,
        setupInfo.resourceIndexMap,
        setupInfo.constantBindings
    );

    // setup variables
    const std::vector<AnyFX::VariableBase*> variables = effect->GetVariables();
    for (size_t i = 0; i < variables.size(); i++)
    {
        AnyFX::VariableBase* var = variables[i];
        VkReflectionInfo::Variable refl;
        refl.name = var->name.c_str();
        refl.blockBinding = -1;
        refl.blockSet = -1;
        refl.type = var->type;
        if (var->parentBlock)
        {
            refl.blockName = var->parentBlock->name.c_str();
            refl.blockBinding = var->parentBlock->binding;
            refl.blockSet = var->parentBlock->set;
        }

        reflectionInfo.variables.Append(refl);
        reflectionInfo.variablesByName.Add(refl.name, refl);
    }

    // setup varblocks (uniform buffers)
    const std::vector<AnyFX::VarblockBase*> varblocks = effect->GetVarblocks();
    for (size_t i = 0; i < varblocks.size(); i++)
    {
        AnyFX::VarblockBase* var = varblocks[i];
        VkReflectionInfo::UniformBuffer refl;
        refl.name = var->name.c_str();
        refl.binding = var->binding;
        refl.set = var->set;
        refl.byteSize = var->alignedSize;

        if (var->binding != 0xFFFFFFFF)
        {
            n_assert(var->binding < 64);
            reflectionInfo.uniformBuffersMask.Resize(Math::max(var->set + 1, (uint)reflectionInfo.uniformBuffersMask.Size()), 0);
            reflectionInfo.uniformBuffersMask[var->set] |= (1ull << (uint64_t)var->binding);
        }

        reflectionInfo.uniformBuffers.Append(refl);
        reflectionInfo.uniformBuffersByName.Add(refl.name, refl);
        reflectionInfo.uniformBuffersPerSet.Resize(Math::max(var->set + 1, (uint)reflectionInfo.uniformBuffersPerSet.Size()), nullptr);
        reflectionInfo.uniformBuffersPerSet[var->set].Append(refl);
    }

    // Sort uniform buffers by binding
    for (auto& set : reflectionInfo.uniformBuffersPerSet)
    {
        set.SortWithFunc(
            [](const VkReflectionInfo::UniformBuffer& lhs, const VkReflectionInfo::UniformBuffer& rhs) -> bool
            {
                return lhs.binding < rhs.binding;
            }
        );
    }

    // setup shader variations
    const std::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
    if (!programs.empty())
    {
        for (size_t i = 0; i < programs.size(); i++)
        {
            // get program object from shader subsystem
            AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);

            // allocate new program object and set it up
            Ids::Id32 programId = shaderProgramAlloc.Alloc();
            VkShaderProgramSetup(programId, info.name, program, setupInfo.pipelineLayout);

            // make an ID which is the shader id and program id
            ShaderProgramId shaderProgramId;
            shaderProgramId.shader = ret.id;
            shaderProgramId.program = programId;
            runtimeInfo.programMap.Add(shaderProgramAlloc.Get<ShaderProgram_SetupInfo>(programId).mask, shaderProgramId);
        }

        // set active variation
        runtimeInfo.activeMask = runtimeInfo.programMap.KeyAtIndex(0);
        runtimeInfo.activeShaderProgram = runtimeInfo.programMap.ValueAtIndex(0);
    }

    // delete the AnyFX effect
    delete effect;

#if __NEBULA_HTTP__
    //res->debugState = res->CreateState();
#endif
    return ret;
}


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

    /*
    // setup variables
    const std::vector<AnyFX::VariableBase*> variables = effect->GetVariables();
    for (size_t i = 0; i < variables.size(); i++)
    {
        AnyFX::VariableBase* var = variables[i];
        VkReflectionInfo::Variable refl;
        refl.name = var->name.c_str();
        refl.blockBinding = -1;
        refl.blockSet = -1;
        refl.type = var->type;
        if (var->parentBlock)
        {
            refl.blockName = var->parentBlock->name.c_str();
            refl.blockBinding = var->parentBlock->binding;
            refl.blockSet = var->parentBlock->set;
        }

        reflectionInfo.variables.Append(refl);
        reflectionInfo.variablesByName.Add(refl.name, refl);
    }

    // setup varblocks (uniform buffers)
    const std::vector<AnyFX::VarblockBase*> varblocks = effect->GetVarblocks();
    for (size_t i = 0; i < varblocks.size(); i++)
    {
        AnyFX::VarblockBase* var = varblocks[i];
        VkReflectionInfo::UniformBuffer refl;
        refl.name = var->name.c_str();
        refl.binding = var->binding;
        refl.set = var->set;
        refl.byteSize = var->alignedSize;

        if (var->binding != 0xFFFFFFFF)
        {
            n_assert(var->binding < 64);
            reflectionInfo.uniformBuffersMask.Resize(Math::max(var->set + 1, (uint)reflectionInfo.uniformBuffersMask.Size()), 0);
            reflectionInfo.uniformBuffersMask[var->set] |= (1ull << (uint64)var->binding);
        }

        reflectionInfo.uniformBuffers.Append(refl);
        reflectionInfo.uniformBuffersByName.Add(refl.name, refl);
        reflectionInfo.uniformBuffersPerSet.Resize(Math::max(var->set + 1, (uint)reflectionInfo.uniformBuffersPerSet.Size()), nullptr);
        reflectionInfo.uniformBuffersPerSet[var->set].Append(refl);
    }

    // Sort uniform buffers by binding
    for (auto& set : reflectionInfo.uniformBuffersPerSet)
    {
        set.SortWithFunc(
            [](const VkReflectionInfo::UniformBuffer& lhs, const VkReflectionInfo::UniformBuffer& rhs) -> bool
            {
                return lhs.binding < rhs.binding;
            }
        );
    }
    */
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

    // set active variation
    runtimeInfo.activeMask = runtimeInfo.programMap.KeyAtIndex(0);
    runtimeInfo.activeShaderProgram = runtimeInfo.programMap.ValueAtIndex(0);

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
void
ReloadShader(const ShaderId id, const AnyFX::ShaderEffect* effect)
{
    DeleteShader(id);
    VkReflectionInfo& reflectionInfo = shaderAlloc.Get<Shader_ReflectionInfo>(id.id);
    VkShaderSetupInfo& setupInfo = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    VkShaderRuntimeInfo& runtimeInfo = shaderAlloc.Get<Shader_RuntimeInfo>(id.id);

    reflectionInfo.uniformBuffers.Clear();
    reflectionInfo.uniformBuffersByName.Clear();
    reflectionInfo.uniformBuffersPerSet.Clear();
    reflectionInfo.variables.Clear();
    reflectionInfo.variablesByName.Clear();

    // setup variables
    const std::vector<AnyFX::VariableBase*> variables = effect->GetVariables();
    for (size_t i = 0; i < variables.size(); i++)
    {
        AnyFX::VariableBase* var = variables[i];
        VkReflectionInfo::Variable refl;
        refl.name = var->name.c_str();
        refl.blockBinding = var->binding;
        refl.blockSet = var->set;
        refl.type = var->type;
        if (var->parentBlock)
            refl.blockName = var->parentBlock->name.c_str();

        reflectionInfo.variables.Append(refl);
        reflectionInfo.variablesByName.Add(refl.name, refl);
    }

    // setup varblocks (uniform buffers)
    const std::vector<AnyFX::VarblockBase*> varblocks = effect->GetVarblocks();
    for (size_t i = 0; i < varblocks.size(); i++)
    {
        AnyFX::VarblockBase* var = varblocks[i];
        VkReflectionInfo::UniformBuffer refl;
        refl.name = var->name.c_str();
        refl.binding = var->binding;
        refl.set = var->set;
        refl.byteSize = var->alignedSize;

        reflectionInfo.uniformBuffersPerSet.Resize(var->set + 1, nullptr);
        reflectionInfo.uniformBuffersPerSet[var->set].Append(refl);
        reflectionInfo.uniformBuffers.Append(refl);
        reflectionInfo.uniformBuffersByName.Add(refl.name, refl);
    }

    // Sort uniform buffers by binding
    for (auto& set : reflectionInfo.uniformBuffersPerSet)
    {
        set.SortWithFunc(
            [](const VkReflectionInfo::UniformBuffer& lhs, const VkReflectionInfo::UniformBuffer& rhs) -> bool
            {
                return lhs.binding < rhs.binding;
            }
        );
    }

    // setup shader variations from existing programs
    const std::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
    for (IndexT i = 0; i < programs.size(); i++)
    {
        // get program object from shader subsystem
        AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);
        CoreGraphics::ShaderFeature::Mask mask = CoreGraphics::ShaderFeatureMask(program->GetAnnotationString("Mask").c_str());

        const ShaderProgramId& shaderProgramId = runtimeInfo.programMap[mask];

        // allocate new program object and set it up
        VkShaderProgramSetup(shaderProgramId.programId, setupInfo.name, program, setupInfo.pipelineLayout);

        // trigger a reload in the graphics device
        CoreGraphics::ReloadShaderProgram(shaderProgramId);
    }
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
        return CoreGraphics::ResourceTableSet(crInfo);
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
        info.usageFlags = CoreGraphics::ConstantBuffer;

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
        info.usageFlags = CoreGraphics::ConstantBuffer;

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
        info.usageFlags = CoreGraphics::ConstantBuffer;

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
const IndexT
ShaderGetConstantBinding(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    IndexT index = info.constantBindings.FindIndex(name.Value());
    if (index == InvalidIndex)  return INT32_MAX; // invalid binding
    else                        return info.constantBindings.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantBinding(const CoreGraphics::ShaderId id, const IndexT cIndex)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    return info.constantBindings.ValueAtIndex(cIndex);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantBindingsCount(const CoreGraphics::ShaderId id)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.id);
    return info.constantBindings.Size();
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
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variables[i];
    switch (var.type)
    {
        case AnyFX::Double:
        case AnyFX::Float:
            return FloatVariableType;
        case AnyFX::Short:
        case AnyFX::Integer:
        case AnyFX::UInteger:
            return IntVariableType;
        case AnyFX::Bool:
            return BoolVariableType;
        case AnyFX::Float3:
        case AnyFX::Float4:
        case AnyFX::Double3:
        case AnyFX::Double4:
        case AnyFX::Integer3:
        case AnyFX::Integer4:
        case AnyFX::UInteger3:
        case AnyFX::UInteger4:
        case AnyFX::Short3:
        case AnyFX::Short4:
        case AnyFX::Bool3:
        case AnyFX::Bool4:
            return VectorVariableType;
        case AnyFX::Float2:
        case AnyFX::Double2:
        case AnyFX::Integer2:
        case AnyFX::UInteger2:
        case AnyFX::Short2:
        case AnyFX::Bool2:
            return Vector2VariableType;
        case AnyFX::Matrix2x2:
        case AnyFX::Matrix2x3:
        case AnyFX::Matrix2x4:
        case AnyFX::Matrix3x2:
        case AnyFX::Matrix3x3:
        case AnyFX::Matrix3x4:
        case AnyFX::Matrix4x2:
        case AnyFX::Matrix4x3:
        case AnyFX::Matrix4x4:
            return MatrixVariableType;
            break;
        case AnyFX::Image1D:
        case AnyFX::Image1DArray:
        case AnyFX::Image2D:
        case AnyFX::Image2DArray:
        case AnyFX::Image2DMS:
        case AnyFX::Image2DMSArray:
        case AnyFX::Image3D:
        case AnyFX::ImageCube:
        case AnyFX::ImageCubeArray:
            return ImageReadWriteVariableType;
        case AnyFX::Sampler1D:
        case AnyFX::Sampler1DArray:
        case AnyFX::Sampler2D:
        case AnyFX::Sampler2DArray:
        case AnyFX::Sampler2DMS:
        case AnyFX::Sampler2DMSArray:
        case AnyFX::Sampler3D:
        case AnyFX::SamplerCube:
        case AnyFX::SamplerCubeArray:
            return SamplerVariableType;
        case AnyFX::Texture1D:
        case AnyFX::Texture1DArray:
        case AnyFX::Texture2D:
        case AnyFX::Texture2DArray:
        case AnyFX::Texture2DMS:
        case AnyFX::Texture2DMSArray:
        case AnyFX::Texture3D:
        case AnyFX::TextureCube:
        case AnyFX::TextureCubeArray:
            return TextureVariableType;
        case AnyFX::TextureHandle:
            return TextureVariableType;
        case AnyFX::ImageHandle:
            return ImageReadWriteVariableType;
            break;
        case AnyFX::SamplerHandle:
            return SamplerVariableType;
        default:
            return ConstantBufferVariableType;
    }
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantType
ShaderGetConstantType(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.id).variablesByName[name];
    switch (var.type)
    {
        case AnyFX::Double:
        case AnyFX::Float:
            return FloatVariableType;
        case AnyFX::Short:
        case AnyFX::Integer:
        case AnyFX::UInteger:
            return IntVariableType;
        case AnyFX::Bool:
            return BoolVariableType;
        case AnyFX::Float3:
        case AnyFX::Float4:
        case AnyFX::Double3:
        case AnyFX::Double4:
        case AnyFX::Integer3:
        case AnyFX::Integer4:
        case AnyFX::UInteger3:
        case AnyFX::UInteger4:
        case AnyFX::Short3:
        case AnyFX::Short4:
        case AnyFX::Bool3:
        case AnyFX::Bool4:
            return VectorVariableType;
        case AnyFX::Float2:
        case AnyFX::Double2:
        case AnyFX::Integer2:
        case AnyFX::UInteger2:
        case AnyFX::Short2:
        case AnyFX::Bool2:
            return Vector2VariableType;
        case AnyFX::Matrix2x2:
        case AnyFX::Matrix2x3:
        case AnyFX::Matrix2x4:
        case AnyFX::Matrix3x2:
        case AnyFX::Matrix3x3:
        case AnyFX::Matrix3x4:
        case AnyFX::Matrix4x2:
        case AnyFX::Matrix4x3:
        case AnyFX::Matrix4x4:
            return MatrixVariableType;
            break;
        case AnyFX::Image1D:
        case AnyFX::Image1DArray:
        case AnyFX::Image2D:
        case AnyFX::Image2DArray:
        case AnyFX::Image2DMS:
        case AnyFX::Image2DMSArray:
        case AnyFX::Image3D:
        case AnyFX::ImageCube:
        case AnyFX::ImageCubeArray:
            return ImageReadWriteVariableType;
        case AnyFX::Sampler1D:
        case AnyFX::Sampler1DArray:
        case AnyFX::Sampler2D:
        case AnyFX::Sampler2DArray:
        case AnyFX::Sampler2DMS:
        case AnyFX::Sampler2DMSArray:
        case AnyFX::Sampler3D:
        case AnyFX::SamplerCube:
        case AnyFX::SamplerCubeArray:
            return SamplerVariableType;
        case AnyFX::Texture1D:
        case AnyFX::Texture1DArray:
        case AnyFX::Texture2D:
        case AnyFX::Texture2DArray:
        case AnyFX::Texture2DMS:
        case AnyFX::Texture2DMSArray:
        case AnyFX::Texture3D:
        case AnyFX::TextureCube:
        case AnyFX::TextureCubeArray:
            return TextureVariableType;
        case AnyFX::TextureHandle:
            return TextureHandleType;
        case AnyFX::ImageHandle:
            return ImageHandleType;
            break;
        case AnyFX::SamplerHandle:
            return SamplerHandleType;
        default:
            return ConstantBufferVariableType;
    }
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
Util::StringAtom
ShaderGetProgramName(CoreGraphics::ShaderProgramId id)
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
