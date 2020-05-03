//------------------------------------------------------------------------------
//  bxmlreadertest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "bxmlreadertest.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/bxmlreader.h"

namespace Test
{
__ImplementClass(Test::BXmlReaderTest, 'BXLT', Test::TestCase);

using namespace IO;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
BXmlReaderTest::Run()
{
    Ptr<IoServer> ioServer = IoServer::Create();
    Ptr<Stream> stream = ioServer->CreateStream("home:work/testdata/modelloadsavetest.bxml");
    Ptr<BXmlReader> reader = BXmlReader::Create();
    reader->SetStream(stream);
    
    VERIFY(reader->Open());
    VERIFY(reader->GetCurrentNodeName() == "Nebula");
    VERIFY(reader->HasNode("/Nebula/Model"));
    VERIFY(reader->HasNode("Model"));
    VERIFY(reader->HasNode("/Nebula/Model/Attr"));
    VERIFY(reader->HasNode("/Nebula/Model/Node"));
    VERIFY(reader->HasNode("Model/Node"));

    reader->SetToNode("Model/Node");
    VERIFY(reader->GetCurrentNodeName() == "Node");
    VERIFY(reader->GetCurrentNodePath() == "/Nebula/Model/Node");
    VERIFY(reader->HasAttr("name"));
    VERIFY(reader->HasAttr("class"));
    VERIFY(reader->GetString("name") == "Node0");
    VERIFY(reader->GetString("class") == "Scene::ModelNode");
    VERIFY(reader->GetOptString("bla", "blub") == "blub");

    reader->SetToNode("/Nebula/Model");
    VERIFY(reader->SetToFirstChild("Attr"));
    VERIFY(reader->GetString("name") == "ModelPos");
    VERIFY(reader->SetToNextChild("Attr"));
    VERIFY(reader->GetString("name") == "ModelInt");
    VERIFY(reader->SetToNextChild("Attr"));
    VERIFY(reader->GetString("name") == "ModelBool");
    VERIFY(reader->SetToNextChild("Attr"));
    VERIFY(reader->GetString("name") == "ModelString");
    VERIFY(!reader->SetToNextChild("Attr"));
    VERIFY(reader->GetCurrentNodeName() == "Model");

    reader->SetToNode("/Nebula/Model");
    VERIFY(reader->SetToFirstChild("Node"));
    VERIFY(reader->GetString("name") == "Node0");
    VERIFY(reader->SetToNextChild("Node"));
    VERIFY(reader->GetString("name") == "Node1");
    VERIFY(reader->SetToNextChild("Node"));
    VERIFY(reader->GetString("name") == "Node2");
    VERIFY(!reader->SetToNextChild("Node"));
    VERIFY(reader->GetCurrentNodePath() == "/Nebula/Model");

    reader->SetToNode("/Nebula/Model");
    VERIFY(reader->SetToFirstChild());
    VERIFY(reader->SetToNextChild());
    VERIFY(reader->SetToNextChild());
    VERIFY(reader->SetToNextChild());
    VERIFY(reader->SetToNextChild());
    VERIFY(reader->GetCurrentNodePath() == "/Nebula/Model/Node");
    VERIFY(reader->GetString("name") == "Node0");
    VERIFY(reader->SetToNextChild());
    
    Array<String> attrs = reader->GetAttrs();
    VERIFY(attrs.Size() == 3);
    VERIFY(attrs[0] == "name");
    VERIFY(attrs[1] == "class");
    VERIFY(attrs[2] == "parent");
    VERIFY(reader->GetString("parent") == "Node0");

    reader->Close();
}

} // namespace Test