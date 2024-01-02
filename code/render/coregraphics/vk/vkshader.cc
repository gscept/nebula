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
UpdateOccupancy(uint32_t* occupancyList, uint32_t& slotsUsed, const CoreGraphics::ShaderVisibility vis)
{
    if (AllBits(vis, CoreGraphics::VertexShaderVisibility))             { occupancyList[VertexShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::HullShaderVisibility))               { occupancyList[HullShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::DomainShaderVisibility))             { occupancyList[DomainShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::GeometryShaderVisibility))           { occupancyList[GeometryShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::PixelShaderVisibility))              { occupancyList[PixelShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::TaskShaderVisibility))               { occupancyList[TaskShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::MeshShaderVisibility))               { occupancyList[MeshShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::ComputeShaderVisibility))            { occupancyList[ComputeShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::RayGenerationShaderVisibility))      { occupancyList[RayGenerationShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::RayAnyHitShaderVisibility))          { occupancyList[RayAnyHitShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::RayClosestHitShaderVisibility))      { occupancyList[RayClosestHitShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::RayMissShaderVisibility))            { occupancyList[RayMissShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::RayIntersectionShaderVisibility))    { occupancyList[RayIntersectionShader]++; slotsUsed++; }
    if (AllBits(vis, CoreGraphics::CallableShaderVisibility))           { occupancyList[CallableShader]++; slotsUsed++; }
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderSetup(
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

    // always create push constant range in layout, making all shaders using push constants compatible
    uint32_t maxConstantBytes = props.limits.maxPushConstantsSize;
    uint32_t pushRangeOffset = 0; // we must append previous push range size to offset
    constantRange.Resize(NumShaders); // one per shader stage
    uint i;
    for (i = 0; i < NumShaders; i++)
        constantRange[i] = CoreGraphics::ResourcePipelinePushConstantRange{ 0,0,InvalidVisibility };

    bool usePushConstants = false;

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
            cbo.visibility = ShaderVisibilityFromString(block->GetAnnotationString("Visibility").c_str());
            UpdateOccupancy(numPerStageUniformBuffers, slotsUsed, cbo.visibility);
        }
        else
        {
            for (IndexT i = 0; i < NumShaders; i++)
                numPerStageUniformBuffers[i]++;
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
        }
        {
            // add to resource map
            resourceSlotMapping.Add(block->name.c_str(), block->binding);
            ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(block->set);
            numsets = Math::max(numsets, block->set + 1);

            if (block->set == NEBULA_DYNAMIC_OFFSET_GROUP || block->set == NEBULA_INSTANCE_GROUP)
            {
                cbo.dynamicOffset = true; numUniformDyn += slotsUsed;
            }
            else
            {
                cbo.dynamicOffset = false; numUniform += slotsUsed;
            }

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
        ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.Emplace(buffer->set);
        numsets = Math::max(numsets, buffer->set + 1);

        if (buffer->HasAnnotation("Visibility"))
        {
            rwbo.visibility = ShaderVisibilityFromString(buffer->GetAnnotationString("Visibility").c_str());
            UpdateOccupancy(numPerStageStorageBuffers, slotsUsed, rwbo.visibility);
        }
        else
        {
            for (IndexT i = 0; i < NumShaders; i++)
                numPerStageStorageBuffers[i]++;
        }

        if (buffer->set == NEBULA_DYNAMIC_OFFSET_GROUP || buffer->set == NEBULA_INSTANCE_GROUP)
        {
            rwbo.dynamicOffset = true; numStorageDyn += slotsUsed;
        }
        else
        {
            rwbo.dynamicOffset = false; numStorage += slotsUsed;
        }

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
                smla.visibility = ShaderVisibilityFromString(sampler->GetAnnotationString("Visibility").c_str());
                uint32_t dummy;
                UpdateOccupancy(numPerStageSamplers, dummy, smla.visibility);
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
                tex.visibility = ShaderVisibilityFromString(variable->GetAnnotationString("Visibility").c_str());
                uint32_t dummy = 0;
                UpdateOccupancy(storageImage ? numPerStageStorageImages : numPerStageSampledImages, dummy, tex.visibility);
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

    uint32_t id = shaderAlloc.Alloc();
    VkReflectionInfo& reflectionInfo = shaderAlloc.Get<Shader_ReflectionInfo>(id);
    VkShaderSetupInfo& setupInfo = shaderAlloc.Get<Shader_SetupInfo>(id);
    VkShaderRuntimeInfo& runtimeInfo = shaderAlloc.Get<Shader_RuntimeInfo>(id);

    ShaderId ret;
    ret.resourceId = id;
    ret.resourceType = CoreGraphics::ShaderIdType;

    setupInfo.id = ShaderIdentifier::FromName(info.name);
    setupInfo.name = info.name;
    setupInfo.dev = Vulkan::GetCurrentDevice();

    // the setup code is massive, so just let it be in VkShader...
    ShaderSetup(
        setupInfo.dev,
        info.name,
        Vulkan::GetCurrentProperties(),
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

        reflectionInfo.uniformBuffers.Append(refl);
        reflectionInfo.uniformBuffersByName.Add(refl.name, refl);
    }

    // setup shader variations
    const std::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
    for (size_t i = 0; i < programs.size(); i++)
    {
        // get program object from shader subsystem
        AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);

        // allocate new program object and set it up
        Ids::Id32 programId = shaderProgramAlloc.Alloc();
        VkShaderProgramSetup(programId, info.name, program, setupInfo.pipelineLayout);

        // make an ID which is the shader id and program id
        ShaderProgramId shaderProgramId;
        shaderProgramId.programId = programId;
        shaderProgramId.shaderId = ret.resourceId;
        shaderProgramId.shaderType = ret.resourceType;
        runtimeInfo.programMap.Add(shaderProgramAlloc.Get<ShaderProgram_SetupInfo>(programId).mask, shaderProgramId);
    }

    // delete the AnyFX effect
    delete effect;

    // set active variation
    runtimeInfo.activeMask = runtimeInfo.programMap.KeyAtIndex(0);
    runtimeInfo.activeShaderProgram = runtimeInfo.programMap.ValueAtIndex(0);

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
    VkShaderSetupInfo& setup = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    VkShaderRuntimeInfo& runtime = shaderAlloc.Get<Shader_RuntimeInfo>(id.resourceId);
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
    shaderAlloc.Dealloc(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
void
ReloadShader(const ShaderId id, const AnyFX::ShaderEffect* effect)
{
    DeleteShader(id);
    VkReflectionInfo& reflectionInfo = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId);
    VkShaderSetupInfo& setupInfo = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    VkShaderRuntimeInfo& runtimeInfo = shaderAlloc.Get<Shader_RuntimeInfo>(id.resourceId);

    reflectionInfo.uniformBuffers.Clear();
    reflectionInfo.uniformBuffersByName.Clear();
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

        reflectionInfo.uniformBuffers.Append(refl);
        reflectionInfo.uniformBuffersByName.Add(refl.name, refl);
    }

    // setup shader variations from existing programs
    const std::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
    for (IndexT i = 0; i < programs.size(); i++)
    {
        // get program object from shader subsystem
        AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);
        CoreGraphics::ShaderFeature::Mask mask = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask(program->GetAnnotationString("Mask").c_str());

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
    VkShaderRuntimeInfo& runtime = shaderAlloc.Get<Shader_RuntimeInfo>(shaderId.resourceId);
    IndexT i = runtime.programMap.FindIndex(mask);
    if (i == InvalidIndex)  return CoreGraphics::InvalidShaderProgramId;
    else                    return runtime.programMap.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
ShaderCreateResourceTable(const CoreGraphics::ShaderId id, const IndexT group, const uint overallocationSize)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    IndexT idx = info.descriptorSetLayoutMap.FindIndex(group);
    if (idx == InvalidIndex) return CoreGraphics::InvalidResourceTableId;
    else
    {
        ResourceTableCreateInfo crInfo =
        {
            Util::Get<1>(info.descriptorSetLayouts[info.descriptorSetLayoutMap.ValueAtIndex(idx)]),
            overallocationSize
        };
        return CoreGraphics::CreateResourceTable(crInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
ResourceTableSet
ShaderCreateResourceTableSet(const ShaderId id, const IndexT group, const uint overallocationSize)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    IndexT idx = info.descriptorSetLayoutMap.FindIndex(group);
    if (idx == InvalidIndex) return std::move(CoreGraphics::ResourceTableSet());
    else
    {
        ResourceTableCreateInfo crInfo =
        {
            Util::Get<1>(info.descriptorSetLayouts[info.descriptorSetLayoutMap.ValueAtIndex(idx)]),
            overallocationSize
        };
        return std::move(CoreGraphics::ResourceTableSet(crInfo));
    }
}

//------------------------------------------------------------------------------
/**
*/
const bool
ShaderHasResourceTable(const ShaderId id, const IndexT group)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    return info.descriptorSetLayoutMap.FindIndex(group) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
ShaderCreateConstantBuffer(const CoreGraphics::ShaderId id, const Util::StringAtom& name, CoreGraphics::BufferAccessMode mode)
{
    const auto& uniformBuffers = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffersByName;
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
    const auto& uniformBuffers = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers;
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
const IndexT
ShaderGetConstantBinding(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    IndexT index = info.constantBindings.FindIndex(name.Value());
    if (index == InvalidIndex)  return { INT32_MAX }; // invalid binding
    else                        return info.constantBindings.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantBinding(const CoreGraphics::ShaderId id, const IndexT cIndex)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    return info.constantBindings.ValueAtIndex(cIndex);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantBindingsCount(const CoreGraphics::ShaderId id)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    return info.constantBindings.Size();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableLayoutId
ShaderGetResourceTableLayout(const CoreGraphics::ShaderId id, const IndexT group)
{
    const VkShaderSetupInfo& setupInfo = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    uint layout = setupInfo.descriptorSetLayoutMap[group];
    return Util::Get<1>(setupInfo.descriptorSetLayouts[layout]);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourcePipelineId
ShaderGetResourcePipeline(const CoreGraphics::ShaderId id)
{
    return shaderAlloc.Get<Shader_SetupInfo>(id.resourceId).pipelineLayout;
}

//------------------------------------------------------------------------------
/**
*/
const Resources::ResourceName
ShaderGetName(const ShaderId id)
{
    return shaderAlloc.Get<Shader_SetupInfo>(id.resourceId).name;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantCount(const CoreGraphics::ShaderId id)
{
    return shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variables.Size();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantType
ShaderGetConstantType(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variables[i];
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
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName[name];
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
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName[name];
    return var.blockName;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantBlockName(const CoreGraphics::ShaderId id, const IndexT cIndex)
{
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variables[cIndex];
    return var.blockName;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantName(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variables[i];
    return var.name;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantGroup(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    IndexT idx = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName.FindIndex(name);
    if (idx != InvalidIndex)
    {
        const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName.ValueAtIndex(idx);
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
    IndexT idx = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName.FindIndex(name);
    if (idx != InvalidIndex)
    {
        const VkReflectionInfo::Variable& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName.ValueAtIndex(idx);
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
    return shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers.Size();
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers[i];
    return var.byteSize;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers[i];
    return var.name;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantBufferResourceSlot(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers[i];
    return var.binding;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantBufferResourceGroup(const CoreGraphics::ShaderId id, const IndexT i)
{
    const VkReflectionInfo::UniformBuffer& var = shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers[i];
    return var.set;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetResourceSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    const VkShaderSetupInfo& info = shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
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
    return shaderAlloc.Get<Shader_RuntimeInfo>(id.resourceId).programMap;
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
    VkShaderRuntimeInfo& runtime = shaderAlloc.Get<Shader_RuntimeInfo>(id.resourceId);
    IndexT i = runtime.programMap.FindIndex(mask);
    if (i == InvalidIndex)  return CoreGraphics::InvalidShaderProgramId;
    else                    return runtime.programMap.ValueAtIndex(i);
}

} // namespace CoreGraphics
