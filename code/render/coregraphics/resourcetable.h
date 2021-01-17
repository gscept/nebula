#pragma once
//------------------------------------------------------------------------------
/**
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
    CoreGraphics::TextureId tex;
    IndexT slot;
    IndexT index;
    CoreGraphics::SamplerId sampler;
    bool isDepth : 1;
    bool isStencil : 1;
};

struct ResourceTableTextureView
{
    CoreGraphics::TextureViewId tex;
    IndexT slot;
    IndexT index;
    CoreGraphics::SamplerId sampler;
    bool isDepth : 1;
    bool isStencil : 1;
};

struct ResourceTableBuffer
{
    CoreGraphics::BufferId buf;
    IndexT slot;
    IndexT index;

    bool texelBuffer;
    bool dynamicOffset;

    SizeT size;
    SizeT offset;
};

struct ResourceTableInputAttachment
{
    CoreGraphics::TextureViewId tex;
    IndexT slot;
    IndexT index;
    CoreGraphics::SamplerId sampler;
    bool isDepth : 1;
};

struct ResourceTableSampler
{
    CoreGraphics::SamplerId samp;
    IndexT slot;
};

struct ResourceTableCreateInfo
{
    ResourceTableLayoutId layout;
};

/// create resource table
ResourceTableId CreateResourceTable(const ResourceTableCreateInfo& info);
/// destroy resource table
void DestroyResourceTable(const ResourceTableId id);

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
/// copy resources from a slot, index and array size between resource tables
void ResourceTableCopy(const ResourceTableId from, const IndexT fromSlot, const IndexT fromIndex, const ResourceTableId to, const IndexT toSlot, const IndexT toIndex, const SizeT numResources);

/// disallow the resource table system to make modifications
void ResourceTableBlock(bool b);
/// apply updates of previous sets
void ResourceTableCommitChanges(const ResourceTableId id);

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
