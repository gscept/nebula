//------------------------------------------------------------------------------
//  @file vkvertexlayout.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkvertexlayout.h"
#include "vkshader.h"
#include "vktypes.h"

namespace Vulkan
{

VkVertexLayoutAllocator vertexLayoutAllocator;
static Threading::CriticalSection vertexSignatureMutex;

//------------------------------------------------------------------------------
/**
*/
VkPipelineVertexInputStateCreateInfo*
VertexLayoutGetDerivative(const CoreGraphics::VertexLayoutId layout, const CoreGraphics::ShaderProgramId shader)
{
    Threading::CriticalScope scope(&vertexSignatureMutex);
    Util::HashTable<uint64_t, DerivativeLayout>& hashTable = vertexLayoutAllocator.Get<VertexSignature_ProgramLayoutMapping>(layout.resourceId);
    const Ids::Id64 shaderHash = shader.HashCode64();

    IndexT i = hashTable.FindIndex(shaderHash);
    if (i != InvalidIndex)
    {
        return &hashTable.ValueAtIndex(shaderHash, i).info;
    }
    else
    {
        const VkProgramReflectionInfo& program = ShaderGetProgramReflection(shader);
        const BindInfo& bindInfo = vertexLayoutAllocator.Get<VertexSignature_BindInfo>(layout.resourceId);
        const VkPipelineVertexInputStateCreateInfo& baseInfo = vertexLayoutAllocator.Get<VertexSignature_VkPipelineInfo>(layout.resourceId);

        IndexT index = hashTable.Add(shaderHash, {});
        DerivativeLayout& layout = hashTable.ValueAtIndex(shaderHash, index);
        layout.info = baseInfo;

        uint32_t i;
        IndexT j;
        for (i = 0; i < program.vsInputSlots.Size(); i++)
        {
            uint32_t slot = program.vsInputSlots[i];
            for (j = 0; j < bindInfo.attrs.Size(); j++)
            {
                VkVertexInputAttributeDescription attr = bindInfo.attrs[j];
                if (attr.location == slot)
                {
                    layout.attrs.Append(attr);
                    break;
                }
            }
        }

        if (program.vsInputSlots.Size() != (uint32_t)layout.attrs.Size())
            n_warning("Warning: Vertex shader (%s) and vertex layout mismatch!\n", program.name.Value());

        layout.info.vertexAttributeDescriptionCount = layout.attrs.Size();
        layout.info.pVertexAttributeDescriptions = layout.attrs.Begin();
        return &layout.info;
    }
}

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutVkBindInfo&
VertexLayoutGetVkBindInfo(const CoreGraphics::VertexLayoutId layout)
{
    Threading::CriticalScope scope(&vertexSignatureMutex);
    return vertexLayoutAllocator.Get<VertexSignature_DynamicBindInfo>(layout.resourceId);
}

} // namespace Vulkan
namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutId
CreateVertexLayout(const VertexLayoutCreateInfo& info)
{
    Ids::Id32 id = vertexLayoutAllocator.Alloc();

    VertexLayoutInfo loadInfo;

    Util::String sig;
    IndexT i;
    SizeT size = 0;
    for (i = 0; i < info.comps.Size(); i++)
    {
        sig.Append(info.comps[i].GetSignature());
        info.comps[i].byteOffset += size;
        size += info.comps[i].GetByteSize();
    }
    sig = Util::String::Sprintf("%s", sig.AsCharPtr());
    Util::StringAtom atom(sig);

    loadInfo.signature = Util::StringAtom(sig);
    loadInfo.vertexByteSize = size;
    loadInfo.shader = Ids::InvalidId64;
    loadInfo.comps = info.comps;
    vertexLayoutAllocator.Set<VertexSignature_LayoutInfo>(id, loadInfo);

    Util::HashTable<uint64_t, DerivativeLayout>& hashTable = vertexLayoutAllocator.Get<VertexSignature_ProgramLayoutMapping>(id);
    VkPipelineVertexInputStateCreateInfo& vertexInfo = vertexLayoutAllocator.Get<VertexSignature_VkPipelineInfo>(id);
    BindInfo& bindInfo = vertexLayoutAllocator.Get<VertexSignature_BindInfo>(id);
    VertexLayoutVkBindInfo& dynamicBindInfo = vertexLayoutAllocator.Get<VertexSignature_DynamicBindInfo>(id);

    // create binds
    bindInfo.binds.Resize(CoreGraphics::MaxNumVertexStreams);
    bindInfo.binds.Fill(VkVertexInputBindingDescription{});
    bindInfo.attrs.Resize(loadInfo.comps.Size());

    dynamicBindInfo.binds.Resize(CoreGraphics::MaxNumVertexStreams);
    for (auto& bind : dynamicBindInfo.binds)
    {
        bind.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
        bind.pNext = nullptr;
        bind.binding = 0xFFFFFFFF;
        bind.stride = 0xFFFFFFFF;
        bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bind.divisor = 1;
    }
    dynamicBindInfo.attrs.Resize(loadInfo.comps.Size());

    SizeT strides[CoreGraphics::MaxNumVertexStreams] = { 0 };

    uint32_t numUsedStreams = 0;
    IndexT curOffset[CoreGraphics::MaxNumVertexStreams];
    bool usedStreams[CoreGraphics::MaxNumVertexStreams];
    Memory::Fill(curOffset, CoreGraphics::MaxNumVertexStreams * sizeof(IndexT), 0);
    Memory::Fill(usedStreams, CoreGraphics::MaxNumVertexStreams * sizeof(bool), 0);
    Util::Array<SizeT> streamSizes;

    IndexT compIndex;
    for (compIndex = 0; compIndex < loadInfo.comps.Size(); compIndex++)
    {
        const CoreGraphics::VertexComponent& component = loadInfo.comps[compIndex];
        VkVertexInputAttributeDescription* attr = &bindInfo.attrs[compIndex];
        attr->location = component.GetIndex();
        attr->binding = component.GetStreamIndex();
        attr->format = VkTypes::AsVkVertexType(component.GetFormat());
        attr->offset = curOffset[component.GetStreamIndex()];

        VkVertexInputAttributeDescription2EXT* attr2 = &dynamicBindInfo.attrs[compIndex];
        attr2->sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
        attr2->pNext = nullptr;
        attr2->location = component.GetIndex();
        attr2->binding = component.GetStreamIndex();
        attr2->format = VkTypes::AsVkVertexType(component.GetFormat());
        attr2->offset = curOffset[component.GetStreamIndex()];

        if (usedStreams[attr->binding])
        {
            bindInfo.binds[attr->binding].stride += component.GetByteSize();
            dynamicBindInfo.binds[attr->binding].stride += component.GetByteSize();
            streamSizes[attr->binding] += component.GetByteSize();
        }
        else
        {
            bindInfo.binds[attr->binding].stride = component.GetByteSize();
            dynamicBindInfo.binds[attr->binding].stride = component.GetByteSize();
            usedStreams[attr->binding] = true;
            streamSizes.Append(component.GetByteSize());
            numUsedStreams++;
        }

        bindInfo.binds[attr->binding].binding = component.GetStreamIndex();
        bindInfo.binds[attr->binding].inputRate = component.GetStrideType() == CoreGraphics::VertexComponent::PerVertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
        dynamicBindInfo.binds[attr->binding].binding = component.GetStreamIndex();
        dynamicBindInfo.binds[attr->binding].inputRate = component.GetStrideType() == CoreGraphics::VertexComponent::PerVertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
        curOffset[component.GetStreamIndex()] += component.GetByteSize();
    }
    vertexLayoutAllocator.Set<VertexSignature_StreamSize>(id, streamSizes);
    dynamicBindInfo.binds.Resize(numUsedStreams);

    vertexInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        NULL,
        0,
        numUsedStreams,
        bindInfo.binds.Begin(),
        (uint32_t)bindInfo.attrs.Size(),
        bindInfo.attrs.Begin()
    };

    VertexLayoutId ret;
    ret.resourceId = id;
    ret.resourceType = VertexLayoutIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyVertexLayout(const VertexLayoutId id)
{
    vertexLayoutAllocator.Dealloc(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VertexLayoutGetSize(const VertexLayoutId id)
{
    return vertexLayoutAllocator.Get<VertexSignature_LayoutInfo>(id.resourceId).vertexByteSize;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VertexLayoutGetStreamSize(const VertexLayoutId id, IndexT stream)
{
    return vertexLayoutAllocator.Get<VertexSignature_StreamSize>(id.resourceId)[stream];
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<VertexComponent>&
VertexLayoutGetComponents(const VertexLayoutId id)
{
    return vertexLayoutAllocator.Get<VertexSignature_LayoutInfo>(id.resourceId).comps;
}

} // namespace Vulkan
