//------------------------------------------------------------------------------
// vkstreamtexturepool.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkstreamtexturepool.h"
#include "coregraphics/texture.h"
#include "io/ioserver.h"
#include "coregraphics/vk/vktypes.h"
#include "IL/il.h"

#include "vkloader.h"
#include "vkgraphicsdevice.h"
#include "vkutilities.h"
#include "math/scalar.h"
#include "vkshaderserver.h"
#include "coregraphics/memorytexturepool.h"
#include "vksubmissioncontext.h"
#include "profiling/profiling.h"
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
	// empty
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
void
VkStreamTexturePool::Setup()
{
	ResourceStreamPool::Setup();
	this->async = true;
	this->placeholderResourceName = "tex:system/white.dds";
	this->failResourceName = "tex:system/error.dds";
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
	N_SCOPE(CreateAndLoad, TextureStream);
	n_assert(stream.isvalid());
	n_assert(stream->CanBeMapped());
	n_assert(this->GetState(res) == Resources::Resource::Pending);

	void* srcData = stream->MemoryMap();
	uint srcDataSize = stream->GetSize();

	static const int NumBasicLods = 5;

	// during the load-phase, we can safetly get the structs
	texturePool->EnterGet();
	VkTextureRuntimeInfo& runtimeInfo = texturePool->Get<Texture_RuntimeInfo>(res.resourceId);
	VkTextureLoadInfo& loadInfo = texturePool->Get<Texture_LoadInfo>(res.resourceId);
	VkTextureStreamInfo& streamInfo = texturePool->Get<Texture_StreamInfo>(res.resourceId);

	streamInfo.mappedBuffer = srcData;
	streamInfo.bufferSize = srcDataSize;
	streamInfo.highestLod = NumBasicLods;
	streamInfo.stream = stream;
	loadInfo.dev = Vulkan::GetCurrentDevice();
	texturePool->LeaveGet();

	VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
	VkDevice dev = Vulkan::GetCurrentDevice();

	// load using IL
	ILuint image = ilGenImage();
	ilBindImage(image);
	ilSetInteger(IL_DXTC_NO_DECOMPRESS, IL_TRUE);
	ilSetInteger(IL_DDS_FIRST_MIP, -NumBasicLods);
	ilSetInteger(IL_DDS_LAST_MIP, -1);
	ilLoadL(IL_DDS, srcData, srcDataSize);

	ILuint startWidth = ilGetInteger(IL_IMAGE_WIDTH);
	ILuint startHeight = ilGetInteger(IL_IMAGE_HEIGHT);
	ILuint startDepth = ilGetInteger(IL_IMAGE_DEPTH);
	ILuint width = ilGetInteger(IL_DDS_WIDTH_HEADER);
	ILuint height = ilGetInteger(IL_DDS_HEIGHT_HEADER);
	ILuint depth = ilGetInteger(IL_DDS_DEPTH_HEADER);
	ILuint bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	ILuint numImages = ilGetInteger(IL_NUM_IMAGES);
	ILuint numFaces = ilGetInteger(IL_NUM_FACES);
	ILuint numLayers = ilGetInteger(IL_NUM_LAYERS);
	ILuint mips = ilGetInteger(IL_NUM_MIPMAPS);
	ILuint imageMips = ilGetInteger(IL_DDS_MIP_HEADER_COUNT); // get the mip count from the header, not from the data
	ILenum cube = ilGetInteger(IL_IMAGE_CUBEFLAGS);
	ILenum format = ilGetInteger(IL_PIXEL_FORMAT);	// only available when loading DDS, so this might need some work...

	VkFormat vkformat = VkTypes::AsVkFormat(format);
	VkTypes::VkBlockDimensions block = VkTypes::AsVkBlockSize(vkformat);

	runtimeInfo.type = cube ? CoreGraphics::TextureCube : depth > 1 ? CoreGraphics::Texture3D : CoreGraphics::Texture2D;

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
		cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
		VkTypes::AsVkImageType(runtimeInfo.type),
		vkformat,
		extents,
		imageMips,
		cube ? (uint32_t)numFaces : (uint32_t)numImages,
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

	// allocate memory backing
	uint32_t alignedSize;
	VkUtilities::AllocateImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, VkMemoryPropertyFlagBits(0), alignedSize);
	vkBindImageMemory(dev, loadInfo.img, loadInfo.mem, 0);

	// create image view
	VkImageSubresourceRange subres;
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.baseArrayLayer = 0;
	subres.baseMipLevel = Math::n_max((ILint)imageMips - NumBasicLods, 0);
	subres.layerCount = info.arrayLayers;
	subres.levelCount = Math::n_min((ILint)imageMips, NumBasicLods);
	VkImageViewCreateInfo viewCreate =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		loadInfo.img,
		VkTypes::AsVkImageViewType(runtimeInfo.type),
		vkformat,
		VkTypes::AsVkMapping(format),
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
	ILuint i;
	ILuint j;
	if (cube)
	{
		for (i = 0; i < 6; i++)
		{
			ilBindImage(image);
			ilActiveFace(i);
			ilActiveMipmap(0);
			for (j = 0; j < mips; j++)
			{
				// move to next mip
				ilBindImage(image);
				ilActiveFace(i);
				ilActiveMipmap(j);

				ILuint size = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
				ILubyte* buf = ilGetData();

				int32_t mipWidth = Math::n_max(startWidth >> j, 1u);
				int32_t mipHeight = Math::n_max(startHeight >> j, 1u);
				int32_t mipDepth = Math::n_max(startDepth >> j, 1u);

				extents.width = mipWidth;
				extents.height = mipHeight;
				extents.depth = mipDepth;

				VkImageSubresourceRange res = subres;
				res.layerCount = 1;
				res.levelCount = 1;
				res.baseMipLevel = subres.baseMipLevel + j;
				res.baseArrayLayer = subres.baseArrayLayer + i;

				VkBuffer outBuf;
				VkDeviceMemory outMem;
				VkUtilities::ImageUpdate(
					dev, 
					CoreGraphics::SubmissionContextGetCmdBuffer(sub), 
					TransferQueueType, 
					loadInfo.img, 
					extents, 
					res.baseMipLevel,
					res.baseArrayLayer,
					size, 
					(uint32_t*)buf, 
					outBuf, 
					outMem);

				// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
				SubmissionContextFreeDeviceMemory(sub, dev, outMem);
				SubmissionContextFreeBuffer(sub, dev, outBuf);
			}
		}
	}
	else
	{
		for (j = 0; j < mips; j++)
		{
			// move to next mip
			ilBindImage(image);
			ilActiveMipmap(j);

			ILuint size = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
			ILubyte* buf = ilGetData();

			int32_t mipWidth = Math::n_max(startWidth >> j, 1u);
			int32_t mipHeight = Math::n_max(startHeight >> j, 1u);
			int32_t mipDepth = Math::n_max(startDepth >> j, 1u);

			extents.width = mipWidth;
			extents.height = mipHeight;
			extents.depth = mipDepth;

			VkImageSubresourceRange res = subres;
			res.layerCount = 1;
			res.levelCount = 1;
			res.baseMipLevel = subres.baseMipLevel + j;
			res.baseArrayLayer = 0;

			VkBuffer outBuf;
			VkDeviceMemory outMem;
			VkUtilities::ImageUpdate(
				dev, 
				CoreGraphics::SubmissionContextGetCmdBuffer(sub), 
				TransferQueueType, 
				loadInfo.img, 
				extents, 
				res.baseMipLevel,
				res.baseArrayLayer,
				size, 
				(uint32_t*)buf, 
				outBuf, 
				outMem);

			// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
			SubmissionContextFreeDeviceMemory(sub, dev, outMem);
			SubmissionContextFreeBuffer(sub, dev, outBuf);
		}
	}
	ilDeleteImage(image);

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
	loadInfo.mips = Math::n_max(imageMips, 1u);
	loadInfo.format = VkTypes::AsNebulaPixelFormat(vkformat);
	loadInfo.dev = dev;
	runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(TextureId(res), runtimeInfo.type);

	//stream->MemoryUnmap();

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName((TextureId)res, stream->GetURI().LocalPath().AsCharPtr());
#endif

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamTexturePool::Unload(const Resources::ResourceId id)
{
	VkTextureStreamInfo& streamInfo = texturePool->Get<Texture_StreamInfo>(id.resourceId);
	streamInfo.stream->MemoryUnmap();
	texturePool->Unload(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkStreamTexturePool::UpdateLOD(const Resources::ResourceId& id, const IndexT lod, bool immediate)
{
	N_SCOPE(UpdateLOD, TextureStream);
	texturePool->EnterGet();
	VkTextureStreamInfo& streamInfo = texturePool->Get<Texture_StreamInfo>(id.resourceId);
	VkTextureLoadInfo& loadInfo = texturePool->Get<Texture_LoadInfo>(id.resourceId);
	VkTextureRuntimeInfo& runtimeInfo = texturePool->Get<Texture_RuntimeInfo>(id.resourceId);
	texturePool->LeaveGet();

	// if the lod is undefined, just add 1 mip
	IndexT adjustedLod = lod;
	if (adjustedLod == -1)
		adjustedLod = streamInfo.highestLod + 1;

	// abort if the lod is already higher
	if (streamInfo.highestLod > (uint32_t)adjustedLod)
		return;

	// bump lod
	streamInfo.highestLod = adjustedLod;

	// if we are already at the highest lod, return
	if (streamInfo.highestLod > loadInfo.mips)
		return;

	VkDevice dev = Vulkan::GetCurrentDevice();

	// load using IL
	ILuint image = ilGenImage();
	ilBindImage(image);
	ilSetInteger(IL_DXTC_NO_DECOMPRESS, IL_TRUE);
	ilSetInteger(IL_DDS_FIRST_MIP, -adjustedLod); // count lods backwards
	ilSetInteger(IL_DDS_LAST_MIP, -adjustedLod);
	ilLoadL(IL_DDS, streamInfo.mappedBuffer, streamInfo.bufferSize);

	ILuint startWidth = ilGetInteger(IL_IMAGE_WIDTH);
	ILuint startHeight = ilGetInteger(IL_IMAGE_HEIGHT);
	ILuint startDepth = ilGetInteger(IL_IMAGE_DEPTH);
	ILuint width = ilGetInteger(IL_DDS_WIDTH_HEADER);
	ILuint height = ilGetInteger(IL_DDS_HEIGHT_HEADER);
	ILuint depth = ilGetInteger(IL_DDS_DEPTH_HEADER);
	ILuint bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	ILuint numImages = ilGetInteger(IL_NUM_IMAGES);
	ILuint numFaces = ilGetInteger(IL_NUM_FACES);
	ILuint numLayers = ilGetInteger(IL_NUM_LAYERS);
	ILuint mips = ilGetInteger(IL_NUM_MIPMAPS);
	ILuint imageMips = ilGetInteger(IL_DDS_MIP_HEADER_COUNT); // get the mip count from the header, not from the data
	ILenum cube = ilGetInteger(IL_IMAGE_CUBEFLAGS);
	ILenum format = ilGetInteger(IL_PIXEL_FORMAT);	// only available when loading DDS, so this might need some work...

	VkImageSubresourceRange subres;
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.baseArrayLayer = 0;
	subres.baseMipLevel = Math::n_max((ILint)imageMips - adjustedLod, 0);
	subres.layerCount = loadInfo.layers;
	subres.levelCount = 1;

	// create image
	VkExtent3D extents;
	extents.width = width;
	extents.height = height;
	extents.depth = depth;

	// create image view
	VkImageSubresourceRange viewSubres;
	viewSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewSubres.baseArrayLayer = 0;
	viewSubres.baseMipLevel = Math::n_max((ILint)imageMips - adjustedLod, 0);
	viewSubres.layerCount = loadInfo.layers;
	viewSubres.levelCount = imageMips - viewSubres.baseMipLevel;
	VkImageViewCreateInfo viewCreate =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		loadInfo.img,
		VkTypes::AsVkImageViewType(runtimeInfo.type),
		VkTypes::AsVkFormat(loadInfo.format),
		VkTypes::AsVkMapping(format),
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
	ILuint i;
	ILuint j;
	if (cube)
	{
		for (i = 0; i < 6; i++)
		{
			ilBindImage(image);
			ilActiveFace(i);
			ilActiveMipmap(0);
			for (j = 0; j < mips; j++)
			{
				// move to next mip
				ilBindImage(image);
				ilActiveFace(i);
				ilActiveMipmap(j);

				ILuint size = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
				ILubyte* buf = ilGetData();

				int32_t mipWidth = Math::n_max(startWidth >> j, 1u);
				int32_t mipHeight = Math::n_max(startHeight >> j, 1u);
				int32_t mipDepth = Math::n_max(startDepth >> j, 1u);

				extents.width = mipWidth;
				extents.height = mipHeight;
				extents.depth = mipDepth;

				VkImageSubresourceRange res = subres;
				res.layerCount = 1;
				res.levelCount = 1;
				res.baseMipLevel = subres.baseMipLevel + j;
				res.baseArrayLayer = subres.baseArrayLayer + i;

				VkBuffer outBuf;
				VkDeviceMemory outMem;
				VkUtilities::ImageUpdate(
					dev, 
					CoreGraphics::SubmissionContextGetCmdBuffer(sub), 
					TransferQueueType, 
					loadInfo.img, 
					extents, 
					res.baseMipLevel,
					res.baseArrayLayer,
					size, 
					(uint32_t*)buf, 
					outBuf, 
					outMem);

				// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
				SubmissionContextFreeDeviceMemory(sub, dev, outMem);
				SubmissionContextFreeBuffer(sub, dev, outBuf);
			}
		}
	}
	else
	{
		for (j = 0; j < mips; j++)
		{
			// move to next mip
			ilBindImage(image);
			ilActiveMipmap(j);

			ILuint size = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
			ILubyte* buf = ilGetData();

			int32_t mipWidth = Math::n_max(startWidth >> j, 1u);
			int32_t mipHeight = Math::n_max(startHeight >> j, 1u);
			int32_t mipDepth = Math::n_max(startDepth >> j, 1u);

			extents.width = mipWidth;
			extents.height = mipHeight;
			extents.depth = mipDepth;

			VkImageSubresourceRange res = subres;
			res.layerCount = 1;
			res.levelCount = 1;
			res.baseMipLevel = subres.baseMipLevel + j;
			res.baseArrayLayer = 0;

			VkBuffer outBuf;
			VkDeviceMemory outMem;
			VkUtilities::ImageUpdate(
				dev, 
				CoreGraphics::SubmissionContextGetCmdBuffer(sub), 
				TransferQueueType, 
				loadInfo.img, 
				extents, 
				subres.baseMipLevel + j,
				0, 
				size, 
				(uint32_t*)buf, 
				outBuf, 
				outMem);

			// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
			SubmissionContextFreeDeviceMemory(sub, dev, outMem);
			SubmissionContextFreeBuffer(sub, dev, outBuf);
		}
	}
	ilDeleteImage(image);

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


	VkShaderServer::Instance()->AddPendingImageView(viewCreate, TextureId(id));
	// update binding
	//if (loadInfo.bindless)
	//VkShaderServer::Instance()->ReregisterTexture(TextureId(id), runtimeInfo.type, runtimeInfo.bind);
}

} // namespace Vulkan
