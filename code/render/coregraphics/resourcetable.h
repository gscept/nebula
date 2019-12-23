#pragma once
//------------------------------------------------------------------------------
/**
	A resource table declares a list of resources (ResourceTable in DX12, DescriptorSet in Vulkan)

	(C) 2018 Individual contributors, see AUTHORS file	
*/
//------------------------------------------------------------------------------

#include "ids/id.h"
#include "ids/idpool.h"
#include "texture.h"
#include "shaderrwbuffer.h"
#include "constantbuffer.h"
#include "sampler.h"
#include "config.h"
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

struct ResourceTableTexture
{
	CoreGraphics::TextureId tex;
	IndexT slot;
	IndexT index;
	CoreGraphics::SamplerId sampler;
	bool isDepth : 1;
};

struct ResourceTableConstantBuffer
{
	CoreGraphics::ConstantBufferId buf;
	IndexT slot;
	IndexT index;
	bool dynamicOffset;
	bool texelBuffer;

	SizeT size;
	SizeT offset;
};

struct ResourceTableShaderRWBuffer
{
	CoreGraphics::ShaderRWBufferId buf;
	IndexT slot;
	IndexT index;
	bool dynamicOffset;
	bool texelBuffer;

	SizeT size;
	SizeT offset;
};

struct ResourceTableInputAttachment
{
	CoreGraphics::TextureId tex;
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
void DestroyResourceTable(const ResourceTableId& id);


/// set resource table texture
void ResourceTableSetTexture(const ResourceTableId& id, const ResourceTableTexture& tex);
/// set resource table input attachment
void ResourceTableSetInputAttachment(const ResourceTableId& id, const ResourceTableInputAttachment& tex);
/// set resource table texture as read-write
void ResourceTableSetRWTexture(const ResourceTableId& id, const ResourceTableTexture& tex);
/// set resource table constant buffer
void ResourceTableSetConstantBuffer(const ResourceTableId& id, const ResourceTableConstantBuffer& buf);
/// set resource table shader rw buffer
void ResourceTableSetRWBuffer(const ResourceTableId& id, const ResourceTableShaderRWBuffer& buf);
/// set resource table sampler
void ResourceTableSetSampler(const ResourceTableId& id, const ResourceTableSampler& samp);

/// apply updates of previous sets
void ResourceTableCommitChanges(const ResourceTableId& id);

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
