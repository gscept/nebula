//------------------------------------------------------------------------------
// vkshader.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshader.h"
#include "vkconstantbuffer.h"
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
	const VkPhysicalDeviceProperties props,
	AnyFX::ShaderEffect* effect,
	VkPushConstantRange& constantRange,
	Util::Array<CoreGraphics::SamplerId>& immutableSamplers,
	Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
	Util::Dictionary<uint32_t, uint32_t>& setLayoutMap,
	CoreGraphics::ResourcePipelineId& pipelineLayout,
	Util::Dictionary<Util::StringAtom, uint32_t>& resourceSlotMapping,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBinding>& constantBindings
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
	constantRange.size = props.limits.maxPushConstantsSize;
	constantRange.offset = 0;
	constantRange.stageFlags = VK_SHADER_STAGE_ALL;
	bool usePushConstants = false;
	uint32_t pushConstantSet = 0xFFFFFFFF;

#define uint_max(a, b) (a > b ? a : b)

    // assert we are not over-stepping any uniform buffer limit we are using, perStage is used for ALL_STAGES
	uint32_t maxUniformBufferRange = props.limits.maxUniformBufferRange;
    uint32_t maxUniformBuffersDyn = props.limits.maxDescriptorSetUniformBuffersDynamic;
	uint32_t maxUniformBuffers = props.limits.maxDescriptorSetUniformBuffers;
	uint32_t numUniformDyn = 0;
	uint32_t numUniform = 0;
	uint i;
	for (i = 0; i < varblocks.size(); i++) 
	{ 
		AnyFX::VkVarblock* block = static_cast<AnyFX::VkVarblock*>(varblocks[i]);
		VkDescriptorSetLayoutBinding& binding = block->bindingLayout;
		ResourceTableLayoutConstantBuffer cbo;
		cbo.slot = binding.binding;
		cbo.num = binding.descriptorCount;
		cbo.visibility = AllVisibility;
		uint32_t slotsUsed = 0;

		if (block->variables.empty()) continue;
		if (AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
		{
			usePushConstants = true;
			pushConstantSet = block->set;
			continue; // if push-constant block, do not add to resource table!
		};

		// add to resource map
		resourceSlotMapping.Add(block->name.c_str(), block->binding);
		ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.AddUnique(block->set);
		numsets = uint_max(numsets, block->set + 1);

		if (block->HasAnnotation("Visibility"))
		{
			CoreGraphicsShaderVisibility vis = ShaderVisibilityFromString(block->GetAnnotationString("Visibility").c_str());
			cbo.visibility = vis;
			if ((vis & VertexShaderVisibility) == VertexShaderVisibility)		slotsUsed++;
			if ((vis & HullShaderVisibility) == HullShaderVisibility)			slotsUsed++;
			if ((vis & DomainShaderVisibility) == DomainShaderVisibility)		slotsUsed++;
			if ((vis & GeometryShaderVisibility) == GeometryShaderVisibility)	slotsUsed++;
			if ((vis & PixelShaderVisibility) == PixelShaderVisibility)			slotsUsed++;
			if ((vis & ComputeShaderVisibility) == ComputeShaderVisibility)		slotsUsed++;
		}
		if (block->set == NEBULA_DYNAMIC_OFFSET_GROUP) { cbo.dynamicOffset = true; numUniformDyn += slotsUsed; }
		else											{ cbo.dynamicOffset = false; numUniform += slotsUsed; }

		rinfo.constantBuffers.Append(cbo);
		n_assert(block->alignedSize < maxUniformBufferRange);

		const std::vector<AnyFX::VariableBase*>& vars = block->variables;
		uint j;
		for (j = 0; j < vars.size(); j++)
		{
			const AnyFX::VariableBase* var = vars[j];
#if NEBULA_DEBUG
			n_assert(!constantBindings.Contains(var->name.c_str()));
#endif
			constantBindings.Add(var->name.c_str(), { block->offsetsByName[var->name], (uint)var->arraySize, (uint)var->byteSize });
		}
	}
    n_assert(maxUniformBuffersDyn >= numUniformDyn);
	n_assert(maxUniformBuffers >= numUniform);
    uint32_t maxPerStageUniformBuffers = props.limits.maxPerStageDescriptorUniformBuffers;
    n_assert(maxPerStageUniformBuffers >= varblocks.size());

	// do the same for storage buffers
	uint32_t maxStorageBufferRange = props.limits.maxStorageBufferRange;
	uint32_t maxStorageBuffersDyn = props.limits.maxDescriptorSetStorageBuffersDynamic;
	uint32_t maxStorageBuffers = props.limits.maxDescriptorSetStorageBuffers;
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

		if (buffer->size == 0) continue;
		ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.AddUnique(buffer->set);
		numsets = uint_max(numsets, buffer->set + 1);

		if (buffer->HasAnnotation("Visibility"))
		{
			CoreGraphicsShaderVisibility vis = ShaderVisibilityFromString(buffer->GetAnnotationString("Visibility").c_str());
			rwbo.visibility = vis;
			if ((vis & VertexShaderVisibility) == VertexShaderVisibility)		slotsUsed++;
			if ((vis & HullShaderVisibility) == HullShaderVisibility)			slotsUsed++;
			if ((vis & DomainShaderVisibility) == DomainShaderVisibility)		slotsUsed++;
			if ((vis & GeometryShaderVisibility) == GeometryShaderVisibility)	slotsUsed++;
			if ((vis & PixelShaderVisibility) == PixelShaderVisibility)			slotsUsed++;
			if ((vis & ComputeShaderVisibility) == ComputeShaderVisibility)		slotsUsed++;
		}

		if (buffer->set == NEBULA_DYNAMIC_OFFSET_GROUP) { rwbo.dynamicOffset = true; numStorageDyn += slotsUsed; }
		else											 { rwbo.dynamicOffset = false; numStorage += slotsUsed; }

		rinfo.rwBuffers.Append(rwbo);
		n_assert(buffer->alignedSize < maxStorageBufferRange);
	}
	n_assert(maxStorageBuffersDyn >= numStorageDyn);
	n_assert(maxStorageBuffers >= numStorage);
	uint32_t maxPerStageStorageBuffers = props.limits.maxPerStageDescriptorStorageBuffers;
	n_assert(maxPerStageStorageBuffers >= varbuffers.size());

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
				CoreGraphicsShaderVisibility vis = ShaderVisibilityFromString(sampler->GetAnnotationString("Visibility").c_str());
				smla.visibility = vis;
			}
			smla.slot = sampler->bindingLayout.binding;
			smla.sampler = samp;

			ResourceTableLayoutCreateInfo& rinfo = layoutCreateInfos.AddUnique(sampler->set);
			rinfo.samplers.Append(smla);

			numsets = uint_max(numsets, sampler->set + 1);
		}
	}

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
			tex.immutableSampler = SamplerId::Invalid();
			tex.visibility = AllVisibility;
			
			if (variable->HasAnnotation("Visibility"))
			{
				CoreGraphicsShaderVisibility vis = ShaderVisibilityFromString(variable->GetAnnotationString("Visibility").c_str());
				tex.visibility = vis;
			}

            if (remainingTextures < (uint32_t)variable->arraySize) n_error("Too many textures in shader!");
            else
            {
                remainingTextures -= variable->arraySize;
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
			if (variable->bindingLayout.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)	info.rwTextures.Append(tex);
			else																			info.textures.Append(tex);

			numsets = uint_max(numsets, variable->set + 1);
		}
		else if (variable->type >= AnyFX::InputAttachment && variable->type <= AnyFX::InputAttachmentUIntegerMS)
		{
			resourceSlotMapping.Add(variable->name.c_str(), variable->binding);
			ResourceTableLayoutInputAttachment ia;
			ia.slot = variable->bindingLayout.binding;
			ia.num = variable->bindingLayout.descriptorCount;
			ia.visibility = PixelShaderVisibility;				// only visible from pixel shaders anyways...

			ResourceTableLayoutCreateInfo& info = layoutCreateInfos.AddUnique(variable->set);
			info.inputAttachments.Append(ia);

			numsets = uint_max(numsets, variable->set + 1);
		}
	}

	// create a string for caching pipelines
	Util::String pipelineSignature;

	// skip the rest if we don't have any descriptor sets
	if (!layoutCreateInfos.IsEmpty())
	{
		setLayouts.Resize(numsets);
		for (IndexT i = 0; i < layoutCreateInfos.Size(); i++)
		{
			const ResourceTableLayoutCreateInfo& info = layoutCreateInfos.ValueAtIndex(i);
			ResourceTableLayoutId layout = CreateResourceTableLayout(info);
			setLayouts[i] = std::make_pair(layoutCreateInfos.KeyAtIndex(i), layout);
			setLayoutMap.Add(layoutCreateInfos.KeyAtIndex(i), i);
		}
	}

	Util::Array<ResourceTableLayoutId> layoutList;
	Util::Array<uint32_t> layoutIndices;
	for (IndexT i = 0; i < setLayouts.Size(); i++)
	{
		const ResourceTableLayoutId& a = std::get<1>(setLayouts[i]);
		if (a != ResourceTableLayoutId::Invalid())
		{
			layoutList.Append(a);
			layoutIndices.Append(std::get<0>(setLayouts[i]));
		}
	}
	ResourcePipelinePushConstantRange push;
	push.size = props.limits.maxPushConstantsSize;
	push.offset = 0;
	push.vis = AllVisibility;
	ResourcePipelineCreateInfo piInfo =
	{
		layoutList, layoutIndices, push
	};
	pipelineLayout = CreateResourcePipeline(piInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderCleanup(
	VkDevice dev,
	Util::Array<CoreGraphics::SamplerId>& immutableSamplers,
	Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers,
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
		if (std::get<1>(setLayouts[i]) != CoreGraphics::ResourceTableLayoutId::Invalid())
			CoreGraphics::DestroyResourceTableLayout(std::get<1>(setLayouts[i]));
	}
	setLayouts.Clear();

	for (i = 0; i < buffers.Size(); i++)
	{
		CoreGraphics::DestroyConstantBuffer(buffers.ValueAtIndex(i));
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