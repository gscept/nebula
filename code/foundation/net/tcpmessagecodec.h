#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::TcpMessageCodec

    Helperclass that provides function to encode and decode sreams into messages.
    
    The encoder adds header informations at the beginning of the stream,
    that includes the complete size of the stream. Call EncodeToMessage at the
    sending site.

    The decoder reads sequential incoming streams and append the data
    to an internal buffer. It concatenates the incoming streams to
    a list of streams containing complete messages.
    Call DecodeStream at the receiving site and check for completed messsages
    with GetMessages.

    (C) 2009 Radon Labs
	(C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/memorystream.h"
#include "io/binaryreader.h"

//------------------------------------------------------------------------------
namespace Net
{
class TcpMessageCodec
{
public:
    /// Constructor
    TcpMessageCodec();
    /// Destructor
    virtual ~TcpMessageCodec();

    /// Attachs header information to the stream and returns a copy with header
    void EncodeToMessage(const Ptr<IO::Stream> & stream, const Ptr<IO::Stream> &output);  
    /// Decodes a given Stream. Check for HasMessages() if this completes a message.
    void DecodeStream(const Ptr<IO::Stream> & stream);
    /// Returns true, if there are messages in the internal message queue.
    bool HasMessages();
    /// Gets the list of all created messages since the last call of this function
    Util::Array<Ptr<IO::Stream> > DequeueMessages();

private: 
    /// what data is currently expected from the decoder
    enum ReceiveState
    {
        HeaderData,
        MessageData
    };
    
    ReceiveState receiveState;
    Ptr<IO::Stream> headerStream;
    Ptr<IO::Stream> messageStream;
    SizeT messageSize;
    IndexT headerPosition;
    IndexT messagePostition;
    Util::Array<Ptr<IO::Stream> > completedMessages;
};
//------------------------------------------------------------------------------
} // namespace Net