//------------------------------------------------------------------------------
//  resourcedictionarybuilder.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "core/refcounted.h"
#include "resources/streaming/resourceinfo.h"
#include "coregraphics/pixelformat.h"
#include "resourcedictionarybuilder.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/binarywriter.h"
#include "io/uri.h"
#include "util/stringatom.h"
#include "system/byteorder.h"
#include "resources/streaming/textureinfo.h"
#include "resources/streaming/poolresourcemapper.h"
#include "io/xmlwriter.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;
using namespace System;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
ResourceDictionaryBuilder::ResourceDictionaryBuilder(void) :
    platform(Platform::Win32)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceDictionaryBuilder::~ResourceDictionaryBuilder(void)
{
    this->dict.Clear();    
}

//------------------------------------------------------------------------------
/**
*/
bool
ResourceDictionaryBuilder::ReadTextureData(Ptr<Stream>& stream, TextureInfo& format)
{
    n_assert(stream->IsOpen());
    bool invalid = false;

    void* srcData = stream->Map();
    uint srcDataSize = stream->GetSize();

#if __DX11__
	HRESULT hr;
	D3DX11_IMAGE_INFO imageInfo = {0};
	hr = D3DX11GetImageInfoFromMemory(srcData, srcDataSize, NULL, &imageInfo, NULL);
#elif (__OGL4__ || __VULKAN__)
	ILint imageInfo = ilGenImage();
	ilBindImage(imageInfo);
	ILboolean success = ilLoadL(IL_DDS, srcData, srcDataSize);
#elif __DX9__
	HRESULT hr;
    D3DXIMAGE_INFO imageInfo = {0};
    hr = D3DXGetImageInfoFromFileInMemory(srcData, srcDataSize, &imageInfo);
#endif

#if __DX11__ || __DX9__
    if (FAILED(hr))
    {
        n_error("ResourceDictionaryBuilder: failed to obtain image info from file '%s'!", stream->GetURI().AsString());
    }
#elif __OGL4__
	if (!success)
	{
		n_error("ResourceDictionaryBuilder: failed to obtain image info from file '%s'!", stream->GetURI().AsString().AsCharPtr());
	}
#endif
    format.SetInfo(imageInfo);
    format.SetSize(stream->GetSize());
    // check if format is valid
    if (format.GetDepth() == 0 ||
        format.GetHeight() == 0 ||
        format.GetWidth() == 0 ||
        format.GetMipLevels() == 0 ||
        format.GetPixelFormat() == CoreGraphics::PixelFormat::InvalidPixelFormat ||
        format.GetType() == CoreGraphics::Texture::InvalidType ||
        format.GetSize() == 0)
    {
#ifdef DEBUG_RES_DICT_BUILDER
        n_error("ResourceDictionaryBuilder::ReadTextureData(): invalid texture '%s' :\n"
            "height: %i\twidth: %i\n"
            "depth: %i\tmipLevels: %i\n"
            "pixelFormat: '%s'\ttype: %i\n"
            "size: %i",
            stream->GetURI().AsString().AsCharPtr(), format.GetHeight(), format.GetWidth(), format.GetDepth(),
            format.GetMipLevels(), CoreGraphics::PixelFormat::ToString(format.GetPixelFormat()).AsCharPtr(), format.GetType(), format.GetSize());
#endif
        invalid = true;
    }

#if __OGL4__
	ilDeleteImage(imageInfo);
#endif
    stream->Unmap();
    
    return invalid;
}

//------------------------------------------------------------------------------
/**
*/
bool
ResourceDictionaryBuilder::BuildDictionary()
{
    n_assert(this->texDir.IsValid());
    n_assert(this->dict.IsEmpty());

    if (!this->AddTexturesToDictionary())
    {
        return false;
    }
    if (!this->SaveDictionary())
    {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ResourceDictionaryBuilder::AddTexturesToDictionary()
{
    // for each category...
    Array<String> categories = IoServer::Instance()->ListDirectories(URI(this->texDir), "*");
    IndexT catIndex;
    for (catIndex = 0; catIndex < categories.Size(); catIndex++)
    {
        // ignore revision control system dirs
        if ((categories[catIndex] != "CVS") && (categories[catIndex] != ".svn"))
        {
            // for each texture in category...
            String catDir;
            catDir.Format("%s/%s", this->texDir.AsCharPtr(), categories[catIndex].AsCharPtr());
            String pattern;
			pattern = "*.dds";
            Array<String> files = IoServer::Instance()->ListFiles(catDir, pattern);
            IndexT fileIndex;
            for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
            {
                // build abs path and resource id string
                String absPath;
                absPath.Format("%s/%s/%s", this->texDir.AsCharPtr(), categories[catIndex].AsCharPtr(), files[fileIndex].AsCharPtr());
                String resId;
                resId.Format("tex:%s/%s", categories[catIndex].AsCharPtr(), files[fileIndex].AsCharPtr());

                // get the resource file size, we don't care about header overhead
                Ptr<Stream> stream = IoServer::Instance()->CreateStream(absPath);
                TextureInfo texInfo;
                stream->SetAccessMode(Stream::ReadAccess);
                if (false == stream->Open())
                {
                    n_printf("ERROR, SKIPPED RESOURCE '%s' because file could not be read\n", absPath.AsCharPtr());
                    continue;
                }
                bool skip = this->ReadTextureData(stream, texInfo);
                stream->Close();
                stream = 0;
                if (true == skip)
                {
                    n_printf("ERROR, SKIPPED RESOURCE '%s' because one or more of following data is invalid:\t"
                        "height: %i\twidth: %i\t"
                        "depth: %i\tmipLevels: %i\t"
                        "pixelFormat: '%s'\ttype: %i\t"
                        "size: %i\n",
                        absPath.AsCharPtr(),
                        texInfo.GetHeight(), texInfo.GetWidth(),
                        texInfo.GetDepth(), texInfo.GetMipLevels(),
                        CoreGraphics::PixelFormat::ToString(texInfo.GetPixelFormat()).AsCharPtr(), texInfo.GetType(),
                        texInfo.GetSize());
                }
                else
                {
                    n_printf("Adding '%s'to dictionary.\t", resId.AsCharPtr());
#if DEBUG_RES_DICT_BUILDER
                    n_printf("height: %i\twidth: %i\t"
                        "depth: %i\tmipLevels: %i\t"
                        "pixelFormat: '%s'\ttype: %i\t"
                        "size: %i\n",
                        texInfo.GetHeight(), texInfo.GetWidth(),
                        texInfo.GetDepth(), texInfo.GetMipLevels(),
                        CoreGraphics::PixelFormat::ToString(texInfo.GetPixelFormat()).AsCharPtr(), texInfo.GetType(),
                        texInfo.GetSize());
#endif
                    
                    this->dict.Add(resId, texInfo);
                }
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ResourceDictionaryBuilder::SaveDictionary()
{
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(URI(this->texDir + "/resources.dic"));
    Ptr<BinaryWriter> writer = BinaryWriter::Create();
    writer->SetStream(stream);
    if (Platform::Win32 == this->platform)
    {
        writer->SetStreamByteOrder(ByteOrder::LittleEndian);
    }
    else
    {
        writer->SetStreamByteOrder(ByteOrder::BigEndian);
    }
    if (writer->Open())
    {
        // write header
        FourCC magic('RDIC');
        writer->WriteUInt(magic.AsUInt());      // magic number 'RDIC'
        writer->WriteUInt(2);                   // version (0002)
        writer->WriteUInt(this->dict.Size());   // number of entries in the dictionary

        // for each dictionary entry...
        IndexT i;
        for (i = 0; i < this->dict.Size(); i++)
        {
            const StringAtom& resId = this->dict.KeyAtIndex(i);
            TextureInfo* texFormat = &this->dict.ValueAtIndex(i);
            n_assert(resId.AsString().Length() < MaxResIdLength);

            // copy resource id content to a memory buffer
            uchar buf[MaxResIdLength] = { 0 };
            Memory::Copy(resId.Value(), buf, resId.AsString().Length());

            // Texture::Type and PixelFormat are enums and so written as int
            // - if this may produce problems on platforms change to other data-type
            // here and in appropriate Resources::ResourceDictionaryReader-classes

            // write current entry to file
            writer->WriteInt(int(texFormat->GetType()));
            writer->WriteUInt(texFormat->GetWidth());
            writer->WriteUInt(texFormat->GetHeight());
            writer->WriteUInt(texFormat->GetDepth());
            writer->WriteUInt(texFormat->GetMipLevels());
            writer->WriteInt(int(texFormat->GetPixelFormat()));
            writer->WriteUInt(texFormat->GetSize());
            writer->WriteRawData(buf, sizeof(buf));
        }
        writer->Close();
        // godsend test pool with 500MB
        this->CreateDefaultTexturePool(URI(this->texDir + "/default_pool.xml"), __maxTextureBytes__);
        return true;
    }
    else
    {
        n_printf("ERROR: couldn't open '%s' for writing!\n", this->texDir.AsCharPtr());
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceDictionaryBuilder::Unload()
{
    this->dict.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceDictionaryBuilder::CreateDefaultTexturePool(const URI& fileName, uint maxBytes)
{
    n_printf("creating default pools ...\n");
    // create at least one pool with one slot per texture for default pool
    Dictionary<StringAtom, PoolSetupInfo> slotsPerType;
    Dictionary<StringAtom, int> numSlotRequests;
    // reserve some initial buffer space
    slotsPerType.Reserve(100);
    uint bytesLeft = maxBytes;
    IndexT dictIdx, addedIdx;
    StringAtom poolName;
    for (dictIdx = 0; dictIdx < this->dict.Size(); dictIdx++)
    {
        poolName.Clear();
        const TextureInfo* curInfo = &this->dict.ValueAtIndex(dictIdx);
        for (addedIdx = 0; addedIdx < slotsPerType.Size(); addedIdx++)
        {
            if (curInfo->IsEqual(slotsPerType.ValueAtIndex(addedIdx).info))
            {
                poolName = slotsPerType.KeyAtIndex(addedIdx);
                break;
            }
        }

        if (false == poolName.IsValid())
        {
            bytesLeft -= curInfo->GetSize();
            // ensure we can append a new pool with one slot
            n_assert2(bytesLeft >= 0, "Error: ResourceDictionaryBuiler::CreateDefaultTexturePool() - not enough space given to create at least one resource slot per Texture type. Size given: ");
            // 1 slot minimum and slot-type once requested...
            StringAtom poolId = CoreGraphics::PixelFormat::ToString(curInfo->GetPixelFormat()) + String::FromInt(curInfo->GetWidth()) + String::FromInt(curInfo->GetHeight()) + String::FromInt(curInfo->GetMipLevels());
            PoolSetupInfo newPool;
            newPool.numSlots = 1;
            newPool.info = (ResourceInfo*)curInfo;
            slotsPerType.Add(poolId, newPool);
            numSlotRequests.Add(poolId, 1);
        }
        else
        {
            // increase counter of requested slot
            numSlotRequests[poolName]++;
        }
    }

    n_printf("bytes left after adding 1 slot per pool:%ukB of %u kB total\n", bytesLeft / 1024, maxBytes / 1024);

    // evaluate numSlots dictionary
    for (dictIdx = 0; dictIdx < numSlotRequests.Size(); dictIdx++)
    {
        n_assert(slotsPerType.Contains(numSlotRequests.KeyAtIndex(dictIdx)) && numSlotRequests.ValueAtIndex(dictIdx) > 0);
    }

    IndexT highestPoolRequestCounter, poolIdx;
    StringAtom highestPoolRequest;
    uint totalLoopCount = 0;
    bool allSlotsEnabled = false;
    while (bytesLeft > 0)
    {
        highestPoolRequestCounter = -1;
        highestPoolRequest.Clear();
        for (poolIdx = 0; poolIdx < numSlotRequests.Size(); poolIdx++)
        {
            // if slot has more requests than any previous and there is enough space left, set this slot as highest priority for slot-increase
            if (numSlotRequests.ValueAtIndex(poolIdx) > highestPoolRequestCounter &&
                slotsPerType[numSlotRequests.KeyAtIndex(poolIdx)].numSlots < numSlotRequests.ValueAtIndex(poolIdx) &&
                slotsPerType[numSlotRequests.KeyAtIndex(poolIdx)].info->GetSize() <= bytesLeft)
            {
                highestPoolRequestCounter = numSlotRequests.ValueAtIndex(poolIdx);
                highestPoolRequest = numSlotRequests.KeyAtIndex(poolIdx);
            }
        }
        totalLoopCount++;
        if (highestPoolRequest.IsValid())
        {
            slotsPerType[highestPoolRequest].numSlots++;
            bytesLeft -= slotsPerType[highestPoolRequest].info->GetSize();
        }
        else
        {
            allSlotsEnabled = true;
            break;
        }
    }
    n_printf("default texture pool creation: loop count(%u), loop count * pools(%u)\n", totalLoopCount, totalLoopCount * (uint)slotsPerType.Size());
    n_printf("bytes left after creating max slots per pool:%ukB of %u kB total\n", bytesLeft / 1024, maxBytes / 1024);
    if (allSlotsEnabled)
    {
        n_printf("enough space to load all textures!\n");
    }
    PoolResourceMapper::WriteTexturePoolToXML(fileName, slotsPerType);
}
} // namespace ToolkitUtil
