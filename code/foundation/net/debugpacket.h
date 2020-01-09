#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::DebugPacket
    
    Encapsulates a data packet for debug communication. Currently this class
    is only used on the Wii to communicate on the host PC, with the Wii
    specific classes hidden away in the Wii port of N3. DebugPacket is
    "public" because it's also used in Win32 proxy tools.
    
    Every packet consists of a 4-byte header, and 1020 bytes of playload
    data, so that each packet is exactly 1 KByte long.
    
    A packet looks like this:
    
    uint magic;         // FourCC('DPKT')
    uint count;         // message counter
    ushort port;        // port number
    ushort payloadSize; // 0xFFFF if packet is full and more data follows, else size of data in packet
    ubyte payload[PacketSize - HeaderSize)

    The payloadSize must be interpreted like this:
    
    0xFFFF: full payload of data, current message is continued in next package
    else:   payloadSize is number of bytes in packet, current message is complete
        
    IMPORTANT NOTE:
    
    The DebugPacket does not allocate any external memory, and thus can be safely used
    from Wii interrupt handlers.    
        
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/stream.h"

//------------------------------------------------------------------------------
namespace Net
{
class DebugPacket
{
public:
    /// packet commands
    enum Command
    {
        PC2WiiCmd,      // message from PC to Wii has been sent
        Wii2PCCmd,      // message from Wii to PC has been sent
        PC2WiiAck,      // Wii-2-PC message has arrived on PC
        Wii2PCAck,      // PC-2-Wii message has arrived on Wii
    };

    /// packet size
    static const SizeT PacketSize = 512;
    /// header size
    static const SizeT HeaderSize = 12;
    /// max payload size
    static const SizeT MaxPayloadSize = PacketSize - HeaderSize;

    /// constructor
    DebugPacket();
    /// destructor
    virtual ~DebugPacket();
    
    /// encode stream into a series of packets
    static Util::Array<DebugPacket> EncodeStream(ushort portNum, uint firstPacketCounter, const Ptr<IO::Stream>& stream);
    /// decode a series of packets into a stream
    static void DecodePackets(const Util::Array<DebugPacket>& packets, const Ptr<IO::Stream>& stream, ushort& outPortNum);

    /// write data to packet, returns number of bytes written, all data must be written in a single call!
    SizeT Write(ushort portNum, uint packetCounter, const ubyte* buf, SizeT numBytes);
    /// write raw data to packet (must have valid packet header, size of src buffer must be PacketSize!
    void WriteRaw(const void* buf, SizeT bufSize);

    /// set the "has data" flag on the packet
    void SetDataValid(bool b);    
    /// return true if the packet contains data
    bool HasData() const;
    /// get magic code at start of header
    Util::FourCC GetMagic() const;
    /// get message counter
    uint GetCount() const;
    /// get the port number of the packet
    ushort GetPort() const;
    /// return true if this is the last packet of a message
    bool IsFinalPacket() const;
    /// get actual payload size
    SizeT GetPayloadSize() const;
    /// get pointer to payload
    const ubyte* GetPayload() const;
    /// get pointer to raw packet buffer (contains header + payload)
    const ubyte* GetRawBuffer() const;
    
private:
    ubyte buffer[PacketSize];
    bool hasData;
};

//------------------------------------------------------------------------------
/**
*/
inline void
DebugPacket::SetDataValid(bool b)
{
    this->hasData = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
DebugPacket::HasData() const
{    
    return this->hasData;
}

} // namespace Net
//------------------------------------------------------------------------------

