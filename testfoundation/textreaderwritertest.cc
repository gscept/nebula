//------------------------------------------------------------------------------
//  textreaderwritertest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "textreaderwritertest.h"
#include "io/ioserver.h"
#include "io/memorystream.h"
#include "io/textreader.h"
#include "io/textwriter.h"

namespace Test
{
__ImplementClass(Test::TextReaderWriterTest, 'TRWT', Test::TestCase);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
void
TextReaderWriterTest::Run()
{
    // create necessary objects...
    Ptr<IoServer> ioServer = IoServer::Create();
    Ptr<MemoryStream> stream = MemoryStream::Create();
    Ptr<TextReader> reader = TextReader::Create();
    Ptr<TextWriter> writer = TextWriter::Create();

    // create a file stream, text writer and text reader
    reader->SetStream(stream.upcast<Stream>());
    writer->SetStream(stream.upcast<Stream>());

    // write some stuff to the stream
    stream->SetAccessMode(Stream::WriteAccess);
    this->Verify(stream->Open());
    this->Verify(writer->Open());
    writer->WriteChar('A');
    writer->WriteChar('B');
    writer->WriteChar('C');
    writer->WriteString(" abc");
    writer->WriteChar('\n');
    writer->WriteLine("The first line.");
    writer->WriteLine("The second line, this is a verrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrryyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong line!");
    Array<String> lines;
    lines.Append("The first array line.");
    lines.Append("The second array line.");
    lines.Append("The third array line.");
    lines.Append("This is the last line.");
    writer->WriteLines(lines);
    writer->WriteString("Something without a newline at the end...");
    writer->Close();
    stream->Close();

    // read back stuff...
    stream->SetAccessMode(Stream::ReadAccess);
    this->Verify(stream->Open());
    this->Verify(reader->Open());
    lines = reader->ReadAllLines();
    reader->Close();
    stream->Close();

    // verify result...
    this->Verify(lines.Size() == 8);
    this->Verify(lines[0] == "ABC abc");
    this->Verify(lines[1] == "The first line.");
    this->Verify(lines[2] == "The second line, this is a verrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrryyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong line!");
    this->Verify(lines[3] == "The first array line.");
    this->Verify(lines[4] == "The second array line.");
    this->Verify(lines[5] == "The third array line.");
    this->Verify(lines[6] == "This is the last line.");
    this->Verify(lines[7] == "Something without a newline at the end...");
}

}; // namespace Test
