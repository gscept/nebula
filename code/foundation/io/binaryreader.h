#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::BinaryReader
    
    A friendly interface to read binary data from a stream. Optionally the
    reader can use memory mapping for optimal read performance. Performs
    automatic byte order conversion if necessary.

    @todo convert endianess!
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"
#if !__OSX__
#include "math/vec2.h"
#include "math/vec4.h"
#include "math/vec3.h"
#include "math/mat4.h"
#endif
#include "util/guid.h"
#include "util/blob.h"
#include "system/byteorder.h"

//------------------------------------------------------------------------------
namespace IO
{
class BinaryReader : public StreamReader
{
    __DeclareClass(BinaryReader);
public:        
    /// constructor
    BinaryReader();
    /// destructor
    virtual ~BinaryReader();
    /// call before Open() to enable memory mapping (if stream supports mapping)
    void SetMemoryMappingEnabled(bool b);
    /// return true if memory mapping is enabled
    bool IsMemoryMappingEnabled() const;
    /// set the stream byte order (default is host byte order)
    void SetStreamByteOrder(System::ByteOrder::Type byteOrder);
    /// get the stream byte order
    System::ByteOrder::Type GetStreamByteOrder() const;
    /// begin reading from the stream
    virtual bool Open();
    /// end reading from the stream
    virtual void Close();
    /// read an 8-bit char from the stream
    char ReadChar();
    /// read an 8-bit unsigned character from the stream
    unsigned char ReadUChar();
    /// read a 16-bit short from the stream
    short ReadShort();
    /// read a 16-bit unsigned short from the stream
    unsigned short ReadUShort();
    /// read a 32-bit int from the stream
    int ReadInt();
    /// read a 32-bit unsigned int from the stream
    unsigned int ReadUInt();
    /// read a 64-bit int from the stream
	long long ReadInt64();
    /// read a 64-bit unsigned int from the stream
	unsigned long long ReadUInt64();
    /// read a float value from the stream
    float ReadFloat();
    /// read a compressed float value from the stream, lossy and needed to be in the range of -1.0 and +1.0
    float ReadFloatFromNormalizedUByte2();
    /// read a compressed float value from the stream, lossy and needed to be in the range of 0.0 and +1.0
    float ReadFloatFromUnsignedNormalizedUByte2();
    /// read a double value from the stream
    double ReadDouble();
    /// read a bool value from the stream
    bool ReadBool();
    /// read a string from the stream
    Util::String ReadString();
    #if !__OSX__
    /// read a vec2 from the stream
    Math::vec2 ReadFloat2();
    /// read a vector from the stream, (x,y,z,0.0)
    Math::vec3 ReadVec3();
    /// read a vec4 from the stream
    Math::vec4 ReadVec4();
    /// read a mat4 from the stream
    Math::mat4 ReadMat4();
    #endif
    /// read a float array from the stream
    Util::Array<float> ReadFloatArray();
    /// read an int array from the stream
    Util::Array<int> ReadIntArray();
	/// read an int array from the stream
	Util::Array<uint> ReadUIntArray();
    /// read a bool array from the stream
    Util::Array<bool> ReadBoolArray();
    /// read a guid
    Util::Guid ReadGuid();
    /// read a blob of data
    Util::Blob ReadBlob();
    /// read raw data
    void ReadRawData(void* ptr, SizeT numBytes);

public:
    bool enableMapping;
    bool isMapped;
    System::ByteOrder byteOrder;
    unsigned char* mapCursor;
    unsigned char* mapEnd;
};

//------------------------------------------------------------------------------
/**
*/
inline void
BinaryReader::SetMemoryMappingEnabled(bool b)
{
    this->enableMapping = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
BinaryReader::IsMemoryMappingEnabled() const
{
    return this->enableMapping;
}

//------------------------------------------------------------------------------
/**
*/
inline void
BinaryReader::SetStreamByteOrder(System::ByteOrder::Type order)
{
    this->byteOrder.SetFromByteOrder(order);
}

//------------------------------------------------------------------------------
/**
*/
inline System::ByteOrder::Type
BinaryReader::GetStreamByteOrder() const
{
    return this->byteOrder.GetFromByteOrder();
}

} // namespace IO
//------------------------------------------------------------------------------
