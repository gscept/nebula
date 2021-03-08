//------------------------------------------------------------------------------
// vkshader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshader.h"
#include "coregraphics/shaderserver.h"
#include "lowlevel/vk/vksampler.h"
#include "lowlevel/vk/vkvarblock.h"
#include "lowlevel/vk/vkvarbuffer.h"
#include "lowlevel/vk/vkvariable.h"
#include "coregraphics/sampler.h"
#include "coregraphics/resourcetable.h"
#include "vktypes.h"
#include "vksampler.h"

namespace Vulkan
{


Util::Dictionary<Util::StringAtom, VkDescriptorSetLayout> VkShaderLayoutCache;
Util::Dictionary<Util::StringAtom, VkPipelineLayout> VkShaderPipelineCache;
Util::Dictionary<Util::StringAtom, VkDescriptorSet> VkShaderDescriptorSetCache;


//------------------------------------------------------------------------------
/**
*/
void
VkShaderSetup(
    VkDevice dev,
    const Util::StringAtom& name,
    const VkPhysicalDeviceProperties props,
    AnyFX::ShaderEffect* effect,
    Util::FixedArray<CoreGraphics::ResourcePipelinePushConstantRange>& constantRange,
    Util::Array<CoreGraphics::SamplerId>& immutableSamplers,
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

    enum ShaderStages
    {
        VertexShader,
        HullShader,
        DomainShader,
        GeometryShader,
        PixelShader,
        ComputeShader,

        NumShaders
    };

    // always create push constant range in layout, making all shaders using push constants compatible
    uint32_t maxConstantBytes = props.limits.maxPushConstantsSize;
    uint32_t pushRangeOffset = 0; // we must append previous push range size to offset
    constantRange.Resize(6); // one per shader stage
    uint i;
    for (i = 0; i < NumShaders; i++)
        constantRange[i] = CoreGraphics::ResourcePipelinePushConstantRange{ 0,0,InvalidVisibility };

    bool usePushConstants = false;

#define uint_max(a, b) (a > b ? a : b)

    // assert we are not over-stepping any uniform buffer limit we are using, perStage is used for ALL_STAGES
    uint32_t numUniformDyn = 0;
    uint32_t numUniform = 0;

    uint32_t numPerStageUniformBuffers[NumShaders] = {0};
    uint32_t numPerStageStorageBuffers[NumShaders] = {0};
    uint32_t numPerStageSamplers[NumShaders] = {0};
    uint32_t numPerStageSampledImages[NumShaders] = {0};
    uint32_t numPerStageStorageImages[NumShaders] = {0};
    uint32_t numInputAttachments = 0;
    for (i = 0; i < varblocks.size(); i++) 
    { 
        AnyFX::VkVarblock* block = static_cast<AnyFX::VkVarblock*>(varblocks[i]);
        VkDescriptorSetLayoutBinding& binding = block->bindingLayout;
        ResourceTableLayoutConstantBuffer cbo;
        cbo.slot = binding.binding;
        cbo.num = binding.descriptorCount;
        cbo.visibility = AllVisibility;
        uint32_t slotsUsed = 0;

        if (block->HasAnnotation("Visibility"))
        {
            CoreGraphics::ShaderVisibility vis = ShaderVisibilityFromString(block->GetAnnotationString("Visibility").c_str());
            cbo.visibility = vis;
            if ((vis & VertexShaderVisibility) == VertexShaderVisibility)       { numPerStageUniformBuffers[VertexShader]++; slotsUsed++; }
            if ((vis & HullShaderVisibility) == HullShaderVisibility)           { numPerStageUniformBuffers[HullShader]++; slotsUsed++; }
            if ((vis & DomainShaderVisibility) == DomainShaderVisibility)       { numPerStageUniformBuffers[DomainShader]++; slotsUsed++; }
            if ((vis & GeometryShaderVisibility) == GeometryShaderVisibility)   { numPerStageUniformBuffers[GeometryShader]++; slotsUsed++; }
            if ((vis & PixelShaderVisibility) == PixelShaderVisibility)         { numPerStageUniformBuffers[PixelShader]++; slotsUsed++; }
            if ((vis & ComputeShaderVisibility) == ComputeShaderVisibility)     { numPerStageUniformBuffers[ComputeShader]++; slotsUsed++; }
        }

        if (block->variables.empty()) continue;
        if (AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
        {
            n_assert(block->alignedSize <= maxConstantBytes);
            n_assert(block->alignedSize <= props.limits.maxPushConstantsSize);
            maxConstantBytes -= block->alignedSize;
            CoreGraphics::ResourcePipelinePushConstantRange range;
            range.offset = pushRangeOffset;
            range.size = block->alignedSize;
            range.vis = AllGraphicsVisibility; // only allow for fragment bit...
            constantRange[0] = range; // okay, this is hacky
            pushRangeOffset += block->alignedSize;
            usePushConstants = true;
            goto skipbuffer; // if push-constant block, do not add to resource table, but add constant bindings!
        };
        {
        // add to resource map
        resourceSlotMapping.Add(block->name.c_str(), block->binding);
        ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.AddUnique(block->set);
        numsets = uint_max(numsets, block->set + 1);

        if (block->set == NEBULA_DYNAMIC_OFFSET_GROUP || block->set == NEBULA_INSTANCE_GROUP) { cbo.dynamicOffset = true; numUniformDyn += slotsUsed; }
        else                                            { cbo.dynamicOffset = false; numUniform += slotsUsed; }

        rinfo.constantBuffers.Append(cbo);
        n_assert(block->alignedSize <= props.limits.maxUniformBufferRange);
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
    n_assert(props.limits.maxDescriptorSetUniformBuffersDynamic >= numUniformDyn);
    n_assert(props.limits.maxDescriptorSetUniformBuffers >= numUniform);
    uint32_t maxPerStageUniformBuffers = props.limits.maxPerStageDescriptorUniformBuffers;
    for (uint i = 0; i < NumShaders; i++)
        n_assert(maxPerStageUniformBuffers >= numPerStageUniformBuffers[i]);

    // do the same for storage buffers
    uint32_t numStorageDyn = 0;
    uint32_t numStorage = 0;
    for (i = 0; i < varbuffers.size(); i++)
    {
        AnyFX::VkVarbuffer* buffer = static_cast<AnyFX::VkVarbuffer*>(varbuffers[i]);
        resourceSlotMapping.Add(buffer->name.c_str(), buffer->binding);
        VkDescriptorSetLayoutBinding& binding = buffer->bindingLayout;
        ResourceTableLayoutShaderRWBuffer rwbo;
        rwbo.slot = binding.binding;
        rwbo.num = binding.descriptorCount;
        rwbo.visibility = AllVisibility;
        uint32_t slotsUsed = 0;

        if (buffer->alignedSize == 0) continue;
        ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.AddUnique(buffer->set);
        numsets = uint_max(numsets, buffer->set + 1);

        if (buffer->HasAnnotation("Visibility"))
        {
            CoreGraphics::ShaderVisibility vis = ShaderVisibilityFromString(buffer->GetAnnotationString("Visibility").c_str());
            rwbo.visibility = vis;
            if ((vis & VertexShaderVisibility) == VertexShaderVisibility)       { numPerStageStorageBuffers[VertexShader]++; slotsUsed++; }
            if ((vis & HullShaderVisibility) == HullShaderVisibility)           { numPerStageStorageBuffers[HullShader]++; slotsUsed++; }
            if ((vis & DomainShaderVisibility) == DomainShaderVisibility)       { numPerStageStorageBuffers[DomainShader]++; slotsUsed++; }
            if ((vis & GeometryShaderVisibility) == GeometryShaderVisibility)   { numPerStageStorageBuffers[GeometryShader]++; slotsUsed++; }
            if ((vis & PixelShaderVisibility) == PixelShaderVisibility)         { numPerStageStorageBuffers[PixelShader]++; slotsUsed++; }
            if ((vis & ComputeShaderVisibility) == ComputeShaderVisibility)     { numPerStageStorageBuffers[ComputeShader]++; slotsUsed++; }
        }

        if (buffer->set == NEBULA_DYNAMIC_OFFSET_GROUP || buffer->set == NEBULA_INSTANCE_GROUP) { rwbo.dynamicOffset = true; numStorageDyn += slotsUsed; }
        else                                             { rwbo.dynamicOffset = false; numStorage += slotsUsed; }

        rinfo.rwBuffers.Append(rwbo);
        n_assert(buffer->alignedSize < props.limits.maxStorageBufferRange);
    }
    n_assert(props.limits.maxDescriptorSetStorageBuffersDynamic >= numStorageDyn);
    n_assert(props.limits.maxDescriptorSetStorageBuffers >= numStorage);
    uint32_t maxPerStageStorageBuffers = props.limits.maxPerStageDescriptorStorageBuffers;
    for (uint i = 0; i < NumShaders; i++)
        n_assert(maxPerStageStorageBuffers >= numPerStageStorageBuffers[i]);

    uint32_t maxTextures = props.limits.maxDescriptorSetSampledImages;
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
            immutableSamplers.Append(samp);

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
            immutableSamplers.Append(samp);

            if (sampler->HasAnnotation("Visibility"))
            {
                CoreGraphics::ShaderVisibility vis = ShaderVisibilityFromString(sampler->GetAnnotationString("Visibility").c_str());
                if ((vis & VertexShaderVisibility) == VertexShaderVisibility)       { numPerStageSamplers[VertexShader]++; };
                if ((vis & HullShaderVisibility) == HullShaderVisibility)           { numPerStageSamplers[HullShader]++; };
                if ((vis & DomainShaderVisibility) == DomainShaderVisibility)       { numPerStageSamplers[DomainShader]++; };
                if ((vis & GeometryShaderVisibility) == GeometryShaderVisibility)   { numPerStageSamplers[GeometryShader]++; };
                if ((vis & PixelShaderVisibility) == PixelShaderVisibility)         { numPerStageSamplers[PixelShader]++; };
                if ((vis & ComputeShaderVisibility) == ComputeShaderVisibility)     { numPerStageSamplers[ComputeShader]++; };
                smla.visibility = vis;
            }
            smla.slot = sampler->bindingLayout.binding;
            smla.sampler = samp;

            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.AddUnique(sampler->set);
            rinfo.samplers.Append(smla);

            numsets = uint_max(numsets, sampler->set + 1);
        }
    }

    // make sure we don't use too many samplers
    uint32_t maxPerStageSamplers = props.limits.maxPerStageDescriptorSamplers;
    for (uint i = 0; i < NumShaders; i++)
        n_assert(maxPerStageSamplers >= numPerStageSamplers[i]);

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
    immutableSamplers.Append(placeholderSampler);

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
                CoreGraphics::ShaderVisibility vis = ShaderVisibilityFromString(variable->GetAnnotationString("Visibility").c_str());
                if ((vis & VertexShaderVisibility) == VertexShaderVisibility)       { storageImage ? numPerStageStorageImages[VertexShader]++	: numPerStageSampledImages[VertexShader]++; }
                if ((vis & HullShaderVisibility) == HullShaderVisibility)           { storageImage ? numPerStageStorageImages[HullShader]++		: numPerStageSampledImages[HullShader]++; }
                if ((vis & DomainShaderVisibility) == DomainShaderVisibility)       { storageImage ? numPerStageStorageImages[DomainShader]++	: numPerStageSampledImages[DomainShader]++; }
                if ((vis & GeometryShaderVisibility) == GeometryShaderVisibility)   { storageImage ? numPerStageStorageImages[GeometryShader]++ : numPerStageSampledImages[GeometryShader]++; }
                if ((vis & PixelShaderVisibility) == PixelShaderVisibility)         { storageImage ? numPerStageStorageImages[PixelShader]++	: numPerStageSampledImages[PixelShader]++; }
                if ((vis & ComputeShaderVisibility) == ComputeShaderVisibility)     { storageImage ? numPerStageStorageImages[ComputeShader]++	: numPerStageSampledImages[ComputeShader]++; }
                tex.visibility = vis;
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

            ResourceTableLayoutCreateInfo& info = layoutCreateInfos.AddUnique(variable->set);

            // if storage, store in rwTextures, otherwise use ordinary textures and figure out if we are to use sampler
            if (storageImage) info.rwTextures.Append(tex);
            else              info.textures.Append(tex);

            numsets = uint_max(numsets, variable->set + 1);
        }
        else if (variable->type >= AnyFX::InputAttachment && variable->type <= AnyFX::InputAttachmentUIntegerMS)
        {
            resourceSlotMapping.Add(variable->name.c_str(), variable->binding);
            ResourceTableLayoutInputAttachment ia;
            ia.slot = variable->bindingLayout.binding;
            ia.num = variable->bindingLayout.descriptorCount;
            numInputAttachments += variable->bindingLayout.descriptorCount;
            ia.visibility = PixelShaderVisibility;              // only visible from pixel shaders anyways...

            ResourceTableLayoutCreateInfo& info = layoutCreateInfos.AddUnique(variable->set);
            info.inputAttachments.Append(ia);

            numsets = uint_max(numsets, variable->set + 1);
        }
    }
    uint32_t maxPerStageStorageImages = props.limits.maxPerStageDescriptorStorageImages;
    for (uint i = 0; i < NumShaders; i++)
        n_assert(maxPerStageStorageImages >= numPerStageStorageImages[i]);

    uint32_t maxPerStageSampledImages = props.limits.maxPerStageDescriptorSampledImages;
    for (uint i = 0; i < NumShaders; i++)
        n_assert(maxPerStageSampledImages >= numPerStageSampledImages[i]);

    n_assert(numInputAttachments <= props.limits.maxDescriptorSetInputAttachments);

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
VkShaderCleanup(
    VkDevice dev,
    Util::Array<CoreGraphics::SamplerId>& immutableSamplers,
    Util::FixedArray<Util::Pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
    Util::Dictionary<Util::StringAtom, CoreGraphics::BufferId>& buffers,
    CoreGraphics::ResourcePipelineId& pipelineLayout
)
{
    IndexT i;
    for (i = 0; i < immutableSamplers.Size(); i++)
    {
        CoreGraphics::DestroySampler(immutableSamplers[i]);
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

} // namespace Vulkan
