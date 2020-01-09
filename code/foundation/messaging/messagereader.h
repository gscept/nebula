#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::MessageReader
    
    Implements a binary stream protocol for decoding messages from
    a stream.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/binaryreader.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class Message;

class MessageReader : public IO::StreamReader
{
    __DeclareClass(MessageReader);
public:
    /// constructor
    MessageReader();
    /// set stream to read from
    virtual void SetStream(const Ptr<IO::Stream>& s);
    /// decode a new message from the stream
    Message* ReadMessage();
private:
    Ptr<IO::BinaryReader> binaryReader;
};

} // namespace Messaging
//------------------------------------------------------------------------------
