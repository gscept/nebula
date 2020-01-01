//------------------------------------------------------------------------------
//  binaryreader.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/binaryreader.h"

namespace IO
{
__ImplementClass(IO::BinaryReader, 'BINR', IO::StreamReader);

using namespace Util;
using namespace System;

//------------------------------------------------------------------------------
/**
*/
BinaryReader::BinaryReader() :
    enableMapping(false),
    isMapped(false),
    mapCursor(0),
    mapEnd(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
BinaryReader::~BinaryReader()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryReader::Open()
{
    if (StreamReader::Open())
    {
        if (this->enableMapping && this->stream->CanBeMapped())
        {
            this->isMapped = true;
            this->mapCursor = (unsigned char*) this->stream->Map();
            this->mapEnd = this->mapCursor + this->stream->GetSize();
        }
        else
        {
            this->isMapped = false;
            this->mapCursor = 0;
            this->mapEnd = 0;
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryReader::Close()
{
    StreamReader::Close();
    this->isMapped = false;
    this->mapCursor = 0;
    this->mapEnd = 0;
}

//------------------------------------------------------------------------------
/**
*/
char
BinaryReader::ReadChar()
{
    if (this->isMapped)
    {
        n_assert(this->mapCursor < this->mapEnd);
        return *this->mapCursor++;
    }
    else
    {
        char c;
        this->stream->Read(&c, sizeof(c));
        return c;
    }
}

//------------------------------------------------------------------------------
/**
*/
unsigned char
BinaryReader::ReadUChar()
{
    if (this->isMapped)
    {
        n_assert(this->mapCursor < this->mapEnd);
        return *this->mapCursor++;
    }
    else
    {
        unsigned char c;
        this->stream->Read(&c, sizeof(c));
        return c;
    }
}

//------------------------------------------------------------------------------
/**
*/
short
BinaryReader::ReadShort()
{
    short val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(short)) <= this->mapEnd);        
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
    return this->byteOrder.Convert<short>(val);
}

//------------------------------------------------------------------------------
/**
*/ 
unsigned short
BinaryReader::ReadUShort()
{
    ushort val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(unsigned short)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
    return this->byteOrder.Convert<ushort>(val);
}

//------------------------------------------------------------------------------
/**
*/
int
BinaryReader::ReadInt()
{
    int val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(int)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
    return this->byteOrder.Convert<int>(val);
}

//------------------------------------------------------------------------------
/**
*/
unsigned int
BinaryReader::ReadUInt()
{
    uint val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(unsigned int)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
    return this->byteOrder.Convert<uint>(val);
}

//------------------------------------------------------------------------------
/**
*/
long long
BinaryReader::ReadInt64()
{
	long long val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
		n_assert((this->mapCursor + sizeof(long long)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
	return this->byteOrder.Convert<int64_t>(val);
}

//------------------------------------------------------------------------------
/**
*/
unsigned long long
BinaryReader::ReadUInt64()
{
	unsigned long long val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
		n_assert((this->mapCursor + sizeof(unsigned long long)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
	return this->byteOrder.Convert<uint64_t>(val);
}

//------------------------------------------------------------------------------
/**
*/
float
BinaryReader::ReadFloat()
{
    float val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(float)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
    return this->byteOrder.Convert<float>(val);
}

//------------------------------------------------------------------------------
/**
*/
float
BinaryReader::ReadFloatFromNormalizedUByte2()
{
    unsigned short compressedVal = this->ReadUShort();
    float result = ((float)compressedVal / 32767.5f) + 1.0f;
    return result;
}

//------------------------------------------------------------------------------
/**
*/
float
BinaryReader::ReadFloatFromUnsignedNormalizedUByte2()
{
    unsigned short compressedVal = this->ReadUShort();
    float result = (float)compressedVal / 65535.0f;
    return result;
}

//------------------------------------------------------------------------------
/**
*/
double
BinaryReader::ReadDouble()
{
    double val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(double)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
    return this->byteOrder.Convert<double>(val);
}

//------------------------------------------------------------------------------
/**
*/
bool
BinaryReader::ReadBool()
{
    bool val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(bool)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));        
    }
    return val;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
BinaryReader::ReadString()
{
    if (this->isMapped)
    {
        ushort length = this->ReadUShort();
        n_assert((this->mapCursor + length) <= this->mapEnd);
        Util::String str;
        if (length > 0)
        {
			str.Fill(length, 0);
            char* buf = (char*) str.AsCharPtr();
            Memory::Copy(this->mapCursor, buf, length);
            this->mapCursor += length;
            buf[length] = 0;
        }
        return str;    
    }
    else
    {
        ushort length = this->ReadUShort();
        Util::String str;
        if (length > 0)
        {
            str.Fill(length, 0);
            char* buf = (char*) str.AsCharPtr();
            this->stream->Read((void*)buf, length);
            buf[length] = 0;
        }
        return str;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Blob
BinaryReader::ReadBlob()
{
    SizeT numBytes = this->ReadUInt();
    Util::Blob blob(numBytes);
    void* ptr = const_cast<void*>(blob.GetPtr());
    if (this->isMapped)
    {
        n_assert((this->mapCursor + numBytes) <= this->mapEnd);
        Memory::Copy(this->mapCursor, ptr, numBytes);
        this->mapCursor += numBytes;
    }
    else
    {
        this->stream->Read(ptr, numBytes);
    }
    return blob;
}

//------------------------------------------------------------------------------
/**
*/
Util::Guid
BinaryReader::ReadGuid()
{
    Util::Blob blob = this->ReadBlob();
    return Util::Guid((const unsigned char*) blob.GetPtr(), blob.Size());
}
     
#if !__OSX__
//------------------------------------------------------------------------------
/**
*/
Math::float2 
BinaryReader::ReadFloat2()
{
    Math::float2 val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(Math::float2)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }

    val.set(this->byteOrder.Convert<float>(val.x()),
            this->byteOrder.Convert<float>(val.y()));
    return val;    
}

//------------------------------------------------------------------------------
/**
*/ 
Math::float4
BinaryReader::ReadFloat4()
{
    Math::float4 val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(Math::float4)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));     
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
    this->byteOrder.ConvertInPlace<Math::float4>(val);
    return val;
}    

//------------------------------------------------------------------------------
/**
*/
Math::point 
BinaryReader::ReadPoint()
{
    float val[3];
    const SizeT readSize = sizeof(float) * 3;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + readSize) <= this->mapEnd);
        Memory::Copy(this->mapCursor, val, readSize);
        this->mapCursor += readSize;
    }
    else
    {
        this->stream->Read(val, readSize);
    }

    this->byteOrder.ConvertInPlace<float>(val[0]);
    this->byteOrder.ConvertInPlace<float>(val[1]);
    this->byteOrder.ConvertInPlace<float>(val[2]);

    return Math::point(val[0], val[1], val[2]);
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
BinaryReader::ReadVector()
{
    float val[3];    
    const SizeT readSize = sizeof(float) * 3;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + readSize) <= this->mapEnd);
        Memory::Copy(this->mapCursor, val, readSize);
        this->mapCursor += readSize;
    }
    else
    {
        this->stream->Read(val, readSize);
    }

    this->byteOrder.ConvertInPlace<float>(val[0]);
    this->byteOrder.ConvertInPlace<float>(val[1]);
    this->byteOrder.ConvertInPlace<float>(val[2]);

    return Math::vector(val[0], val[1], val[2]);
}

//------------------------------------------------------------------------------
/**
*/ 
Math::matrix44
BinaryReader::ReadMatrix44()
{
    Math::matrix44 val;
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(Math::matrix44)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, &val, sizeof(val));        
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Read(&val, sizeof(val));
    }
    this->byteOrder.ConvertInPlace<Math::matrix44>(val);
    return val;
}
#endif

//------------------------------------------------------------------------------
/**
*/
Util::Array<float> 
BinaryReader::ReadFloatArray()
{
    int size = this->ReadInt();
    float* buf = (float*)Memory::Alloc(Memory::ScratchHeap, sizeof(float) * size);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(Math::matrix44)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, buf, sizeof(float) * size);        
        this->mapCursor += sizeof(float) * size;
    }
    else
    {
        this->stream->Read(buf, sizeof(float) * size);
    }
    Util::Array<float> val(size, 0, 0);
    IndexT i;
    for (i = 0; i < size; i++) val[i] = buf[i];
    Memory::Free(Memory::ScratchHeap, (void*)buf);
    return val;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<int> 
BinaryReader::ReadIntArray()
{
    int size = this->ReadInt();
    int* buf = (int*)Memory::Alloc(Memory::ScratchHeap, sizeof(int) * size);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(Math::matrix44)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, buf, sizeof(int) * size);        
        this->mapCursor += sizeof(int) * size;
    }
    else
    {
        this->stream->Read(buf, sizeof(int) * size);
    }
    Util::Array<int> val(size, 0, 0);
    IndexT i;
    for (i = 0; i < size; i++) val[i] = buf[i];
    Memory::Free(Memory::ScratchHeap, (void*)buf);
    return val;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<uint>
BinaryReader::ReadUIntArray()
{
	uint size = this->ReadInt();
	Util::Array<uint> val(size, 0, 0);
	if (size == 0)
	{
		//Early out if size is 0
		return val;
	}

	uint* buf = (uint*)Memory::Alloc(Memory::ScratchHeap, sizeof(uint) * size);
	if (this->isMapped)
	{
		// note: the memory copy is necessary to circumvent alignment problem on some CPUs
		n_assert((this->mapCursor + sizeof(Math::matrix44)) <= this->mapEnd);
		Memory::Copy(this->mapCursor, buf, sizeof(uint) * size);
		this->mapCursor += sizeof(uint) * size;
	}
	else
	{
		this->stream->Read(buf, sizeof(uint) * size);
	}
	uint i;
	for (i = 0; i < size; i++) val[i] = buf[i];
	Memory::Free(Memory::ScratchHeap, (void*)buf);
	return val;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<bool> 
BinaryReader::ReadBoolArray()
{
    int size = this->ReadInt();
    bool* buf = (bool*)Memory::Alloc(Memory::ScratchHeap, sizeof(bool) * size);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(Math::matrix44)) <= this->mapEnd);
        Memory::Copy(this->mapCursor, buf, sizeof(bool) * size);        
        this->mapCursor += sizeof(bool) * size;
    }
    else
    {
        this->stream->Read(buf, sizeof(bool) * size);
    }
    Util::Array<bool> val(size, 0, 0);
    IndexT i;
    for (i = 0; i < size; i++) val[i] = buf[i];
    Memory::Free(Memory::ScratchHeap, (void*)buf);
    return val;
}

//------------------------------------------------------------------------------
/**
*/ 
void
BinaryReader::ReadRawData(void* ptr, SizeT numBytes)
{
    n_assert((ptr != 0) && (numBytes > 0));
    if (this->isMapped)
    {
        n_assert((this->mapCursor + numBytes) <= this->mapEnd);
        Memory::Copy(this->mapCursor, ptr, numBytes);
        this->mapCursor += numBytes;
    }
    else
    {
        this->stream->Read(ptr, numBytes);
    }
}

} // namespace IO
