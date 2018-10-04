#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::DebugMessage
    
    Encapsulates a stream and a port number for debug communication. Currently
    this class is only used on the Wii for communication over the HIO2
    channel, but is public for the PC proxy tools communicating with the
    Wii devkit.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "io/stream.h"

//------------------------------------------------------------------------------
namespace Net
{
class DebugMessage
{
public:
    /// default constructor
    DebugMessage();
    /// constructor
    DebugMessage(ushort port, const Ptr<IO::Stream>& data);
    /// get the port number
    ushort GetPort() const;
    /// get the data stream
    const Ptr<IO::Stream>& GetStream() const;
    /// return true if the message is valid
    bool IsValid() const;
    
private:
    ushort portNum;
    Ptr<IO::Stream> dataStream;
};

//------------------------------------------------------------------------------
/**
*/
inline 
DebugMessage::DebugMessage() :
    portNum(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
DebugMessage::DebugMessage(ushort p, const Ptr<IO::Stream>& s) :
    portNum(p),
    dataStream(s)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline ushort
DebugMessage::GetPort() const
{
    return this->portNum;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<IO::Stream>&
DebugMessage::GetStream() const
{
    return this->dataStream;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
DebugMessage::IsValid() const
{
    return this->dataStream.isvalid();
}

} // namespace Net
//------------------------------------------------------------------------------

