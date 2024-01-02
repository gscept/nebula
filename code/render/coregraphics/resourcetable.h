#pragma once
//------------------------------------------------------------------------------
/**
    @file coregraphics/resourcetable.h
    
    A resource table declares a list of resources (ResourceTable in DX12, DescriptorSet in Vulkan)

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file 
*/
//------------------------------------------------------------------------------

#include "ids/id.h"
#include "ids/idpool.h"
#include "texture.h"
#include "textureview.h"
#include "buffer.h"
#include "sampler.h"
#include "accelerationstructure.h"
#include "coregraphics/config.h"
namespace CoreGraphics
{


ID_24_8_TYPE(ResourceTableLayoutId);

struct ResourceTableLayoutTexture
{
    IndexT slot;
    SizeT num;
    CoreGraphics::ShaderVisibility visibility;
    CoreGraphics::SamplerId immutableSampler;
};

struct ResourceTableLayoutConstantBuffer
{
    IndexT slot;
    SizeT num;
    CoreGraphics::ShaderVisibility visibility;
    
    bool dynamicOffset;
};

struct ResourceTableLayoutShaderRWBuffer
{
    IndexT slot;
    SizeT num;
    CoreGraphics::ShaderVisibility visibility;

    bool dynamicOffset;
};

struct ResourceTableLayoutAccelerationStructure
{
    IndexT slot;
    SizeT num;
    CoreGraphics::ShaderVisibility visibility;
};

struct ResourceTableLayoutSampler
{
    IndexT slot;
    CoreGraphics::ShaderVisibility visibility;
    CoreGraphics::SamplerId sampler;
};

struct ResourceTableLayoutInputAttachment
{
    IndexT slot;
    SizeT num;
    CoreGraphics::ShaderVisibility visibility;
};

struct ResourceTableLayoutCreateInfo
{
    Util::Array<ResourceTableLayoutTexture> textures;
    Util::Array<ResourceTableLayoutTexture> rwTextures;
    Util::Array<ResourceTableLayoutConstantBuffer> constantBuffers;
    Util::Array<ResourceTableLayoutShaderRWBuffer> rwBuffers;
    Util::Array<ResourceTableLayoutAccelerationStructure> accelerationStructures;
    Util::Array<ResourceTableLayoutSampler> samplers;
    Util::Array<ResourceTableLayoutInputAttachment> inputAttachments;
    uint32_t descriptorPoolInitialGrow = 1;
};

/// create resource table layout
ResourceTableLayoutId CreateResourceTableLayout(const ResourceTableLayoutCreateInfo& info);
/// destroy resource table layout
void DestroyResourceTableLayout(const ResourceTableLayoutId& id);

//------------------------------------------------------------------------------
/**
*/

ID_24_8_TYPE(ResourceTableId);

extern Util::Array<CoreGraphics::ResourceTableId> PendingTableCommits;
extern bool ResourceTableBlocked;
extern Threading::CriticalSection PendingTableCommitsLock;

struct ResourceTableTexture
{
    ResourceTableTexture()
        : tex(InvalidTextureId)
        , slot(0)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(false)
        , isStencil(false)
    {};

    ResourceTableTexture(const CoreGraphics::TextureId tex, IndexT slot)
        : tex(tex)
        , slot(slot)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(false)
        , isStencil(false)
    {};

    ResourceTableTexture(const CoreGraphics::TextureId tex, IndexT slot, IndexT index)
        : tex(tex)
        , slot(slot)
        , index(index)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(false)
        , isStencil(false)
    {};

    ResourceTableTexture(const CoreGraphics::TextureId tex, IndexT slot, bool isDepth, bool isStencil)
        : tex(tex)
        , slot(slot)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(isDepth)
        , isStencil(isStencil)
    {};

    ResourceTableTexture(const CoreGraphics::TextureId tex, IndexT slot, IndexT index, CoreGraphics::SamplerId sampler, bool isDepth = false, bool isStencil = false)
        : tex(tex)
        , slot(slot)
        , index(index)
        , sampler(sampler)
        , isDepth(isDepth)
        , isStencil(isStencil)
    {};

    CoreGraphics::TextureId tex;
    IndexT slot;
    IndexT index;
    CoreGraphics::SamplerId sampler;
    bool isDepth : 1;
    bool isStencil : 1;
};

struct ResourceTableTextureView
{
    ResourceTableTextureView()
        : tex(InvalidTextureViewId)
        , slot(0)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(false)
        , isStencil(false)
    {};

    ResourceTableTextureView(const CoreGraphics::TextureViewId tex, IndexT slot)
        : tex(tex)
        , slot(slot)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(false)
        , isStencil(false)
    {};

    ResourceTableTextureView(const CoreGraphics::TextureViewId tex, IndexT slot, IndexT index)
        : tex(tex)
        , slot(slot)
        , index(index)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(false)
        , isStencil(false)
    {};

    ResourceTableTextureView(const CoreGraphics::TextureViewId tex, IndexT slot, bool isDepth = false, bool isStencil = false)
        : tex(tex)
        , slot(slot)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(isDepth)
        , isStencil(isStencil)
    {};

    ResourceTableTextureView(const CoreGraphics::TextureViewId tex, IndexT slot, IndexT index, CoreGraphics::SamplerId sampler, bool isDepth = false, bool isStencil = false)
        : tex(tex)
        , slot(slot)
        , index(index)
        , sampler(sampler)
        , isDepth(isDepth)
        , isStencil(isStencil)
    {};

    CoreGraphics::TextureViewId tex;
    IndexT slot;
    IndexT index;
    CoreGraphics::SamplerId sampler;
    bool isDepth : 1;
    bool isStencil : 1;
};

struct ResourceTableInputAttachment
{
    ResourceTableInputAttachment()
        : tex(InvalidTextureViewId)
        , slot(0)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(false)
    {};

    ResourceTableInputAttachment(const CoreGraphics::TextureViewId tex, IndexT slot)
        : tex(tex)
        , slot(slot)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(false)
    {};

    ResourceTableInputAttachment(const CoreGraphics::TextureViewId tex, IndexT slot, bool isDepth = false)
        : tex(tex)
        , slot(slot)
        , index(0)
        , sampler(CoreGraphics::InvalidSamplerId)
        , isDepth(isDepth)
    {};

    ResourceTableInputAttachment(const CoreGraphics::TextureViewId tex, IndexT slot, IndexT index, CoreGraphics::SamplerId sampler, bool isDepth = false)
        : tex(tex)
        , slot(slot)
        , index(index)
        , sampler(sampler)
        , isDepth(isDepth)
    {};

    CoreGraphics::TextureViewId tex;
    IndexT slot;
    IndexT index;
    CoreGraphics::SamplerId sampler;
    bool isDepth : 1;
};

struct ResourceTableBuffer
{
    ResourceTableBuffer()
        : buf(InvalidBufferId)
        , slot(0)
        , index(0)
        , texelBuffer(false)
        , dynamicOffset(false)
        , size(NEBULA_WHOLE_BUFFER_SIZE)
        , offset(0)
    {};

    ResourceTableBuffer(const CoreGraphics::BufferId buf, IndexT slot)
        : buf(buf)
        , slot(slot)
        , index(0)
        , texelBuffer(false)
        , dynamicOffset(false)
        , size(NEBULA_WHOLE_BUFFER_SIZE)
        , offset(0)
    {};

    ResourceTableBuffer(const CoreGraphics::BufferId buf, IndexT slot, SizeT size)
        : buf(buf)
        , slot(slot)
        , index(0)
        , texelBuffer(false)
        , dynamicOffset(false)
        , size(size)
        , offset(0)
    {};

    ResourceTableBuffer(const CoreGraphics::BufferId buf, IndexT slot, SizeT size, SizeT offset)
        : buf(buf)
        , slot(slot)
        , index(0)
        , texelBuffer(false)
        , dynamicOffset(false)
        , size(size)
        , offset(offset)
    {};

    ResourceTableBuffer(const CoreGraphics::BufferId buf, IndexT slot, SizeT index, SizeT size, SizeT offset, bool texelBuffer = false, bool dynamicOffset = false)
        : buf(buf)
        , slot(slot)
        , index(index)
        , texelBuffer(texelBuffer)
        , dynamicOffset(dynamicOffset)
        , size(size)
        , offset(offset)
    {};

    CoreGraphics::BufferId buf;
    IndexT slot;
    IndexT index;

    SizeT size;
    SizeT offset;

    bool texelBuffer;
    bool dynamicOffset;
};

struct ResourceTableSampler
{
    CoreGraphics::SamplerId samp;
    IndexT slot;
};

struct ResourceTableTlas
{
    ResourceTableTlas(const CoreGraphics::TlasId tlas, IndexT slot)
        : tlas(tlas)
        , slot(slot)
    {};
    CoreGraphics::TlasId tlas;
    IndexT slot;
};

struct ResourceTableCreateInfo
{
    ResourceTableLayoutId layout;
    uint overallocationSize = 256;
};

/// create resource table
ResourceTableId CreateResourceTable(const ResourceTableCreateInfo& info);
/// destroy resource table
void DestroyResourceTable(const ResourceTableId id);

/// Get resource table layout 
const ResourceTableLayoutId& ResourceTableGetLayout(CoreGraphics::ResourceTableId id);

/// set resource table texture
void ResourceTableSetTexture(const ResourceTableId id, const ResourceTableTexture& tex);
/// set resource table texture
void ResourceTableSetTexture(const ResourceTableId id, const ResourceTableTextureView& tex);
/// set resource table input attachment
void ResourceTableSetInputAttachment(const ResourceTableId id, const ResourceTableInputAttachment& tex);
/// set resource table texture as read-write
void ResourceTableSetRWTexture(const ResourceTableId id, const ResourceTableTexture& tex);
/// set resource table texture as read-write
void ResourceTableSetRWTexture(const ResourceTableId id, const ResourceTableTextureView& tex);
/// set resource table constant buffer
void ResourceTableSetConstantBuffer(const ResourceTableId id, const ResourceTableBuffer& buf);
/// set resource table shader rw buffer
void ResourceTableSetRWBuffer(const ResourceTableId id, const ResourceTableBuffer& buf);
/// set resource table sampler
void ResourceTableSetSampler(const ResourceTableId id, const ResourceTableSampler& samp);
/// Set resource table acceleration structure
void ResourceTableSetAccelerationStructure(const ResourceTableId id, const ResourceTableTlas& tlas);
/// copy resources from a slot, index and array size between resource tables
void ResourceTableCopy(const ResourceTableId from, const IndexT fromSlot, const IndexT fromIndex, const ResourceTableId to, const IndexT toSlot, const IndexT toIndex, const SizeT numResources);

/// disallow the resource table system to make modifications
void ResourceTableBlock(bool b);
/// apply updates of previous sets
void ResourceTableCommitChanges(const ResourceTableId id);

//------------------------------------------------------------------------------
/**
    Set of buffers which creates a resource table per each buffered frame
*/
struct ResourceTableSet
{
    /// Default constructor
    ResourceTableSet() {};
    /// Constructor
    ResourceTableSet(const ResourceTableCreateInfo& createInfo);
    /// Move constructor
    ResourceTableSet(ResourceTableSet&& rhs);
    /// Destructor
    ~ResourceTableSet();

    /// Run a for each function per table
    void ForEach(std::function<void(const ResourceTableId, const IndexT)> func);

    /// Move assignment
    void operator=(ResourceTableSet&& rhs);

    /// Get buffer for this frame
    const CoreGraphics::ResourceTableId Get();

    Util::FixedArray<CoreGraphics::ResourceTableId> tables;
};

//------------------------------------------------------------------------------
/**
*/

ID_24_8_TYPE(ResourcePipelineId);

struct ResourcePipelinePushConstantRange
{
    SizeT size;
    SizeT offset;
    CoreGraphics::ShaderVisibility vis;
};

struct ResourcePipelineCreateInfo
{
    Util::Array<ResourceTableLayoutId> tables;
    Util::Array<uint32_t> indices;
    ResourcePipelinePushConstantRange push;
};

/// create resource pipeline
ResourcePipelineId CreateResourcePipeline(const ResourcePipelineCreateInfo& info);
/// destroy resource pipeline
void DestroyResourcePipeline(const ResourcePipelineId& id);

} // namespace CoreGraphics
