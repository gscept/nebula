#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of vertex layout

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "util/hashtable.h"
namespace Vulkan
{

/// Get derivative of vertex layout based on shader
VkPipelineVertexInputStateCreateInfo* VertexLayoutGetDerivative(const CoreGraphics::VertexLayoutId layout, const CoreGraphics::ShaderProgramId shader);

struct DerivativeLayout
{
    VkPipelineVertexInputStateCreateInfo info;
    Util::Array<VkVertexInputAttributeDescription> attrs;

    DerivativeLayout()
    {

    }
    DerivativeLayout(const DerivativeLayout& rhs)
    {
        this->info = rhs.info;
        this->attrs = rhs.attrs;
        this->info.vertexAttributeDescriptionCount = this->attrs.Size();
        this->info.pVertexAttributeDescriptions = this->attrs.Begin();
    }
    DerivativeLayout(DerivativeLayout&& rhs)
    {
        this->info = rhs.info;
        this->attrs = std::move(rhs.attrs);
        this->info.vertexAttributeDescriptionCount = this->attrs.Size();
        this->info.pVertexAttributeDescriptions = this->attrs.Begin();
    }

    void operator=(const DerivativeLayout& rhs)
    {
        this->info = rhs.info;
        this->attrs = rhs.attrs;
        this->info.vertexAttributeDescriptionCount = this->attrs.Size();
        this->info.pVertexAttributeDescriptions = this->attrs.Begin();
    }

    void operator=(DerivativeLayout&& rhs)
    {
        this->info = rhs.info;
        this->attrs = std::move(rhs.attrs);
        this->info.vertexAttributeDescriptionCount = this->attrs.Size();
        this->info.pVertexAttributeDescriptions = this->attrs.Begin();
    }
};

struct BindInfo
{
    Util::FixedArray<VkVertexInputBindingDescription> binds;
    Util::FixedArray<VkVertexInputAttributeDescription> attrs;
};

enum
{
    VertexSignature_ProgramLayoutMapping
    , VertexSignature_VkPipelineInfo
    , VertexSignature_BindInfo
    , VertexSignature_LayoutInfo
};


typedef Ids::IdAllocator<
    Util::HashTable<uint64_t, DerivativeLayout>,        //0 program-to-derivative layout binding
    VkPipelineVertexInputStateCreateInfo,               //1 base vertex input state
    BindInfo,                                           //2 setup info
    CoreGraphics::VertexLayoutInfo                      //3 base info
> VkVertexLayoutAllocator;
extern VkVertexLayoutAllocator vertexLayoutAllocator;
} // namespace Vulkan
