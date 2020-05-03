//------------------------------------------------------------------------------
//  jsonreaderwritertest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "pjson/pjson.h"
#include "stdneb.h"
#include "jsonreaderwritertest.h"
#include "io/ioserver.h"
#include "io/filestream.h"
#include "io/memorystream.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"

namespace Test
{
__ImplementClass(Test::JSonReaderWriterTest, 'JRWT', Test::TestCase);

using namespace IO;
using namespace Util;
using namespace Math;


//------------------------------------------------------------------------------
/**
*/
void
JSonReaderWriterTest::Run()
{   
    // create necessary objects
    Ptr<IoServer> ioServer = IoServer::Create();
    Ptr<MemoryStream> ostream = MemoryStream::Create();
    
    Ptr<JsonWriter> writer = JsonWriter::Create();
    writer->SetStream(ostream);
    VERIFY(writer->Open());
    VERIFY(writer->IsOpen());

    writer->BeginArray("array");
    writer->BeginObject();
    writer->Add(5,"zahl");
    writer->Add("hallo","string");
    writer->Add(Math::vec4(1.5f),"float4");
    writer->End();
    writer->Add(6);
    writer->End();
    writer->Close();
    ostream->Open();
    void * buffer = ostream->Map();
    n_printf("%s", (char*)buffer);
    
    
    Ptr<Stream> stream = ioServer->CreateStream(URI("bin:jsontest.json"));    
    Ptr<JsonReader> reader = JsonReader::Create();
    
    reader->SetStream(stream);
       
    stream->SetAccessMode(Stream::ReadAccess);
    VERIFY(stream->Open());
    VERIFY(reader->Open());    
    VERIFY(reader->HasNode("/colors"));
    VERIFY(reader->SetToFirstChild());
    VERIFY(!reader->HasNode("colors"));        
    VERIFY(reader->SetToFirstChild());    
    VERIFY(reader->HasAttr("color"));
    VERIFY(reader->HasAttr("code"));
    VERIFY(!reader->HasAttr("bla"));
    VERIFY(reader->GetString("color") == "black");
    VERIFY(reader->SetToFirstChild("code"));
    vec4 col = reader->GetVec4("rgba");
    VERIFY(col == vec4(255, 255, 255, 1));
    Util::Array<float> rgba;
    reader->Get<Util::Array<float>>(rgba, "rgba");
    bool same = true;
    for (int i = 0; i < 4; i++) same &= col[i] == rgba[i];
    VERIFY(same);
    VERIFY(reader->SetToParent());
    VERIFY(reader->SetToNextChild());
    VERIFY(reader->HasAttr("color"));
    VERIFY(reader->GetString("color") == "white");
    
    reader->SetToNode("/colors/[3]");
    VERIFY(reader->GetString("color") == "blue");
    reader->SetToNode("[3]/rgba");
    VERIFY(reader->GetVec4() == vec4(0, 0, 255, 1));
    
    reader->Close();
    stream->Close();
}

}
