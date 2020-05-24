//------------------------------------------------------------------------------
// vkstreamtexturepool.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkstreamtexturepool.h"
#include "coregraphics/texture.h"
#include "io/ioserver.h"
#include "coregraphics/vk/vktypes.h"
#include "coregraphics/load/glimltypes.h"

#include "vkloader.h"
#include "vkgraphicsdevice.h"
#include "vkutilities.h"
#include "math/scalar.h"
#include "vkshaderserver.h"
#include "coregraphics/memorytexturepool.h"
#include "vksubmissioncontext.h"
#include "profiling/profiling.h"
#include "threading/interlocked.h"
namespace Vulkan
{

__ImplementClass(Vulkan::VkStreamTexturePool, 'VKTL', Resources::ResourceStreamPool);

using namespace CoreGraphics;
using namespace Resources;
using namespace IO;
//------------------------------------------------------------------------------
/**
*/
VkStreamTexturePool::VkStreamTexturePool()
{
	this->async = true;
	this->placeholderResourceName = "tex:system/white.dds";
	this->failResourceName = "tex:system/error.dds";

	this->streamerThreadName = "Texture Pool Streamer Thread";
}

//------------------------------------------------------------------------------
/**
*/
VkStreamTexturePool::~VkStreamTexturePool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline Resources::ResourceUnknownId
VkStreamTexturePool::AllocObject()
{
	return texturePool->AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamTexturePool::DeallocObject(const Resources::ResourceUnknownId id)
{
	texturePool->DeallocObject(id);
}

//------------------------------------------------------------------------------
/**
*/
ResourcePool::LoadStatus
VkStreamTexturePool::LoadFromStream(const Resources::ResourceId res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
	N_SCOPE_ACCUM(CreateAndLoad, TextureStream);
	n_assert(stream.isvalid());
	n_assert(stream->CanBeMapped());
	n_assert(this->GetState(res) == Resources::Resource::Pending);

	void* srcData = stream->MemoryMap();
	uint srcDataSize = stream->GetSize();

	static const int NumBasicLods = 5;

	// load using gliml
	gliml::context ctx;
	if (ctx.load_dds(srcData, srcDataSize))
	{
		// during the load-phase, we can safetly get the structs
		texturePool->EnterGet();
		VkTextureRuntimeInfo& runtimeInfo = texturePool->Get<Texture_RuntimeInfo>(res.resourceId);
		VkTextureLoadInfo& loadInfo = texturePool->Get<Texture_LoadInfo>(res.resourceId);
		VkTextureStreamInfo& streamInfo = texturePool->Get<Texture_StreamInfo>(res.resourceId);

		streamInfo.mappedBuffer = srcData;
		streamInfo.bufferSize = srcDataSize;
		streamInfo.stream = stream;
		streamInfo.ctx = ctx;

		loadInfo.dev = Vulkan::GetCurrentDevice();
		texturePool->LeaveGet();

		VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
		VkDevice dev = Vulkan::GetCurrentDevice();

	
		// FIXME uses the values from the first face in the image for everything. cubemaps with differing resolutions will break
		int numMips = ctx.num_mipmaps(0);
		int mips = numMips - NumBasicLods;
		int depth = ctx.image_depth(0, 0);
		int width = ctx.image_width(0, 0);
		int height = ctx.image_height(0, 0);
		bool isCube = ctx.num_faces() > 1;

		streamInfo.lowestLod = Math::n_max(0, numMips - NumBasicLods);

		CoreGraphics::PixelFormat::Code nebulaFormat = CoreGraphics::Gliml::ToPixelFormat(ctx);
		VkFormat vkformat = VkTypes::AsVkFormat(nebulaFormat);
		VkTypes::VkBlockDimensions block = VkTypes::AsVkBlockSize(vkformat);

		runtimeInfo.type = isCube ? CoreGraphics::TextureCube : ctx.is_3d() ? CoreGraphics::Texture3D : CoreGraphics::Texture2D;

		// use linear if we really have to
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDev, vkformat, &formatProps);
		bool forceLinear = false;
		if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
		{
			forceLinear = true;
		}

		// create image
		VkExtent3D extents;
		extents.width = width;
		extents.height = height;
		extents.depth = depth;

		VkImageCreateInfo info =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			NULL,
			isCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
			VkTypes::AsVkImageType(runtimeInfo.type),
			vkformat,
			extents,
			numMips,
			ctx.num_faces(),
			VK_SAMPLE_COUNT_1_BIT,
			forceLinear ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
		};
		VkResult stat = vkCreateImage(dev, &info, NULL, &loadInfo.img);
		n_assert(stat == VK_SUCCESS);

		CoreGraphics::Alloc alloc = AllocateMemory(loadInfo.dev, loadInfo.img, CoreGraphics::MemoryPool_DeviceLocal);
		stat = vkBindImageMemory(loadInfo.dev, loadInfo.img, alloc.mem, alloc.offset);
		n_assert(stat == VK_SUCCESS);
		loadInfo.mem = alloc;

		// create image view
		VkImageSubresourceRange subres;
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = Math::n_max(numMips - NumBasicLods, 0);
		subres.layerCount = info.arrayLayers;
		subres.levelCount = Math::n_min(numMips, NumBasicLods);
		VkImageViewCreateInfo viewCreate =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			loadInfo.img,
			VkTypes::AsVkImageViewType(runtimeInfo.type),
			vkformat,
			VkTypes::AsVkMapping(nebulaFormat),
			subres
		};
		stat = vkCreateImageView(dev, &viewCreate, NULL, &runtimeInfo.view);
		n_assert(stat == VK_SUCCESS);

		// use resource submission
		CoreGraphics::LockResourceSubmission();
		CoreGraphics::SubmissionContextId sub = CoreGraphics::GetResourceSubmissionContext();

		// transition to transfer
		VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
			CoreGraphics::BarrierStage::Host,
			CoreGraphics::BarrierStage::Transfer,
			VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

		// now load texture by walking through all images and mips
		for (int i = 0; i < ctx.num_faces(); i++)
		{
			for (int j = subres.baseMipLevel; j < ctx.num_mipmaps(i); j++)
			{
				extents.width = ctx.image_width(i, j);
				extents.height = ctx.image_height(i, j);
				extents.depth = ctx.image_depth(i, j);

				VkImageSubresourceRange res = subres;
				res.layerCount = 1;
				res.levelCount = 1;
				res.baseMipLevel = j;
				res.baseArrayLayer = subres.baseArrayLayer + i;

				VkBuffer outBuf;
				CoreGraphics::Alloc outMem;
				VkUtilities::ImageUpdate(
					dev,
					CoreGraphics::SubmissionContextGetCmdBuffer(sub),
					TransferQueueType,
					loadInfo.img,
					extents,
					res.baseMipLevel,
					res.baseArrayLayer,
					ctx.image_size(i, j),
					(uint32_t*)ctx.image_data(i, j),
					outBuf,
					outMem);

				// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
				SubmissionContextFreeMemory(sub, outMem);
				SubmissionContextFreeBuffer(sub, dev, outBuf);
			}
		}

		// transition image to be used for rendering
		VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierStage::AllGraphicsShaders,
			VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, TransferQueueType, GraphicsQueueType, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

		// perform final transition on graphics queue
		CoreGraphics::SubmissionContextId gfxSub = CoreGraphics::GetHandoverSubmissionContext();
		VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(gfxSub),
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierStage::AllGraphicsShaders,
			VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, TransferQueueType, GraphicsQueueType, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		CoreGraphics::UnlockResourceSubmission();

		loadInfo.dims.width = width;
		loadInfo.dims.height = height;
		loadInfo.dims.depth = depth;
		loadInfo.layers = info.arrayLayers;
		loadInfo.mips = Math::n_max(numMips, 1);
		loadInfo.format = nebulaFormat;// VkTypes::AsNebulaPixelFormat(vkformat);
		loadInfo.dev = dev;
		runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(TextureId(res), runtimeInfo.type);

		//stream->MemoryUnmap();

#if NEBULA_GRAPHICS_DEBUG
		ObjectSetName((TextureId)res, stream->GetURI().LocalPath().AsCharPtr());
#endif
		return ResourcePool::Success;
	}
	stream->MemoryUnmap();
	return ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamTexturePool::Unload(const Resources::ResourceId id)
{
	VkTextureStreamInfo& streamInfo = texturePool->Get<Texture_StreamInfo>(id.resourceId);
	VkTextureLoadInfo& loadInfo = texturePool->Get<Texture_LoadInfo>(id.resourceId);
	VkTextureRuntimeInfo& runtimeInfo = texturePool->Get<Texture_RuntimeInfo>(id.resourceId);
	streamInfo.stream->MemoryUnmap();
	texturePool->Unload(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkStreamTexturePool::StreamMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate)
{
	N_SCOPE_ACCUM(StreamMaxLOD, TextureStream);
	texturePool->EnterGet();
	VkTextureStreamInfo& streamInfo = texturePool->Get<Texture_StreamInfo>(id.resourceId);
	VkTextureLoadInfo& loadInfo = texturePool->Get<Texture_LoadInfo>(id.resourceId);
	VkTextureRuntimeInfo& runtimeInfo = texturePool->Get<Texture_RuntimeInfo>(id.resourceId);
	texturePool->LeaveGet();

	// if the lod is undefined, just add 1 mip
	IndexT adjustedLod = Math::n_max(0.0f, Math::n_ceil(loadInfo.mips * lod));
	

	// abort if the lod is already higher
	if (streamInfo.lowestLod <= (uint32_t)adjustedLod)
		return;

	// bump lod
	adjustedLod = Math::n_min(adjustedLod, (IndexT)loadInfo.mips);
	IndexT maxLod = loadInfo.mips - streamInfo.lowestLod;

	VkDevice dev = Vulkan::GetCurrentDevice();
	streamInfo.lowestLod = adjustedLod;

	const gliml::context& ctx = streamInfo.ctx;

	VkImageSubresourceRange subres;
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.baseArrayLayer = 0;
	subres.baseMipLevel = adjustedLod;
	subres.layerCount = loadInfo.layers;
	subres.levelCount = 1;

	// create image
	VkExtent3D extents;
	extents.width = ctx.image_width(0, 0);
	extents.height = ctx.image_height(0, 0);
	extents.depth = ctx.image_depth(0, 0);

	// create image view
	VkImageSubresourceRange viewSubres;
	viewSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewSubres.baseArrayLayer = 0;
	viewSubres.baseMipLevel = adjustedLod;
	viewSubres.layerCount = loadInfo.layers;
	viewSubres.levelCount = loadInfo.mips - viewSubres.baseMipLevel;
	VkImageViewCreateInfo viewCreate =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		loadInfo.img,
		VkTypes::AsVkImageViewType(runtimeInfo.type),
		VkTypes::AsVkFormat(loadInfo.format),
		VkTypes::AsVkMapping(loadInfo.format),
		viewSubres
	};

	// use resource submission
	CoreGraphics::LockResourceSubmission();
	CoreGraphics::SubmissionContextId sub = CoreGraphics::GetResourceSubmissionContext();
	CoreGraphics::SubmissionContextId gfxSub = CoreGraphics::GetHandoverSubmissionContext();

	// transition to transfer
	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(gfxSub),
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		CoreGraphics::BarrierStage::Transfer,
		VkUtilities::ImageMemoryBarrier(loadInfo.img, viewSubres, GraphicsQueueType, TransferQueueType, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

	// now load texture by walking through all images and mips
	for (int i = 0; i < ctx.num_faces(); i++)
	{
		for (int j = adjustedLod; j < maxLod; j++)
		{
			extents.width = ctx.image_width(i, j);
			extents.height = ctx.image_height(i, j);
			extents.depth = ctx.image_depth(i, j);

			VkImageSubresourceRange res = subres;
			res.layerCount = 1;
			res.levelCount = 1;
			res.baseMipLevel = j;
			res.baseArrayLayer = subres.baseArrayLayer + i;

			VkBuffer outBuf;
			CoreGraphics::Alloc outMem;
			VkUtilities::ImageUpdate(
				dev,
				CoreGraphics::SubmissionContextGetCmdBuffer(sub),
				TransferQueueType,
				loadInfo.img,
				extents,
				res.baseMipLevel,
				res.baseArrayLayer,
				ctx.image_size(i, j),
				(uint32_t*)ctx.image_data(i, j),
				outBuf,
				outMem);

			// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
			SubmissionContextFreeMemory(sub, outMem);
			SubmissionContextFreeBuffer(sub, dev, outBuf);
		}
	}

	// transition image to be used for rendering
	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::Transfer,
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		VkUtilities::ImageMemoryBarrier(loadInfo.img, viewSubres, TransferQueueType, GraphicsQueueType, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	// perform final transition on graphics queue
	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(gfxSub),
		CoreGraphics::BarrierStage::Transfer,
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		VkUtilities::ImageMemoryBarrier(loadInfo.img, viewSubres, TransferQueueType, GraphicsQueueType, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	CoreGraphics::UnlockResourceSubmission();

	// create new view
	//VkImageView oldView = runtimeInfo.view;
	//VkResult res = vkCreateImageView(dev, &viewCreate, nullptr, &runtimeInfo.view);
	//n_assert(res == VK_SUCCESS);

	//if (loadInfo.bindless)
	VkShaderServer::Instance()->AddPendingImageView(TextureId(id), viewCreate, runtimeInfo.bind);
	// update binding
	//if (loadInfo.bindless)
	//VkShaderServer::Instance()->ReregisterTexture(TextureId(id), runtimeInfo.type, runtimeInfo.bind);
}

} // namespace Vulkan
