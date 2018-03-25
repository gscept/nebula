//------------------------------------------------------------------------------
//  binarywriter.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/binarywriter.h"

namespace IO
{
__ImplementClass(IO::BinaryWriter, 'BINW', IO::StreamWriter);

using namespace Util;
using namespace System;

#if !__OSX__    
using namespace Math;
#endif
    
//------------------------------------------------------------------------------
/**
*/
BinaryWriter::BinaryWriter() :
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
BinaryWriter::~BinaryWriter()
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
BinaryWriter::Open()
{
    if (StreamWriter::Open())
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
BinaryWriter::Close()
{
    StreamWriter::Close();
    this->isMapped = false;
    this->mapCursor = 0;
    this->mapEnd = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteChar(char c)
{
    if (this->isMapped)
    {
        n_assert(this->mapCursor < this->mapEnd);
        *this->mapCursor++ = c;
    }
    else
    {
        this->stream->Write(&c, sizeof(c));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteUChar(unsigned char c)
{
    if (this->isMapped)
    {
        n_assert(this->mapCursor < this->mapEnd);
        *this->mapCursor++ = c;
    }
    else
    {
        this->stream->Write(&c, sizeof(c));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteShort(short s)
{
    s = this->byteOrder.Convert<short>(s);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(s)) <= this->mapEnd);
        Memory::Copy(&s, this->mapCursor, sizeof(s));
        this->mapCursor += sizeof(s);
    }
    else
    {
        this->stream->Write(&s, sizeof(s));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteUShort(unsigned short s)
{
    s = this->byteOrder.Convert<ushort>(s);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(s)) <= this->mapEnd);
        Memory::Copy(&s, this->mapCursor, sizeof(s));
        this->mapCursor += sizeof(s);
    }
    else
    {
        this->stream->Write(&s, sizeof(s));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteInt(int i)
{
    i = this->byteOrder.Convert<int>(i);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(i)) <= this->mapEnd);
        Memory::Copy(&i, this->mapCursor, sizeof(i));
        this->mapCursor += sizeof(i);
    }
    else
    {
        this->stream->Write(&i, sizeof(i));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteUInt(unsigned int i)
{
    i = this->byteOrder.Convert<uint>(i);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(i)) <= this->mapEnd);
        Memory::Copy(&i, this->mapCursor, sizeof(i));
        this->mapCursor += sizeof(i);
    }
    else
    {
        this->stream->Write(&i, sizeof(i));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteInt64(int64_t i)
{
	i = this->byteOrder.Convert<int64_t>(i);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(i)) <= this->mapEnd);
        Memory::Copy(&i, this->mapCursor, sizeof(i));
        this->mapCursor += sizeof(i);
    }
    else
    {
        this->stream->Write(&i, sizeof(i));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteUInt64(uint64_t i)
{
	i = this->byteOrder.Convert<uint64_t>(i);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(i)) <= this->mapEnd);
        Memory::Copy(&i, this->mapCursor, sizeof(i));
        this->mapCursor += sizeof(i);
    }
    else
    {
        this->stream->Write(&i, sizeof(i));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteFloat(float f)
{
    f = this->byteOrder.Convert<float>(f);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(f)) <= this->mapEnd);
        Memory::Copy(&f, this->mapCursor, sizeof(f));
        this->mapCursor += sizeof(f);
    }
    else
    {
        this->stream->Write(&f, sizeof(f));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteFloatAsNormalizedUByte2(float f)
{
    n_assert(f >= -1.0f && f <= 1.0f);
    unsigned short compressedVal = (unsigned short)((f + 1.0f) * 32767.5f);   
    this->WriteUShort(compressedVal);
}
          
//------------------------------------------------------------------------------
/**
*/
void 
BinaryWriter::WriteFloatAsUnsignedNormalizedUByte2(float f)
{
    n_assert(f >= 0.0f && f <= 1.0f);
    unsigned short compressedVal = (unsigned short)(f * 65535.0f);   
    this->WriteUShort(compressedVal);
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteDouble(double d)
{
    d = this->byteOrder.Convert<double>(d);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(d)) <= this->mapEnd);
        Memory::Copy(&d, this->mapCursor, sizeof(d));
        this->mapCursor += sizeof(d);
    }
    else
    {
        this->stream->Write(&d, sizeof(d));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteBool(bool b)
{
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(b)) <= this->mapEnd);
        Memory::Copy(&b, this->mapCursor, sizeof(b));
        this->mapCursor += sizeof(b);
    }
    else
    {
        this->stream->Write(&b, sizeof(b));
    }
}

//------------------------------------------------------------------------------
/**
    NOTE: for strings, first the length will be written into a
    32-bit int, then the string contents without the 0-terminator.
*/
void
BinaryWriter::WriteString(const Util::String& s)
{
    n_assert(s.Length() < (1<<16));
    this->WriteUShort(ushort(s.Length()));
    if (s.Length() > 0)
    {
        if (this->isMapped)
        {
            n_assert((this->mapCursor + s.Length()) <= this->mapEnd);
            Memory::Copy(s.AsCharPtr(), this->mapCursor, s.Length());
            this->mapCursor += s.Length();
        }
        else
        {
            this->stream->Write((void*)s.AsCharPtr(), s.Length());
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteBlob(const Util::Blob& blob)
{
    this->WriteUInt(blob.Size());
    if (this->isMapped)
    {
        n_assert((this->mapCursor + blob.Size()) <= this->mapEnd);
        Memory::Copy(blob.GetPtr(), this->mapCursor, blob.Size());
        this->mapCursor += blob.Size();
    }
    else
    {
        this->stream->Write(blob.GetPtr(), blob.Size());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteGuid(const Util::Guid& guid)
{
    const unsigned char* ptr;
    SizeT size = guid.AsBinary(ptr);
    Util::Blob blob(ptr, size);
    this->WriteBlob(blob);
}

#if !__OSX__
//------------------------------------------------------------------------------
/**
*/
void 
BinaryWriter::WriteFloat2(Math::float2 f)
{
    float2 val(this->byteOrder.Convert<float>(f.x()),
               this->byteOrder.Convert<float>(f.y()));
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(f)) <= this->mapEnd);
        Memory::Copy(&val, this->mapCursor, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Write(&val, sizeof(val));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteFloat4(const float4& v)
{
    float4 val = v;
    this->byteOrder.ConvertInPlace<float4>(val);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(v)) <= this->mapEnd);
        Memory::Copy(&val, this->mapCursor, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Write(&val, sizeof(val));
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
BinaryWriter::WritePoint(const Math::point& v)
{
    float val[3];
    val[0] = this->byteOrder.Convert<float>(v.x());
    val[1] = this->byteOrder.Convert<float>(v.y());
    val[2] = this->byteOrder.Convert<float>(v.z());
    const SizeT writeSize = sizeof(float) * 3; 
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + writeSize) <= this->mapEnd);
        Memory::Copy(val, this->mapCursor, writeSize);
        this->mapCursor += writeSize;
    }
    else
    {
        this->stream->Write(val, writeSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
BinaryWriter::WriteVector(const Math::vector& v)
{
    float val[3];
    val[0] = this->byteOrder.Convert<float>(v.x());
    val[1] = this->byteOrder.Convert<float>(v.y());
    val[2] = this->byteOrder.Convert<float>(v.z());
    const SizeT writeSize = sizeof(float) * 3; 
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + writeSize) <= this->mapEnd);
        Memory::Copy(val, this->mapCursor, writeSize);
        this->mapCursor += writeSize;
    }
    else
    {
        this->stream->Write(val, writeSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteMatrix44(const matrix44& m)
{
    matrix44 val = m;
    this->byteOrder.ConvertInPlace<matrix44>(val);
    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + sizeof(val)) <= this->mapEnd);
        Memory::Copy(&val, this->mapCursor, sizeof(val));
        this->mapCursor += sizeof(val);
    }
    else
    {
        this->stream->Write(&val, sizeof(val));
    }
}
#endif

//------------------------------------------------------------------------------
/**
*/
void 
BinaryWriter::WriteFloatArray(const Util::Array<float>& arr)
{
    const SizeT writeSize = sizeof(float) * arr.Size(); 
    float* buf = (float*)Memory::Alloc(Memory::ScratchHeap, writeSize);

    IndexT i;
    for (i = 0; i < arr.Size(); i++)
    {
        buf[i] = this->byteOrder.Convert<float>(arr[i]);
    }

    // first write size
    this->WriteInt(arr.Size());

    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + writeSize) <= this->mapEnd);
        Memory::Copy(buf, this->mapCursor, writeSize);
        this->mapCursor += writeSize;
    }
    else
    {
        this->stream->Write(buf, writeSize);
    }
    Memory::Free(Memory::ScratchHeap, (void*)buf);
}

//------------------------------------------------------------------------------
/**
*/
void 
BinaryWriter::WriteIntArray(const Util::Array<int>& arr)
{
    const SizeT writeSize = sizeof(int) * arr.Size(); 
    int* buf = (int*)Memory::Alloc(Memory::ScratchHeap, writeSize);

    IndexT i;
    for (i = 0; i < arr.Size(); i++)
    {
        buf[i] = this->byteOrder.Convert<int>(arr[i]);
    }

    // first write size
    this->WriteInt(arr.Size());

    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + writeSize) <= this->mapEnd);
        Memory::Copy(buf, this->mapCursor, writeSize);
        this->mapCursor += writeSize;
    }
    else
    {
        this->stream->Write(buf, writeSize);
    }
    Memory::Free(Memory::ScratchHeap, (void*)buf);
}

//------------------------------------------------------------------------------
/**
*/
void 
BinaryWriter::WriteBoolArray(const Util::Array<bool>& arr)
{
    const SizeT writeSize = sizeof(bool) * arr.Size(); 
    bool* buf = (bool*)Memory::Alloc(Memory::ScratchHeap, writeSize);

    IndexT i;
    for (i = 0; i < arr.Size(); i++)
    {
        buf[i] = arr[i];
    }

    // first write size
    this->WriteInt(arr.Size());

    if (this->isMapped)
    {
        // note: the memory copy is necessary to circumvent alignment problem on some CPUs
        n_assert((this->mapCursor + writeSize) <= this->mapEnd);
        Memory::Copy(buf, this->mapCursor, writeSize);
        this->mapCursor += writeSize;
    }
    else
    {
        this->stream->Write(buf, writeSize);
    }
    Memory::Free(Memory::ScratchHeap, (void*)buf);
}  

//------------------------------------------------------------------------------
/**
*/
void
BinaryWriter::WriteRawData(const void* ptr, SizeT numBytes)
{
    n_assert((ptr != 0) && (numBytes > 0));
    if (this->isMapped)
    {
        n_assert((this->mapCursor + numBytes) <= this->mapEnd);
        Memory::Copy(ptr, this->mapCursor, numBytes);
        this->mapCursor += numBytes;
    }
    else
    {
        this->stream->Write(ptr, numBytes);
    }
}

} // namespace IO
