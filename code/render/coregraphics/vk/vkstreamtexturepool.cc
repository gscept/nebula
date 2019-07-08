//------------------------------------------------------------------------------
// vkstreamtextureloader.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkstreamtexturepool.h"
#include "coregraphics/texture.h"
#include "io/ioserver.h"
#include "coregraphics/vk/vktypes.h"
#include "IL/il.h"

#include <vulkan/vulkan.h>
#include "vkgraphicsdevice.h"
#include "vkutilities.h"
#include "math/scalar.h"
#include "vkshaderserver.h"
#include "coregraphics/memorytexturepool.h"
#include "vksubmissioncontext.h"
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
	this->placeholderResourceName = "tex:system/placeholder.dds";
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
	n_assert(stream.isvalid());
	n_assert(stream->CanBeMapped());
	n_assert(this->GetState(res) == Resources::Resource::Pending);

	void* srcData = stream->Map();
	uint srcDataSize = stream->GetSize();

	/// during the load-phase, we can safetly get the structs
	texturePool->EnterGet();
	VkTextureRuntimeInfo& runtimeInfo = texturePool->Get<0>(res.resourceId);
	VkTextureLoadInfo& loadInfo = texturePool->Get<1>(res.resourceId);
	loadInfo.dev = Vulkan::GetCurrentDevice();
	texturePool->LeaveGet();

	VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
	VkDevice dev = Vulkan::GetCurrentDevice();

	// load using IL
	ILuint image = ilGenImage();
	ilBindImage(image);
	ilSetInteger(IL_DXTC_NO_DECOMPRESS, IL_TRUE);
	ilLoadL(IL_DDS, srcData, srcDataSize);

	ILuint width = ilGetInteger(IL_IMAGE_WIDTH);
	ILuint height = ilGetInteger(IL_IMAGE_HEIGHT);
	ILuint depth = ilGetInteger(IL_IMAGE_DEPTH);
	ILuint bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	ILuint numImages = ilGetInteger(IL_NUM_IMAGES);
	ILuint numFaces = ilGetInteger(IL_NUM_FACES);
	ILuint numLayers = ilGetInteger(IL_NUM_LAYERS);
	ILuint mips = ilGetInteger(IL_NUM_MIPMAPS);
	ILenum cube = ilGetInteger(IL_IMAGE_CUBEFLAGS);
	ILenum format = ilGetInteger(IL_PIXEL_FORMAT);	// only available when loading DDS, so this might need some work...

	VkFormat vkformat = VkTypes::AsVkFormat(format);
	VkTypes::VkBlockDimensions block = VkTypes::AsVkBlockSize(vkformat);

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
	extents.depth = 1;
	VkImageCreateInfo info =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		NULL,
		cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
		depth > 1 ? VK_IMAGE_TYPE_3D : (height > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D),
		vkformat,
		extents,
		mips,
		cube ? (uint32_t)numFaces : (uint32_t)numImages,
		VK_SAMPLE_COUNT_1_BIT,
		forceLinear ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkResult stat = vkCreateImage(dev, &info, NULL, &loadInfo.img);
	n_assert(stat == VK_SUCCESS);

	// allocate memory backing
	uint32_t alignedSize;
	VkUtilities::AllocateImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, VkMemoryPropertyFlagBits(0), alignedSize);
	vkBindImageMemory(dev, loadInfo.img, loadInfo.mem, 0);

	// transition into transfer mode
	VkImageSubresourceRange subres;
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.baseArrayLayer = 0;
	subres.baseMipLevel = 0;
	subres.layerCount = info.arrayLayers;
	subres.levelCount = info.mipLevels;

	// use resource submission
	CoreGraphics::SubmissionContextId sub = CoreGraphics::GetResourceSubmissionContext();

	// transition to transfer
	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::Host,
		CoreGraphics::BarrierStage::Transfer,
		VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
	
	uint32_t remainingBytes = alignedSize;

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
				remainingBytes -= size;
				n_assert(remainingBytes >= 0);
				ILubyte* buf = ilGetData();

				// copy buffer, will be deleted later
				char* bufferCopy = new char[size];
				memcpy(bufferCopy, buf, size);

				int32_t mipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(width / Math::n_pow(2, (float)j)));
				int32_t mipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(height / Math::n_pow(2, (float)j)));
				int32_t mipDepth = (int32_t)Math::n_max(1.0f, Math::n_floor(depth / Math::n_pow(2, (float)j)));

				info.extent.width = mipWidth;
				info.extent.height = mipHeight;
				info.extent.depth = 1;

				VkBuffer outBuf;
				VkDeviceMemory outMem;
				VkUtilities::ImageUpdate(dev, CoreGraphics::SubmissionContextGetCmdBuffer(sub), TransferQueueType, loadInfo.img, info, j, i, size, (uint32_t*)bufferCopy, outBuf, outMem);

				// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
				SubmissionContextFreeHostMemory(sub, bufferCopy);
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
			remainingBytes -= size;
			n_assert(remainingBytes >= 0);
			ILubyte* buf = ilGetData();

			// copy buffer, will be deleted later
			char* bufferCopy = new char[size];
			memcpy(bufferCopy, buf, size);

			int32_t mipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(width / Math::n_pow(2, (float)j)));
			int32_t mipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(height / Math::n_pow(2, (float)j)));
			int32_t mipDepth = (int32_t)Math::n_max(1.0f, Math::n_floor(depth / Math::n_pow(2, (float)j)));

			//memcpy((uint8_t*)mappedData + layout.offset, buf, size);
			info.extent.width = mipWidth;
			info.extent.height = mipHeight;
			info.extent.depth = 1;

			VkBuffer outBuf;
			VkDeviceMemory outMem;
			VkUtilities::ImageUpdate(dev, CoreGraphics::SubmissionContextGetCmdBuffer(sub), TransferQueueType, loadInfo.img, info, j, 0, size, (uint32_t*)bufferCopy, outBuf, outMem);

			// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
			SubmissionContextFreeHostMemory(sub, bufferCopy);
			SubmissionContextFreeDeviceMemory(sub, dev, outMem);
			SubmissionContextFreeBuffer(sub, dev, outBuf);
		}
	}

	// transition image to be used for rendering
	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::Host,
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, TransferQueueType, GraphicsQueueType, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		
	ilDeleteImage(image);

	// create view
	VkImageViewType viewType;
	if (cube) viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	else
	{
		if (height > 1)
		{
			if (depth > 1) viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			else		   viewType = VK_IMAGE_VIEW_TYPE_2D;
		}
		else
		{
			if (depth > 1) viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			else		   viewType = VK_IMAGE_VIEW_TYPE_1D;
		}
	}

	VkImageViewCreateInfo viewCreate =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		loadInfo.img,
		viewType,
		vkformat,
		//VK_FORMAT_R8G8B8A8_UNORM,
		VkTypes::AsVkMapping(format),
		subres
	};
	stat = vkCreateImageView(dev, &viewCreate, NULL, &runtimeInfo.view);
	n_assert(stat == VK_SUCCESS);

	loadInfo.dims.width = width;
	loadInfo.dims.height = height;
	loadInfo.dims.depth = depth;
	loadInfo.mips = Math::n_max(mips, 1u);
	loadInfo.format = VkTypes::AsNebulaPixelFormat(vkformat);
	loadInfo.dev = dev;
	runtimeInfo.type = cube ? CoreGraphics::TextureCube : depth > 1 ? CoreGraphics::Texture3D : CoreGraphics::Texture2D;
	runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(TextureId(res), runtimeInfo.type);

	stream->Unmap();

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
	texturePool->Unload(id);
}

} // namespace Vulkan