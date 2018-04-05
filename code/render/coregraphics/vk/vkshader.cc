//------------------------------------------------------------------------------
// vkshader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshader.h"
#include "vkconstantbuffer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"
#include "lowlevel/vk/vksampler.h"

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
	Util::Dictionary<uint32_t, Util::Array<VkDescriptorSetLayoutBinding>>& setBindings,
	Util::Array<VkSampler>& immutableSamplers,
	Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
	VkPipelineLayout& pipelineLayout,
	Util::FixedArray<VkDescriptorSet>& sets,
	Util::FixedArray<VkDescriptorPool>& setPools,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers,
	Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>>& buffersByGroup
	)
{
	const std::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks();
	const std::vector<AnyFX::VarbufferBase*>& varbuffers = effect->GetVarbuffers();
	const std::vector<AnyFX::VariableBase*>& variables = effect->GetVariables();
	const std::vector<AnyFX::SamplerBase*>& samplers = effect->GetSamplers();

    // assert we are not over-stepping any uniform buffer limit we are using, perStage is used for ALL_STAGES
	uint32_t maxUniformBufferRange = props.limits.maxUniformBufferRange;
    uint32_t maxUniformBuffersDyn = props.limits.maxDescriptorSetUniformBuffersDynamic;
	uint32_t maxUniformBuffers = props.limits.maxDescriptorSetUniformBuffers;
	uint32_t numUniformDyn = 0;
	uint32_t numUniform = 0;
	uint i;
	for (i = 0; i < varblocks.size(); i++) 
	{ 
		if (varblocks[i]->Flag("DynamicOffset")) numUniformDyn++;
		else									 numUniform++;
		n_assert(varblocks[i]->alignedSize < maxUniformBufferRange);
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
		if (varbuffers[i]->Flag("DynamicOffset")) maxStorageBuffersDyn++;
		else									  maxStorageBuffers++;
		n_assert(varbuffers[i]->alignedSize < maxStorageBufferRange);
	}
	n_assert(maxStorageBuffersDyn >= maxStorageBuffersDyn);
	n_assert(maxStorageBuffers >= maxStorageBuffers);
	uint32_t maxPerStageStorageBuffers = props.limits.maxPerStageDescriptorStorageBuffers;
	n_assert(maxPerStageStorageBuffers >= varbuffers.size());

    uint32_t maxTextures = props.limits.maxDescriptorSetSampledImages;
    uint32_t remainingTextures = maxTextures;

	// always create push constant range in layout, making all shaders using push constants compatible
	constantRange.size = props.limits.maxPushConstantsSize;
	constantRange.offset = 0;
	constantRange.stageFlags = VK_SHADER_STAGE_ALL;
	bool usePushConstants = false;
	uint32_t numsets = 0;

	Util::Dictionary<IndexT, Util::String> signatures;

	Util::Array<VkDescriptorBufferInfo> bufs;
	Util::Array<VkDescriptorImageInfo> imgs;
	Util::Array<VkWriteDescriptorSet> writes;
	Util::Array<AnyFX::VkSampler*> boundSamplers;

#define uint_max(a, b) (a > b ? a : b)

	// setup varblocks
	for (i = 0; i < varblocks.size(); i++)
	{
		AnyFX::VkVarblock* block = static_cast<AnyFX::VkVarblock*>(varblocks[i]);
		VkDescriptorSetLayoutBinding& binding = block->bindingLayout;
		if (block->Flag("DynamicOffset")) binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		else							  binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		if (block->variables.empty()) continue;
		if (AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
		{
			// can only have one push constant block
			n_assert(usePushConstants == false);
			n_assert(block->alignedSize <= constantRange.size);
			// don't really do anything here...
		}
		else
		{
			IndexT index = setBindings.FindIndex(block->set);
			if (index == InvalidIndex)
			{
				Util::Array<VkDescriptorSetLayoutBinding> arr;
				arr.Append(binding);
				setBindings.Add(block->set, arr);
				signatures.Add(block->set, VkShaderCreateSignature(binding));
				numsets = uint_max(numsets, block->set + 1);

			}
			else
			{
				setBindings.ValueAtIndex(index).Append(binding);
				signatures.ValueAtIndex(index).Append(VkShaderCreateSignature(binding));
			}
		}
	}

	// setup varbuffers
	for (i = 0; i < varbuffers.size(); i++)
	{
		AnyFX::VkVarbuffer* buffer = static_cast<AnyFX::VkVarbuffer*>(varbuffers[i]);
		VkDescriptorSetLayoutBinding binding = buffer->bindingLayout;
		if (buffer->Flag("DynamicOffset")) binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		else							   binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		IndexT index = setBindings.FindIndex(buffer->set);
		if (index == InvalidIndex)
		{
			Util::Array<VkDescriptorSetLayoutBinding> arr;
			arr.Append(binding);
			setBindings.Add(buffer->set, arr);
			signatures.Add(buffer->set, VkShaderCreateSignature(binding));
			numsets = uint_max(numsets, buffer->set + 1);
		}
		else
		{
			setBindings.ValueAtIndex(index).Append(binding);
			signatures.ValueAtIndex(index).Append(VkShaderCreateSignature(binding));
		}
	}

	// setup samplers as immutable coupled with the texture input, before we setup the variables so that it's part of their layout
	immutableSamplers.Reserve((SizeT)samplers.size() + 1);
	for (i = 0; i < samplers.size(); i++)
	{
		AnyFX::VkSampler* sampler = static_cast<AnyFX::VkSampler*>(samplers[i]);
		if (!sampler->textureVariables.empty())
		{
			VkSampler vkSampler;
			VkResult res = vkCreateSampler(dev, &sampler->samplerInfo, NULL, &vkSampler);
			n_assert(res == VK_SUCCESS);

			// add to list so we can remove it later
			immutableSamplers.Append(vkSampler);

			uint j;
			for (j = 0; j < sampler->textureVariables.size(); j++)
			{
				AnyFX::VkVariable* var = static_cast<AnyFX::VkVariable*>(sampler->textureVariables[j]);
				n_assert(var->type >= AnyFX::Sampler1D && var->type <= AnyFX::SamplerCubeArray);
				var->bindingLayout.pImmutableSamplers = &immutableSamplers.Back();
			}
		}
		else
		{
			// create separate sampler
			VkSampler vkSampler;
			VkResult res = vkCreateSampler(dev, &sampler->samplerInfo, NULL, &vkSampler);
			n_assert(res == VK_SUCCESS);
			immutableSamplers.Append(vkSampler);

			sampler->bindingLayout.pImmutableSamplers = &immutableSamplers.Back();
			boundSamplers.Append(sampler);
			IndexT index = setBindings.FindIndex(sampler->set);
			if (index == InvalidIndex)
			{
				Util::Array<VkDescriptorSetLayoutBinding> arr;
				arr.Append(sampler->bindingLayout);
				setBindings.Add(sampler->set, arr);
				signatures.Add(sampler->set, VkShaderCreateSignature(sampler->bindingLayout));
				numsets = uint_max(numsets, sampler->set + 1);
			}
			else
			{
				setBindings.ValueAtIndex(index).Append(sampler->bindingLayout);
				signatures.ValueAtIndex(index).Append(VkShaderCreateSignature(sampler->bindingLayout));
			}
		}
	}

	VkSamplerCreateInfo placeholderSamplerInfo =
	{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		NULL,
		0,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		0,
		false,
		16,
		0,
		VK_COMPARE_OP_NEVER,
		-FLT_MAX,
		FLT_MAX,
		VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		VK_FALSE							
	};

	// create placeholder sampler
	VkSampler placeholderSampler;
	VkResult res = vkCreateSampler(dev, &placeholderSamplerInfo, NULL, &placeholderSampler);
	n_assert(res == VK_SUCCESS);
	immutableSamplers.Append(placeholderSampler);

	// setup variables
	for (i = 0; i < variables.size(); i++)
	{
		AnyFX::VkVariable* variable = static_cast<AnyFX::VkVariable*>(variables[i]);

		// handle samplers, images and textures
		if (variable->type >= AnyFX::Sampler1D && variable->type <= AnyFX::TextureCubeArray)
		{
            if (remainingTextures < (uint32_t)variable->arraySize) n_error("Too many textures in shader!");
            else
            {
                remainingTextures -= variable->arraySize;
            }
			if (variable->bindingLayout.pImmutableSamplers == NULL && 
				variable->bindingLayout.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				variable->bindingLayout.pImmutableSamplers = &placeholderSampler;
			}
			IndexT index = setBindings.FindIndex(variable->set);
			if (index == InvalidIndex)
			{
				Util::Array<VkDescriptorSetLayoutBinding> arr;
				arr.Append(variable->bindingLayout);
				setBindings.Add(variable->set, arr);
				signatures.Add(variable->set, VkShaderCreateSignature(variable->bindingLayout));
				numsets = uint_max(numsets, variable->set + 1);
			}
			else
			{
				setBindings.ValueAtIndex(index).Append(variable->bindingLayout);
				signatures.ValueAtIndex(index).Append(VkShaderCreateSignature(variable->bindingLayout));
			}
		}
		else if (variable->type >= AnyFX::InputAttachment && variable->type <= AnyFX::InputAttachmentUIntegerMS)
		{
			IndexT index = setBindings.FindIndex(variable->set);
			if (index == InvalidIndex)
			{
				Util::Array<VkDescriptorSetLayoutBinding> arr;
				arr.Append(variable->bindingLayout);
				setBindings.Add(variable->set, arr);
				signatures.Add(variable->set, VkShaderCreateSignature(variable->bindingLayout));
				numsets = uint_max(numsets, variable->set + 1);
			}
			else
			{
				setBindings.ValueAtIndex(index).Append(variable->bindingLayout);
				signatures.ValueAtIndex(index).Append(VkShaderCreateSignature(variable->bindingLayout));
			}
		}
	}

	// create a string for caching pipelines
	Util::String pipelineSignature;

	// skip the rest if we don't have any descriptor sets
	if (!setBindings.IsEmpty())
	{
		setLayouts.Resize(numsets);
		for (IndexT i = 0; i < setLayouts.Size(); i++)
		{
			// if signature is defined in this shader, retrieve it
			IndexT layoutIndex = InvalidIndex;
			Util::String signature;
			if (signatures.Contains(i))
			{
				signature = signatures[i];
				layoutIndex = VkShaderLayoutCache.FindIndex(signature);
			}

			// setup layout if this is the first time (during the program) we encounter it
			if (layoutIndex == InvalidIndex)
			{
				VkDescriptorSetLayoutCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				info.pNext = NULL;
				info.flags = 0;

				IndexT bindingIndex = setBindings.FindIndex(i);
				info.bindingCount = 0;
				info.pBindings = VK_NULL_HANDLE;
				if (bindingIndex != InvalidIndex)
				{
					const Util::Array<VkDescriptorSetLayoutBinding>& binds = setBindings.ValueAtIndex(bindingIndex);
					info.bindingCount = binds.Size();
					info.pBindings = binds.Size() > 0 ? &binds[0] : VK_NULL_HANDLE;
				}

				// create layout
				VkResult res = vkCreateDescriptorSetLayout(dev, &info, NULL, &setLayouts[i]);
				assert(res == VK_SUCCESS);

				// add to cache if this shader defined the signature
				if (bindingIndex != InvalidIndex) VkShaderLayoutCache.Add(signature, setLayouts[i]);
			}
			else
			{
				// if this layout has been created before, fetch it from the global cache
				setLayouts[i] = VkShaderLayoutCache.ValueAtIndex(layoutIndex);
			}

			// construct pipeline signature
			pipelineSignature.Append(Util::String::Sprintf("%p", setLayouts[i]) + ";");
		}
	}

	IndexT idx = VkShaderPipelineCache.FindIndex(pipelineSignature);
	if (idx == InvalidIndex)
	{
		// create one pipeline layout for each descriptor set, and one for the entire shader object
		VkPipelineLayoutCreateInfo layoutInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			NULL,
			0,
			setLayouts.Size(),
			setLayouts.Size() > 0 ? setLayouts.Begin() : NULL,
			1,
			&constantRange
		};

		// create pipeline layout, every program should inherit this one
		res = vkCreatePipelineLayout(dev, &layoutInfo, NULL, &pipelineLayout);
		assert(res == VK_SUCCESS);

		// add to cache
		VkShaderPipelineCache.Add(pipelineSignature, pipelineLayout);
	}
	else
	{
		// fetch from cache
		pipelineLayout = VkShaderPipelineCache.ValueAtIndex(idx);
	}

    sets.Resize(setLayouts.Size());
    sets.Fill(VK_NULL_HANDLE);
	setPools.Resize(setLayouts.Size());
	setPools.Fill(VK_NULL_HANDLE);
    for (IndexT i = 0; i < setLayouts.Size(); i++)
    {
        // if signature is defined in this shader, retrieve it
        IndexT layoutIndex = InvalidIndex;
        Util::String signature;
        if (signatures.Contains(i))
        {
            signature = signatures[i];
            layoutIndex = VkShaderLayoutCache.FindIndex(signature);
			IndexT idx = VkShaderDescriptorSetCache.FindIndex(signature);
			if (idx == InvalidIndex)
			{

			createset:
				// get current pool
				VkDescriptorPool pool = VkRenderDevice::Instance()->GetCurrentDescriptorPool();

				// allocate descriptor sets
				VkDescriptorSetAllocateInfo info =
				{
					VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					NULL,
					pool,
					1,
					&setLayouts[i]
				};
				VkDescriptorSet set;
				res = vkAllocateDescriptorSets(dev, &info, &set);
				if (res == VK_ERROR_OUT_OF_POOL_MEMORY_KHR)
				{
					// if we are out of pool memory, create a new pool!
					VkRenderDevice::Instance()->RequestDescriptorPool();
					goto createset;
				}
				n_assert(res == VK_SUCCESS);				

				// add to cache
				sets[i] = set;
				setPools[i] = pool;
				VkShaderDescriptorSetCache.Add(pipelineSignature, set);
        	}
		}
		else
		{
			sets[i] = VkShaderDescriptorSetCache[pipelineSignature];
		}
    }

	// setup varblock backing (this is for the shader default state)
	bufs.Reserve((SizeT)varblocks.size());
	for (i = 0; i < varblocks.size(); i++)
	{
		// get block
		AnyFX::VarblockBase* block = varblocks[i];

		bool usedBySystem = false;
		if (block->HasAnnotation("System")) usedBySystem = block->GetAnnotationBool("System");

		CoreGraphics::ConstantBufferId uniformBuffer = CoreGraphics::ConstantBufferId::Invalid();

		// only create buffer if block is not handled by system
		if (!usedBySystem && block->alignedSize > 0 && !AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
		{
			// create uniform buffer, with single backing
			CoreGraphics::ConstantBufferCreateInfo cbInfo = { false, CoreGraphics::ShaderStateId::Invalid(), block->name.c_str(), block->alignedSize };
			uniformBuffer = CreateConstantBuffer(cbInfo);

			// generate a name which we know will be unique
			Util::String name = block->name.c_str();
			n_assert(!buffers.Contains(name));

			VkDescriptorBufferInfo buf;
			buf.buffer = ConstantBufferGetVk(uniformBuffer);
			buf.offset = 0;
			buf.range = VK_WHOLE_SIZE;
			bufs.Append(buf);

			VkWriteDescriptorSet write;
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.pNext = NULL;
			write.dstBinding = block->binding;
			write.dstSet = sets[block->set];
			write.descriptorCount = 1;
			if (block->Flag("DynamicOffset")) write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			else							  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.dstArrayElement = 0;
			write.pTexelBufferView = NULL;
			write.pImageInfo = NULL;
			write.pBufferInfo = &bufs.Back();
			writes.Append(write);			

			// add buffer to list
			buffers.Add(name, uniformBuffer);
		}

		if (!AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
		{
			if (!buffersByGroup.Contains(block->set)) buffersByGroup.Add(block->set, Util::Array<CoreGraphics::ConstantBufferId>());
			buffersByGroup[block->set].Append(uniformBuffer);
		}		
	}

	// update descriptors
	if (writes.Size() > 0)
	{
		vkUpdateDescriptorSets(dev, writes.Size(), &writes[0], 0, NULL);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderCleanup(
	VkDevice dev,
	Util::Array<VkSampler>& immutableSamplers,
	Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
	VkPipelineLayout& pipelineLayout
)
{
	IndexT i;
	for (i = 0; i < immutableSamplers.Size(); i++)
	{
		vkDestroySampler(dev, immutableSamplers[i], nullptr);
	}
	immutableSamplers.Clear();

	for (i = 0; i < setLayouts.Size(); i++)
	{
		vkDestroyDescriptorSetLayout(dev, setLayouts[i], nullptr);
	}
	setLayouts.Clear();

	vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderGetVkShaderVariableBinding(const CoreGraphics::ShaderStateId shader, const CoreGraphics::ShaderConstantId var)
{
	return CoreGraphics::shaderPool->shaderAlloc.Get<4>(shader.shaderId).Get<3>(shader.stateId).Get<1>(var.id).setBinding;
}

//------------------------------------------------------------------------------
/**
*/
VkDescriptorSet
VkShaderGetVkShaderVariableDescriptorSet(const CoreGraphics::ShaderStateId shader, const CoreGraphics::ShaderConstantId var)
{
	return CoreGraphics::shaderPool->shaderAlloc.Get<4>(shader.shaderId).Get<3>(shader.stateId).Get<1>(var.id).set;
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