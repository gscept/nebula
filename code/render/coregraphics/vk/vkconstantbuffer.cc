//------------------------------------------------------------------------------
// vkconstantbuffer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkrenderdevice.h"
#include "vkconstantbuffer.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/config.h"
#include "vkutilities.h"
#include "coregraphics/shaderpool.h"

namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
VkBuffer
ConstantBufferGetVk(const CoreGraphics::ConstantBufferId id)
{
	VkConstantBufferRuntimeInfo& runtime = constantBufferAllocator.Get<0>(id.id24);
	return runtime.buf;
}

//------------------------------------------------------------------------------
/**
	This handles resizing using the constant buffer ID saved with the stretch interface
*/
SizeT
ConstantBufferStretchInterface::Grow(const SizeT capacity, const SizeT numInstances, SizeT& newCapacity)
{
	VkConstantBufferRuntimeInfo& runtimeInfo = constantBufferAllocator.Get<0>(this->obj.id24);
	VkConstantBufferSetupInfo& setupInfo = constantBufferAllocator.Get<1>(this->obj.id24);
	VkConstantBufferMapInfo& mapInfo = constantBufferAllocator.Get<2>(this->obj.id24);
#if NEBULA_DEBUG
	n_assert(this->obj.id8 == ConstantBufferIdType);
#endif
	// new capacity is the old one, plus the number of elements we wish to allocate, although never allocate fewer than grow
	SizeT increment = capacity >> 1;
	increment = Math::n_iclamp(increment, setupInfo.grow, 65535);
	n_assert(increment >= numInstances);
	newCapacity = capacity + increment;

	// create new buffer
	const std::array<uint32_t, 4> queues = VkRenderDevice::Instance()->GetQueueFamilies();
	setupInfo.info.pQueueFamilyIndices = queues.data();
	setupInfo.info.queueFamilyIndexCount = (uint32_t)queues.size();
	setupInfo.info.size = newCapacity * setupInfo.stride;

	VkBuffer newBuf;
	VkResult res = vkCreateBuffer(setupInfo.dev, &setupInfo.info, NULL, &newBuf);
	n_assert(res == VK_SUCCESS);

	// allocate new instance memory, alignedSize is the aligned size of a single buffer
	VkDeviceMemory newMem;
	uint32_t alignedSize;
	VkUtilities::AllocateBufferMemory(setupInfo.dev, newBuf, newMem, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), alignedSize);

	// bind to buffer, this is the reason why we must destroy and create the buffer again
	res = vkBindBufferMemory(setupInfo.dev, newBuf, newMem, 0);
	n_assert(res == VK_SUCCESS);

	// copy old to new, old memory is already mapped
	void* dstData;

	// map new memory with new capacity, avoids a second map
	res = vkMapMemory(setupInfo.dev, newMem, 0, alignedSize, 0, &dstData);
	n_assert(res == VK_SUCCESS);
	memcpy(dstData, mapInfo.data, setupInfo.size);
	vkUnmapMemory(setupInfo.dev, setupInfo.mem);

	// clean up old data	
	vkDestroyBuffer(setupInfo.dev, runtimeInfo.buf, NULL);
	vkFreeMemory(setupInfo.dev, setupInfo.mem, NULL);

	// replace old device memory and size
	setupInfo.size = alignedSize;
	runtimeInfo.buf = newBuf;
	setupInfo.mem = newMem;
	mapInfo.data = dstData;
	return alignedSize;
}


} // namespace Vulkan

namespace Vulkan
{

VkConstantBufferAllocator constantBufferAllocator(0x00FFFFFF);

}

namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
const ConstantBufferId
CreateConstantBuffer(const ConstantBufferCreateInfo& info)
{
	Ids::Id32 id = constantBufferAllocator.AllocObject();
	VkConstantBufferRuntimeInfo& runtime = constantBufferAllocator.Get<0>(id);
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<1>(id);
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<2>(id);
	ConstantBufferStretchInterface& stretch = constantBufferAllocator.Get<3>(id);

	VkDevice dev = VkRenderDevice::Instance()->GetCurrentDevice();
	VkPhysicalDeviceProperties props = VkRenderDevice::Instance()->GetCurrentProperties();

	setup.reflection = nullptr;
	setup.dev = dev;
	setup.numBuffers = info.numBuffers;
	SizeT size = info.size;

	// if we setup from reflection, then fetch the size from the shader
	if (info.setupFromReflection)
	{
		AnyFX::ShaderEffect* effect = shaderPool->shaderAlloc.Get<0>(info.state.shaderId);
		AnyFX::VkVarblock* varblock = static_cast<AnyFX::VkVarblock*>(effect->GetVarblock(info.name.Value()));

		// setup buffer from other buffer
		setup.reflection = varblock;
		size = varblock->alignedSize;
	}
	
	const std::array<uint32_t, 4> queues = VkRenderDevice::Instance()->GetQueueFamilies();
	setup.info =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,
		(VkDeviceSize)(size * setup.numBuffers),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_SHARING_MODE_CONCURRENT,
		(uint32_t)queues.size(),
		queues.data()
	};
	VkResult res = vkCreateBuffer(setup.dev, &setup.info, NULL, &runtime.buf);
	n_assert(res == VK_SUCCESS);

	uint32_t alignedSize;
	VkUtilities::AllocateBufferMemory(setup.dev, runtime.buf, setup.mem, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), alignedSize);

	// bind to buffer
	res = vkBindBufferMemory(setup.dev, runtime.buf, setup.mem, 0);
	n_assert(res == VK_SUCCESS);

	// size and stride for a single buffer are equal
	setup.size = alignedSize;
	setup.stride = alignedSize / setup.numBuffers;

	// map memory so we can use it later
	res = vkMapMemory(setup.dev, setup.mem, 0, setup.size, 0, &map.data);
	n_assert(res == VK_SUCCESS);

	ConstantBufferId ret;
	ret.id24 = id;
	ret.id8 = ConstantBufferIdType;

	// setup stretch callback interface
	stretch.obj = ret;
	stretch.resizer.Setup(&stretch, setup.stride, props.limits.minUniformBufferOffsetAlignment);

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyConstantBuffer(const ConstantBufferId id)
{
	VkConstantBufferRuntimeInfo& runtime = constantBufferAllocator.Get<0>(id.id24);
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<1>(id.id24);

	// unmap memory, so we can free it
	vkUnmapMemory(setup.dev, setup.mem);

	vkFreeMemory(setup.dev, setup.mem, nullptr);
	vkDestroyBuffer(setup.dev, runtime.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
ConstantBufferSliceId
ConstantBufferAllocateInstance(const ConstantBufferId id)
{
	ConstantBufferStretchInterface& stretch = constantBufferAllocator.Get<3>(id.id24);
	return stretch.resizer.AllocateInstance(1);
}

//------------------------------------------------------------------------------
/**
*/
void
ConstantBufferFreeInstance(ConstantBufferId id, ConstantBufferSliceId slice)
{
	ConstantBufferStretchInterface& stretch = constantBufferAllocator.Get<3>(id.id24);
	stretch.resizer.FreeInstance(slice.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ConstantBufferReset(const ConstantBufferId id)
{
	ConstantBufferStretchInterface& stretch = constantBufferAllocator.Get<3>(id.id24);
	stretch.resizer.SetEmpty();
}

//------------------------------------------------------------------------------
/**
*/
void
ConstantBufferUpdate(const ConstantBufferId id, const void* data, const uint offset, const uint size)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<2>(id.id24);
#if NEBULA_DEBUG
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<1>(id.id24);
	n_assert(size + offset <= (uint)setup.size);
#endif
	byte* buf = (byte*)map.data + offset + map.baseOffset;
	memcpy(buf, data, size);
}

//------------------------------------------------------------------------------
/**
*/
void
ConstantBufferArrayUpdate(const ConstantBufferId id, const void* data, const uint offset, const uint size, const uint count)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<2>(id.id24);
#if NEBULA_DEBUG
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<1>(id.id24);
	n_assert(size + offset <= (uint)setup.size);
#endif
	byte* buf = (byte*)map.data + offset + map.baseOffset;
	memcpy(buf, data, size * count);
}

//------------------------------------------------------------------------------
/**
*/
void
ConstantBufferSetBaseOffset(const ConstantBufferId id, const uint offset)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<2>(id.id24);
	map.baseOffset = offset;
}

//------------------------------------------------------------------------------
/**
*/
const ShaderConstantId
ConstantBufferCreateShaderVariable(const ConstantBufferId id, const ConstantBufferSliceId slice, const Util::StringAtom& name)
{
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<1>(id.id24);
	VkShaderConstantAllocator& alloc = constantBufferAllocator.Get<4>(id.id24);

	Ids::Id24 var = alloc.AllocObject();
	auto it = setup.reflection->variablesByName.find(name.Value());
	n_assert(it != setup.reflection->variablesByName.end());

	// step one, setup variable
	VkShaderConstantSetup((AnyFX::VkVariable*)it->second, var, alloc);

	// step two, bind to uniform buffer
	VkShaderConstantBindToUniformBuffer(var, id, alloc, setup.reflection->offsetsByName[name.Value()], it->second->byteSize, (int8_t*)it->second->currentValue);
	
	return var;
}

//------------------------------------------------------------------------------
/**
*/
void
ConstantBufferDestroyShaderVariable(const ConstantBufferId id, const ShaderConstantId var)
{
	VkShaderConstantAllocator& alloc = constantBufferAllocator.Get<4>(id.id24);
	alloc.DeallocObject(var.id);
}

} // namespace CoreGraphics