//------------------------------------------------------------------------------
//  debugpacket.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/debugpacket.h"
#include "system/byteorder.h"

namespace Net
{

using namespace Util;
using namespace IO;
using namespace System;

//------------------------------------------------------------------------------
/**
*/
DebugPacket::DebugPacket() :
    hasData(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
DebugPacket::~DebugPacket()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Write data to the packet. Returns the number of data written. If all
    data could be written, the returned number is identical with the
    requested size, and the packet's payload size will be set to that size.
    This means the packet completes a message.
    If not all data could be written, the actually written size (MaxPayloadSize)
    will be returned, and the internal payloadSize will be set to 0xFFFF, meaning
    that the packet contains incomplete data which will be continued with the
    next packet.
*/
SizeT
DebugPacket::Write(ushort portNum, uint packetCount, const ubyte* buf, SizeT numBytes)
{
    n_assert(0 != buf);
    n_assert(numBytes > 0);
    n_assert(!this->hasData);    
    this->hasData = true;

    ByteOrder byteOrder(ByteOrder::Host, ByteOrder::Network);
    union { ubyte *b; uint *u; ushort *us; } pun;
    pun.b = this->buffer;
    uint* uintHeader = pun.u;
    ushort* ushortHeader = pun.us;
    ubyte* payloadPtr = this->buffer + HeaderSize;
    
    SizeT numActualBytes = numBytes < MaxPayloadSize ? numBytes : MaxPayloadSize;
    if (numActualBytes < numBytes)
    {    
        // setup the header for an incomplete packet
        uintHeader[0]   = byteOrder.Convert<uint>(FourCC('DPKT').AsUInt());
        uintHeader[1]   = byteOrder.Convert<uint>(packetCount);
        ushortHeader[4] = byteOrder.Convert<ushort>(portNum);
        ushortHeader[5] = byteOrder.Convert<ushort>(0xFFFF);
    }
    else
    {
        // setup the header for a complete packet
        uintHeader[0]   = byteOrder.Convert<uint>(FourCC('DPKT').AsUInt());
        uintHeader[1]   = byteOrder.Convert<uint>(packetCount);
        ushortHeader[4] = byteOrder.Convert<ushort>(portNum);
        ushortHeader[5] = byteOrder.Convert<ushort>((ushort)numActualBytes);
    }

    // copy the payload
    Memory::Copy(buf, payloadPtr, numActualBytes);
    
    // return number of bytes written
    return numActualBytes;
}

//------------------------------------------------------------------------------
/**
    Write raw data to the packet. The source buffer must contain a valid
    packet header and the size of the source data must equal 
    DebugPacket::PacketSize!
*/
void
DebugPacket::WriteRaw(const void* buf, SizeT bufSize)
{
    n_assert(PacketSize == bufSize);
    Memory::Copy(buf, this->buffer, PacketSize);
    this->hasData = true;
}

//------------------------------------------------------------------------------
/**
    Get the magic code at the beginning of the message. Must be
    FourCC('DPKT').
*/
FourCC
DebugPacket::GetMagic() const
{
    n_assert(this->hasData);
    ByteOrder byteOrder(ByteOrder::Network, ByteOrder::Host);
    union pun { const uint *u; const ubyte *b; } pun;
    pun.b = this->buffer;
    FourCC magic = byteOrder.Convert<uint>(pun.u[0]);
    return magic;
}

//------------------------------------------------------------------------------
/**
    Get the packet counter of this packet.
*/
uint
DebugPacket::GetCount() const
{
    n_assert(this->hasData);
    ByteOrder byteOrder(ByteOrder::Network, ByteOrder::Host);
    union pun { const uint *u; const ubyte *b; } pun;
    pun.b = this->buffer;
    uint count = byteOrder.Convert<uint>(pun.u[1]);
    return count;
}

//------------------------------------------------------------------------------
/**
    Get the port number from the packet.
*/
ushort
DebugPacket::GetPort() const
{
    n_assert(this->hasData);
    ByteOrder byteOrder(ByteOrder::Network, ByteOrder::Host);
    union pun { const ushort *us; const ubyte *b; } pun;
    pun.b = this->buffer;
    ushort portNum = byteOrder.Convert<ushort>(pun.us[4]);
    return portNum;
}

//------------------------------------------------------------------------------
/**
    Return true if this is the final packet in a multi-packet message.
*/
bool
DebugPacket::IsFinalPacket() const
{
    n_assert(this->hasData);
    ByteOrder byteOrder(ByteOrder::Network, ByteOrder::Host);
    union pun { const ushort *us; const ubyte *b; } pun;
    pun.b = this->buffer; 
    ushort payloadSize = byteOrder.Convert<ushort>(pun.us[5]);
    return (0xFFFF != payloadSize);
}

//------------------------------------------------------------------------------
/**
    Returns the number of payload data bytes in the packet. This will
    always return a valid number, even if the payloadSize header member is
    set to the special 0xFFFF code.
*/
SizeT
DebugPacket::GetPayloadSize() const
{
    n_assert(this->hasData);
    ByteOrder byteOrder(ByteOrder::Network, ByteOrder::Host);
    union pun { const ushort *us; const ubyte *b; } pun;
    pun.b = this->buffer; 
    ushort payloadSize = byteOrder.Convert<ushort>(pun.us[5]);
    if (0xFFFF == payloadSize)
    {
        payloadSize = MaxPayloadSize;
    }
    return payloadSize;
}

//------------------------------------------------------------------------------
/**
    Get a pointer to the actual payload data.
*/
const ubyte*
DebugPacket::GetPayload() const
{
    n_assert(this->hasData);
    return this->buffer + HeaderSize;
}

//------------------------------------------------------------------------------
/**
    Get a pointer to the raw packet data.
*/
const ubyte*
DebugPacket::GetRawBuffer() const
{
    return this->buffer;
}

//------------------------------------------------------------------------------
/**
*/
Array<DebugPacket>
DebugPacket::EncodeStream(ushort portNum, uint packetCounter, const Ptr<Stream>& stream)
{
    n_assert(stream->CanBeMapped());

    Array<DebugPacket> result;
    SizeT bytesRemaining = stream->GetSize();
    stream->SetAccessMode(Stream::ReadAccess);
    bool streamOpenResult = stream->Open();
    n_assert(streamOpenResult);
    uchar* srcPtr = (uchar*) stream->Map();
    do
    {
        DebugPacket packet;
    
        // write next chunk of data 
        SizeT bytesWritten = packet.Write(portNum, packetCounter++, srcPtr, bytesRemaining);
        srcPtr += bytesWritten;
        bytesRemaining -= bytesWritten;

        // add packet to result
        result.Append(packet);
    }
    while (bytesRemaining > 0);
    stream->Unmap();
    stream->Close();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
void
DebugPacket::DecodePackets(const Array<DebugPacket>& packets,
                           const Ptr<Stream>& stream,
                           ushort& outPortNum)
{
    n_assert(packets.Back().IsFinalPacket());
    n_assert(packets.Size() > 0);

    // read port number from first packet
    outPortNum = packets[0].GetPort();

    // open stream for writing and iterate over packets
    stream->SetAccessMode(Stream::WriteAccess);
    bool streamOpenResult = stream->Open();
    n_assert(streamOpenResult);
    IndexT i;
    for (i = 0; i < packets.Size(); i++)
    {
        const DebugPacket& curPacket = packets[i];
        n_assert(curPacket.HasData());
        if (i < (packets.Size() - 1))
        {
            // make sure there aren't any final packets inbetween
            n_assert(!curPacket.IsFinalPacket());
        }

        // make sure the port number is consistent across all packets
        n_assert(curPacket.GetPort() == outPortNum);

        // copy the packets payload into the stream
        stream->Write(curPacket.GetPayload(), curPacket.GetPayloadSize());
    }
    stream->Close();
}

} // namespace Net