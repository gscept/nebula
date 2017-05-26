//------------------------------------------------------------------------------
//  xmlreaderwritertest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "xmlreaderwritertest.h"
#include "io/ioserver.h"
#include "io/memorystream.h"
#include "io/xmlreader.h"
#include "io/xmlwriter.h"

namespace Test
{
__ImplementClass(Test::XmlReaderWriterTest, 'XRWT', Test::TestCase);

using namespace IO;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
XmlReaderWriterTest::Run()
{
    // create necessary objects
    Ptr<IoServer> ioServer = IoServer::Create();
    Ptr<MemoryStream> stream = MemoryStream::Create();
    Ptr<XmlWriter> writer = XmlWriter::Create();
    Ptr<XmlReader> reader = XmlReader::Create();

    // create an XML file
    writer->SetStream(stream.upcast<Stream>());
    reader->SetStream(stream.upcast<Stream>());

    // write some stuff to the file
    stream->SetAccessMode(Stream::WriteAccess);
    this->Verify(stream->Open());
    this->Verify(writer->Open());
    writer->BeginNode("RootNode");
    writer->SetString("rootString", "Bla");
    writer->SetInt("rootInt", 123);
    {    
        writer->BeginNode("Node0");
        writer->SetBool("attr0", false);
        writer->SetBool("attr1", true);
        writer->EndNode();

        writer->BeginNode("Node1");
        writer->SetFloat("attr", 123.4f);
        writer->EndNode();

        writer->BeginNode("Node2");
        writer->SetFloat4("attr1", float4(4.0f, 5.0f, 6.0f, 7.0f));
        writer->EndNode();

        writer->BeginNode("Node3");
        matrix44 m = matrix44::identity();
        float4 value(10.0,20.0, 30.0, 1.0f);
        m.setrow3(value);
        writer->SetMatrix44("attr", m);
        writer->EndNode();
    }
    writer->EndNode();
    writer->Close();
    stream->Close();

    // read the just written file with a stream reader and verify its contents
    stream->SetAccessMode(Stream::ReadAccess);
    this->Verify(stream->Open());
    this->Verify(reader->Open());
    this->Verify(reader->GetCurrentNodeName() == "RootNode");
    this->Verify(reader->HasNode("/RootNode/Node0"));
    this->Verify(reader->HasNode("Node1"));
    this->Verify(reader->HasNode("/RootNode/Node2"));
    this->Verify(reader->HasNode("/RootNode/Node3"));
    
    this->Verify(reader->SetToFirstChild());
    this->Verify(reader->GetCurrentNodeName() == "Node0");
    this->Verify(reader->HasAttr("attr0"));
    this->Verify(reader->HasAttr("attr1"));
    this->Verify(!reader->HasAttr("bla"));
    this->Verify(reader->GetBool("attr0") == false);
    this->Verify(reader->GetBool("attr1") == true);

    this->Verify(reader->SetToNextChild());
    this->Verify(reader->GetCurrentNodePath() == "/RootNode/Node1");
    this->Verify(reader->HasAttr("attr"));
    this->Verify(reader->GetFloat("attr") == 123.4f);

    reader->SetToNode("/RootNode/Node2");
    this->Verify(reader->GetCurrentNodeName() == "Node2");
    this->Verify(reader->HasAttr("attr1"));
    this->Verify(reader->GetFloat4("attr1") == float4(4.0f, 5.0f, 6.0f, 7.0f));

    this->Verify(reader->SetToNextChild());
    this->Verify(reader->HasAttr("attr"));
    this->Verify(!reader->SetToNextChild());

    reader->Close();
    stream->Close();
}

}
