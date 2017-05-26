//------------------------------------------------------------------------------
// vkshader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshader.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadervariation.h"
#include "lowlevel/vk/vksampler.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkShader, 'VKSH', Base::ShaderBase);

Util::Dictionary<Util::StringAtom, VkDescriptorSetLayout> VkShader::LayoutCache;
Util::Dictionary<Util::StringAtom, VkPipelineLayout> VkShader::ShaderPipelineCache;
Util::Dictionary<Util::StringAtom, VkDescriptorSet> VkShader::DescriptorSetCache;
//------------------------------------------------------------------------------
/**
*/
VkShader::VkShader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkShader::~VkShader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkShader::Unload()
{
	ShaderBase::Unload();
	this->Cleanup();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShader::Reload()
{

}

//------------------------------------------------------------------------------
/**
*/
void
VkShader::Cleanup()
{
	IndexT i;
	for (i = 0; i < this->immutableSamplers.Size(); i++)
	{
		vkDestroySampler(VkRenderDevice::dev, this->immutableSamplers[i], nullptr);
	}
	this->immutableSamplers.Clear();

	for (i = 0; i < this->setLayouts.Size(); i++)
	{
		vkDestroyDescriptorSetLayout(VkRenderDevice::dev, this->setLayouts[i], nullptr);
	}
	this->setLayouts.Clear();

	vkDestroyPipelineLayout(VkRenderDevice::dev, this->pipelineLayout, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShader::OnLostDevice()
{

}

//------------------------------------------------------------------------------
/**
*/
void
VkShader::OnResetDevice()
{

}

//------------------------------------------------------------------------------
/**
*/
void
VkShader::Setup(AnyFX::ShaderEffect* effect)
{
	const eastl::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks();
	const eastl::vector<AnyFX::VarbufferBase*>& varbuffers = effect->GetVarbuffers();
	const eastl::vector<AnyFX::VariableBase*>& variables = effect->GetVariables();
	const eastl::vector<AnyFX::SamplerBase*>& samplers = effect->GetSamplers();

    // assert we are not over-stepping any uniform buffer limit we are using, perStage is used for ALL_STAGES
	uint32_t maxUniformBufferRange = VkRenderDevice::Instance()->deviceProps.limits.maxUniformBufferRange;
    uint32_t maxUniformBuffersDyn = VkRenderDevice::Instance()->deviceProps.limits.maxDescriptorSetUniformBuffersDynamic;
	uint32_t maxUniformBuffers = VkRenderDevice::Instance()->deviceProps.limits.maxDescriptorSetUniformBuffers;
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
    uint32_t maxPerStageUniformBuffers = VkRenderDevice::Instance()->deviceProps.limits.maxPerStageDescriptorUniformBuffers;
    n_assert(maxPerStageUniformBuffers >= varblocks.size());

	// do the same for storage buffers
	uint32_t maxStorageBufferRange = VkRenderDevice::Instance()->deviceProps.limits.maxStorageBufferRange;
	uint32_t maxStorageBuffersDyn = VkRenderDevice::Instance()->deviceProps.limits.maxDescriptorSetStorageBuffersDynamic;
	uint32_t maxStorageBuffers = VkRenderDevice::Instance()->deviceProps.limits.maxDescriptorSetStorageBuffers;
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
	uint32_t maxPerStageStorageBuffers = VkRenderDevice::Instance()->deviceProps.limits.maxPerStageDescriptorStorageBuffers;
	n_assert(maxPerStageStorageBuffers >= varbuffers.size());

    uint32_t maxTextures = VkRenderDevice::Instance()->deviceProps.limits.maxDescriptorSetSampledImages;
    uint32_t remainingTextures = maxTextures;

	// always create push constant range in layout, making all shaders using push constants compatible
	this->constantRange.size = VkRenderDevice::Instance()->deviceProps.limits.maxPushConstantsSize;
	this->constantRange.offset = 0;
	this->constantRange.stageFlags = VK_SHADER_STAGE_ALL;
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
			n_assert(block->alignedSize <= this->constantRange.size);
			// don't really do anything here...
		}
		else
		{
			IndexT index = setBindings.FindIndex(block->set);
			if (index == InvalidIndex)
			{
				Util::Array<VkDescriptorSetLayoutBinding> arr;
				arr.Append(binding);
				this->setBindings.Add(block->set, arr);
				signatures.Add(block->set, CreateSignature(binding));
#if AMD_DESC_SETS
				numsets = uint_max(numsets, block->set + 1);
#else
				numsets++;
#endif

			}
			else
			{
				this->setBindings.ValueAtIndex(index).Append(binding);
				signatures.ValueAtIndex(index).Append(CreateSignature(binding));
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
		IndexT index = this->setBindings.FindIndex(buffer->set);
		if (index == InvalidIndex)
		{
			Util::Array<VkDescriptorSetLayoutBinding> arr;
			arr.Append(binding);
			this->setBindings.Add(buffer->set, arr);
			signatures.Add(buffer->set, CreateSignature(binding));
#if AMD_DESC_SETS
			numsets = uint_max(numsets, buffer->set + 1);
#else
			numsets++;
#endif
		}
		else
		{
			this->setBindings.ValueAtIndex(index).Append(binding);
			signatures.ValueAtIndex(index).Append(CreateSignature(binding));
		}
	}

	// setup samplers as immutable coupled with the texture input, before we setup the variables so that it's part of their layout
	this->immutableSamplers.Reserve(samplers.size() + 1);
	for (i = 0; i < samplers.size(); i++)
	{
		AnyFX::VkSampler* sampler = static_cast<AnyFX::VkSampler*>(samplers[i]);
		if (!sampler->textureVariables.empty())
		{
			VkSampler vkSampler;
			VkResult res = vkCreateSampler(VkRenderDevice::dev, &sampler->samplerInfo, NULL, &vkSampler);
			n_assert(res == VK_SUCCESS);

			// add to list so we can remove it later
			this->immutableSamplers.Append(vkSampler);

			uint j;
			for (j = 0; j < sampler->textureVariables.size(); j++)
			{
				AnyFX::VkVariable* var = static_cast<AnyFX::VkVariable*>(sampler->textureVariables[j]);
				n_assert(var->type >= AnyFX::Sampler1D && var->type <= AnyFX::SamplerCubeArray);
				var->bindingLayout.pImmutableSamplers = &this->immutableSamplers.Back();
			}
		}
		else
		{
			// create separate sampler
			VkSampler vkSampler;
			VkResult res = vkCreateSampler(VkRenderDevice::dev, &sampler->samplerInfo, NULL, &vkSampler);
			n_assert(res == VK_SUCCESS);
			this->immutableSamplers.Append(vkSampler);

			sampler->bindingLayout.pImmutableSamplers = &this->immutableSamplers.Back();
			boundSamplers.Append(sampler);
			IndexT index = this->setBindings.FindIndex(sampler->set);
			if (index == InvalidIndex)
			{
				Util::Array<VkDescriptorSetLayoutBinding> arr;
				arr.Append(sampler->bindingLayout);
				this->setBindings.Add(sampler->set, arr);
				signatures.Add(sampler->set, CreateSignature(sampler->bindingLayout));
#if AMD_DESC_SETS
				numsets = uint_max(numsets, sampler->set + 1);
#else
				numsets++;
#endif
			}
			else
			{
				this->setBindings.ValueAtIndex(index).Append(sampler->bindingLayout);
				signatures.ValueAtIndex(index).Append(CreateSignature(sampler->bindingLayout));
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
	VkResult res = vkCreateSampler(VkRenderDevice::dev, &placeholderSamplerInfo, NULL, &placeholderSampler);
	n_assert(res == VK_SUCCESS);
	this->immutableSamplers.Append(placeholderSampler);

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
			IndexT index = this->setBindings.FindIndex(variable->set);
			if (index == InvalidIndex)
			{
				Util::Array<VkDescriptorSetLayoutBinding> arr;
				arr.Append(variable->bindingLayout);
				this->setBindings.Add(variable->set, arr);
				signatures.Add(variable->set, CreateSignature(variable->bindingLayout));
#if AMD_DESC_SETS
				numsets = uint_max(numsets, variable->set + 1);
#else
				numsets++;
#endif
			}
			else
			{
				this->setBindings.ValueAtIndex(index).Append(variable->bindingLayout);
				signatures.ValueAtIndex(index).Append(CreateSignature(variable->bindingLayout));
			}
		}
		else if (variable->type >= AnyFX::InputAttachment && variable->type <= AnyFX::InputAttachmentUIntegerMS)
		{
			IndexT index = this->setBindings.FindIndex(variable->set);
			if (index == InvalidIndex)
			{
				Util::Array<VkDescriptorSetLayoutBinding> arr;
				arr.Append(variable->bindingLayout);
				this->setBindings.Add(variable->set, arr);
				signatures.Add(variable->set, CreateSignature(variable->bindingLayout));
#if AMD_DESC_SETS
				numsets = uint_max(numsets, variable->set + 1);
#else
				numsets++;
#endif
			}
			else
			{
				this->setBindings.ValueAtIndex(index).Append(variable->bindingLayout);
				signatures.ValueAtIndex(index).Append(CreateSignature(variable->bindingLayout));
			}
		}
	}

	// create a string for caching pipelines
	Util::String pipelineSignature;

	// skip the rest if we don't have any descriptor sets
	if (!setBindings.IsEmpty())
	{
		this->setLayouts.Resize(numsets);
		for (IndexT i = 0; i < this->setLayouts.Size(); i++)
		{
			// if signature is defined in this shader, retrieve it
			IndexT layoutIndex = InvalidIndex;
			Util::String signature;
			if (signatures.Contains(i))
			{
				signature = signatures[i];
				layoutIndex = VkShader::LayoutCache.FindIndex(signature);
			}

			// setup layout if this is the first time (during the program) we encounter it
			if (layoutIndex == InvalidIndex)
			{
				VkDescriptorSetLayoutCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				info.pNext = NULL;
				info.flags = 0;

				IndexT bindingIndex = this->setBindings.FindIndex(i);
#if AMD_DESC_SETS
				info.bindingCount = 0;
				info.pBindings = VK_NULL_HANDLE;
				if (bindingIndex != InvalidIndex)
				{
					const Util::Array<VkDescriptorSetLayoutBinding>& binds = this->setBindings.ValueAtIndex(bindingIndex);
					info.bindingCount = binds.Size();
					info.pBindings = binds.Size() > 0 ? &binds[0] : VK_NULL_HANDLE;
				}
#else
				const Util::Array<VkDescriptorSetLayoutBinding>& binds = this->setBindings.ValueAtIndex(i);
				info.bindingCount = binds.Size();
				info.pBindings = binds.Size() > 0 ? &binds[0] : VK_NULL_HANDLE;
				this->setToIndexMap.Add(this->setBindings.KeyAtIndex(i), i);
#endif

				// create layout
				VkResult res = vkCreateDescriptorSetLayout(VkRenderDevice::dev, &info, NULL, &this->setLayouts[i]);
				assert(res == VK_SUCCESS);

				// add to cache if this shader defined the signature
				if (bindingIndex != InvalidIndex) VkShader::LayoutCache.Add(signature, this->setLayouts[i]);
			}
			else
			{
				// if this layout has been created before, fetch it from the global cache
				this->setLayouts[i] = VkShader::LayoutCache.ValueAtIndex(layoutIndex);
			}

			// construct pipeline signature
			pipelineSignature.Append(Util::String::FromLongLong(this->setLayouts[i]) + ";");
		}
	}

	IndexT idx = VkShader::ShaderPipelineCache.FindIndex(pipelineSignature);
	if (idx == InvalidIndex)
	{
		// create one pipeline layout for each descriptor set, and one for the entire shader object
		VkPipelineLayoutCreateInfo layoutInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			NULL,
			0,
			this->setLayouts.Size(),
			this->setLayouts.Size() > 0 ? this->setLayouts.Begin() : NULL,
			1,
			&this->constantRange
		};

		// create pipeline layout, every program should inherit this one
		res = vkCreatePipelineLayout(VkRenderDevice::dev, &layoutInfo, NULL, &this->pipelineLayout);
		assert(res == VK_SUCCESS);

		// add to cache
		VkShader::ShaderPipelineCache.Add(pipelineSignature, this->pipelineLayout);
	}
	else
	{
		// fetch from cache
		this->pipelineLayout = VkShader::ShaderPipelineCache.ValueAtIndex(idx);
	}

    this->sets.Resize(this->setLayouts.Size());
    this->sets.Fill(VK_NULL_HANDLE);
    for (IndexT i = 0; i < this->setLayouts.Size(); i++)
    {
        // if signature is defined in this shader, retrieve it
        IndexT layoutIndex = InvalidIndex;
        Util::String signature;
        if (signatures.Contains(i))
        {
            signature = signatures[i];
            layoutIndex = VkShader::LayoutCache.FindIndex(signature);
			IndexT idx = VkShader::DescriptorSetCache.FindIndex(signature);
			if (idx == InvalidIndex)
			{
				// allocate descriptor sets
				VkDescriptorSetAllocateInfo info =
				{
					VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					NULL,
					VkRenderDevice::descPool,
					1,
					&this->setLayouts[i]
				};
				VkDescriptorSet set;
				res = vkAllocateDescriptorSets(VkRenderDevice::dev, &info, &set);
				n_assert(res == VK_SUCCESS);

				// add to cache
				this->sets[i] = set;
				VkShader::DescriptorSetCache.Add(pipelineSignature, set);
        	}
		}
		else
		{
			this->sets[i] = VkShader::DescriptorSetCache[pipelineSignature];
		}
    }

	// setup varblock backing
	bufs.Reserve(varblocks.size());
	for (i = 0; i < varblocks.size(); i++)
	{
		// get block
		AnyFX::VarblockBase* block = varblocks[i];

		bool usedBySystem = false;
		if (block->HasAnnotation("System")) usedBySystem = block->GetAnnotationBool("System");

		Ptr<CoreGraphics::ConstantBuffer> uniformBuffer = NULL;
		// only create buffer if block is not handled by system
		if (!usedBySystem && block->alignedSize > 0 && !AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
		{
			// create uniform buffer, with single backing
			uniformBuffer = CoreGraphics::ConstantBuffer::Create();
			uniformBuffer->SetSize(block->alignedSize);
			uniformBuffer->Setup(1);

			// generate a name which we know will be unique
			Util::String name = block->name.c_str();
			n_assert(!this->buffers.Contains(name));

			VkDescriptorBufferInfo buf;
			buf.buffer = uniformBuffer->GetVkBuffer();
			buf.offset = 0;
			buf.range = VK_WHOLE_SIZE;
			bufs.Append(buf);

			VkWriteDescriptorSet write;
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.pNext = NULL;
			write.dstBinding = block->binding;
#if AMD_DESC_SETS
			write.dstSet = this->sets[block->set];
#else
			write.dstSet = this->sets[this->setToIndexMap[block->set]];
#endif
			write.descriptorCount = 1;
			if (block->Flag("DynamicOffset")) write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			else							  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.dstArrayElement = 0;
			write.pTexelBufferView = NULL;
			write.pImageInfo = NULL;
			write.pBufferInfo = &bufs.Back();
			writes.Append(write);			

			// add buffer to list
			this->buffers.Add(name, uniformBuffer);
		}

		if (!AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
		{
			if (!this->buffersByGroup.Contains(block->set)) this->buffersByGroup.Add(block->set, Util::Array<Ptr<CoreGraphics::ConstantBuffer>>());
			this->buffersByGroup[block->set].Append(uniformBuffer);
		}		
	}

	// update descriptors
	if (writes.Size() > 0)
	{
		vkUpdateDescriptorSets(VkRenderDevice::dev, writes.Size(), &writes[0], 0, NULL);
	}
}

//------------------------------------------------------------------------------
/**
*/
Util::String
VkShader::CreateSignature(const VkDescriptorSetLayoutBinding& bind)
{
	return Util::String::Sprintf("%d:%d:%d:%d:%p;", bind.binding, bind.descriptorCount, bind.descriptorType, bind.stageFlags, bind.pImmutableSamplers);
}

} // namespace Vulkan