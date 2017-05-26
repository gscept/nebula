//------------------------------------------------------------------------------
// vkstreamtextureloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkstreamtextureloader.h"
#include "coregraphics/texture.h"
#include "coregraphics/renderdevice.h"
#include "io/ioserver.h"
#include "coregraphics/vk/vktypes.h"
#include "IL/il.h"

#include <vulkan/vulkan.h>
#include "vkrenderdevice.h"
#include "vkutilities.h"
#include "vkscheduler.h"
namespace Vulkan
{

__ImplementClass(Vulkan::VkStreamTextureLoader, 'VKTL', Resources::StreamResourceLoader);

using namespace CoreGraphics;
using namespace Resources;
using namespace IO;
//------------------------------------------------------------------------------
/**
*/
VkStreamTextureLoader::VkStreamTextureLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkStreamTextureLoader::~VkStreamTextureLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
	FIXME: use transfer queue to update texture asynchronously.
*/
bool
VkStreamTextureLoader::SetupResourceFromStream(const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());
	n_assert(stream->CanBeMapped());
	n_assert(this->resource->IsA(Texture::RTTI));
	const Ptr<Texture>& res = this->resource.downcast<Texture>();
	n_assert(!res->IsLoaded());

	stream->SetAccessMode(Stream::ReadAccess);
	if (stream->Open())
	{
		void* srcData = stream->Map();
		uint srcDataSize = stream->GetSize();

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
		vkGetPhysicalDeviceFormatProperties(VkRenderDevice::physicalDev, vkformat, &formatProps);
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
		uint32_t queues[] = { VkRenderDevice::Instance()->drawQueueFamily, VkRenderDevice::Instance()->transferQueueFamily };
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
		VkImage img;
		VkResult stat = vkCreateImage(VkRenderDevice::dev, &info, NULL, &img);
		n_assert(stat == VK_SUCCESS);

		// allocate memory backing
		VkDeviceMemory mem;
		uint32_t alignedSize;
		VkUtilities::AllocateImageMemory(img, mem, VkMemoryPropertyFlagBits(0), alignedSize);
		vkBindImageMemory(VkRenderDevice::dev, img, mem, 0);

		VkScheduler* scheduler = VkScheduler::Instance();

		// transition into transfer mode
		VkImageSubresourceRange subres;
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = info.arrayLayers;
		subres.levelCount = info.mipLevels;
		scheduler->PushImageLayoutTransition(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(img, subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
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

					int32_t mipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(width / Math::n_pow(2, (float)j)));
					int32_t mipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(height / Math::n_pow(2, (float)j)));
					int32_t mipDepth = (int32_t)Math::n_max(1.0f, Math::n_floor(depth / Math::n_pow(2, (float)j)));

					//VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT, j, 0 };
					//VkSubresourceLayout layout;
					//vkGetImageSubresourceLayout(VkRenderDevice::dev, img, &subres, &layout);

					//memcpy((uint8_t*)mappedData + layout.offset, buf, size);
					info.extent.width = mipWidth;
					info.extent.height = mipHeight;
					info.extent.depth = 1;

					// push a deferred image update, since we may not be within a frame
					scheduler->PushImageUpdate(img, info, j, i, size, (uint32_t*)buf);
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

				int32_t mipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(width / Math::n_pow(2, (float)j)));
				int32_t mipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(height / Math::n_pow(2, (float)j)));
				int32_t mipDepth = (int32_t)Math::n_max(1.0f, Math::n_floor(depth / Math::n_pow(2, (float)j)));

				//memcpy((uint8_t*)mappedData + layout.offset, buf, size);
				info.extent.width = mipWidth;
				info.extent.height = mipHeight;
				info.extent.depth = 1;

				// push a deferred image update, since we may not be within a frame
				scheduler->PushImageUpdate(img, info, j, 0, size, (uint32_t*)buf);
			}
		}	

		// transition to something readable by shaders
		VkClearColorValue val = { 1, 0, 0, 1 };
		scheduler->PushImageLayoutTransition(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(img, subres, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		scheduler->PushImageOwnershipChange(VkDeferredCommand::Transfer, VkUtilities::ImageMemoryBarrier(img, subres, VkDeferredCommand::Transfer, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

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
			NULL,
			0,
			img,
			viewType,
			vkformat,
			//VK_FORMAT_R8G8B8A8_UNORM,
			VkTypes::AsVkMapping(format),
			subres
		};
		VkImageView view;
		stat = vkCreateImageView(VkRenderDevice::dev, &viewCreate, NULL, &view);
		n_assert(stat == VK_SUCCESS);

		// setup texture
		if (cube)
		{
			res->SetupFromVkCubeTexture(img, mem, view, width, height, VkTypes::AsNebulaPixelFormat(vkformat), mips);
		}
		else if (depth > 1)
		{
			res->SetupFromVkVolumeTexture(img, mem, view, width, height, depth, VkTypes::AsNebulaPixelFormat(vkformat), mips);
		}
		else
		{
			res->SetupFromVkTexture(img, mem, view, width, height, VkTypes::AsNebulaPixelFormat(vkformat), mips);
		}

		stream->Unmap();
		stream->Close();
		return true;

	}

	return false;
}

} // namespace Vulkan