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
#include "math/vec4.h"
#include "math/mat4.h"

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
    VERIFY(stream->Open());
    VERIFY(writer->Open());
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
        writer->SetVec4("attr1", vec4(4.0f, 5.0f, 6.0f, 7.0f));
        writer->EndNode();

        writer->BeginNode("Node3");
        mat4 m;
        vec4 value(10.0,20.0, 30.0, 1.0f);
        m.row3 = value;
        writer->SetMat4("attr", m);
        writer->EndNode();
    }
    writer->EndNode();
    writer->Close();
    stream->Close();

    // read the just written file with a stream reader and verify its contents
    stream->SetAccessMode(Stream::ReadAccess);
    VERIFY(stream->Open());
    VERIFY(reader->Open());
    VERIFY(reader->GetCurrentNodeName() == "RootNode");
    VERIFY(reader->HasNode("/RootNode/Node0"));
    VERIFY(reader->HasNode("Node1"));
    VERIFY(reader->HasNode("/RootNode/Node2"));
    VERIFY(reader->HasNode("/RootNode/Node3"));
    
    VERIFY(reader->SetToFirstChild());
    VERIFY(reader->GetCurrentNodeName() == "Node0");
    VERIFY(reader->HasAttr("attr0"));
    VERIFY(reader->HasAttr("attr1"));
    VERIFY(!reader->HasAttr("bla"));
    VERIFY(reader->GetBool("attr0") == false);
    VERIFY(reader->GetBool("attr1") == true);

    VERIFY(reader->SetToNextChild());
    VERIFY(reader->GetCurrentNodePath() == "/RootNode/Node1");
    VERIFY(reader->HasAttr("attr"));
    VERIFY(reader->GetFloat("attr") == 123.4f);

    reader->SetToNode("/RootNode/Node2");
    VERIFY(reader->GetCurrentNodeName() == "Node2");
    VERIFY(reader->HasAttr("attr1"));
    VERIFY(reader->GetVec4("attr1") == vec4(4.0f, 5.0f, 6.0f, 7.0f));

    VERIFY(reader->SetToNextChild());
    VERIFY(reader->HasAttr("attr"));
    VERIFY(!reader->SetToNextChild());

    reader->Close();
    stream->Close();
}

}
