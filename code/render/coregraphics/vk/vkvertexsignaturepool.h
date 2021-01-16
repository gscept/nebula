#pragma once
//------------------------------------------------------------------------------
/**
    Implements a pool of vertex signatures (vertex shader input layouts) for Vulkan
    
    (C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcememorypool.h"
#include "util/hashtable.h"
#include "resources/resourceid.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/config.h"

namespace CoreGraphics
{
    void SetVertexLayout(const CoreGraphics::VertexLayoutId& vl);
}

namespace Vulkan
{
class VkVertexSignaturePool : public Resources::ResourceMemoryPool
{
    __DeclareClass(VkVertexSignaturePool);
public:

    /// constructor
    VkVertexSignaturePool();
    /// destructor
    virtual ~VkVertexSignaturePool();

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


    /// update resource
    LoadStatus LoadFromMemory(const Resources::ResourceId id, const void* info);
    /// unload resource
    void Unload(const Resources::ResourceId id);

    /// get byte size
    const SizeT GetVertexLayoutSize(const CoreGraphics::VertexLayoutId id);
    /// get components
    const Util::Array<CoreGraphics::VertexComponent>& GetVertexComponents(const CoreGraphics::VertexLayoutId id);
    /// get derivative
    VkPipelineVertexInputStateCreateInfo* GetDerivativeLayout(const CoreGraphics::VertexLayoutId layout, const CoreGraphics::ShaderProgramId shader);
private:
    friend class VkMemoryVertexBufferPool;
    friend class VertexLayout;

    friend void CoreGraphics::SetVertexLayout(const CoreGraphics::VertexLayoutId& vl);

    Ids::IdAllocator<
        Util::HashTable<uint64_t, DerivativeLayout>,        //0 program-to-derivative layout binding
        VkPipelineVertexInputStateCreateInfo,               //1 base vertex input state
        BindInfo,                                           //2 setup info
        CoreGraphics::VertexLayoutInfo                      //3 base info
    > allocator;
    __ImplementResourceAllocatorTyped(allocator, CoreGraphics::VertexLayoutIdType);
};
} // namespace Vulkan
