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
    
    this->Verify(reader->Open());
    this->Verify(reader->GetCurrentNodeName() == "Nebula3");
    this->Verify(reader->HasNode("/Nebula3/Model"));
    this->Verify(reader->HasNode("Model"));
    this->Verify(reader->HasNode("/Nebula3/Model/Attr"));
    this->Verify(reader->HasNode("/Nebula3/Model/Node"));
    this->Verify(reader->HasNode("Model/Node"));

    reader->SetToNode("Model/Node");
    this->Verify(reader->GetCurrentNodeName() == "Node");
    this->Verify(reader->GetCurrentNodePath() == "/Nebula3/Model/Node");
    this->Verify(reader->HasAttr("name"));
    this->Verify(reader->HasAttr("class"));
    this->Verify(reader->GetString("name") == "Node0");
    this->Verify(reader->GetString("class") == "Scene::ModelNode");
    this->Verify(reader->GetOptString("bla", "blub") == "blub");

    reader->SetToNode("/Nebula3/Model");
    this->Verify(reader->SetToFirstChild("Attr"));
    this->Verify(reader->GetString("name") == "ModelPos");
    this->Verify(reader->SetToNextChild("Attr"));
    this->Verify(reader->GetString("name") == "ModelInt");
    this->Verify(reader->SetToNextChild("Attr"));
    this->Verify(reader->GetString("name") == "ModelBool");
    this->Verify(reader->SetToNextChild("Attr"));
    this->Verify(reader->GetString("name") == "ModelString");
    this->Verify(!reader->SetToNextChild("Attr"));
    this->Verify(reader->GetCurrentNodeName() == "Model");

    reader->SetToNode("/Nebula3/Model");
    this->Verify(reader->SetToFirstChild("Node"));
    this->Verify(reader->GetString("name") == "Node0");
    this->Verify(reader->SetToNextChild("Node"));
    this->Verify(reader->GetString("name") == "Node1");
    this->Verify(reader->SetToNextChild("Node"));
    this->Verify(reader->GetString("name") == "Node2");
    this->Verify(!reader->SetToNextChild("Node"));
    this->Verify(reader->GetCurrentNodePath() == "/Nebula3/Model");

    reader->SetToNode("/Nebula3/Model");
    this->Verify(reader->SetToFirstChild());
    this->Verify(reader->SetToNextChild());
    this->Verify(reader->SetToNextChild());
    this->Verify(reader->SetToNextChild());
    this->Verify(reader->SetToNextChild());
    this->Verify(reader->GetCurrentNodePath() == "/Nebula3/Model/Node");
    this->Verify(reader->GetString("name") == "Node0");
    this->Verify(reader->SetToNextChild());
    
    Array<String> attrs = reader->GetAttrs();
    this->Verify(attrs.Size() == 3);
    this->Verify(attrs[0] == "name");
    this->Verify(attrs[1] == "class");
    this->Verify(attrs[2] == "parent");
    this->Verify(reader->GetString("parent") == "Node0");

    reader->Close();
}

} // namespace Test