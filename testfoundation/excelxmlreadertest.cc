//------------------------------------------------------------------------------
//  excelxmlreadertest.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "excelxmlreadertest.h"
#include "io/excelxmlreader.h"
#include "io/ioserver.h"
#include "io/uri.h"
#include "io/filestream.h"

namespace Test
{
__ImplementClass(Test::ExcelXmlReaderTest, 'exrt', Test::TestCase);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
ExcelXmlReaderTest::Run()
{
    // create necessary objects
    Ptr<IoServer> ioServer = IoServer::Create();
    ioServer->MountStandardArchives();
    Ptr<Stream> stream = ioServer->CreateStream("home:work/testdata/excelxmlreadertest.xml");
    Ptr<ExcelXmlReader> reader = ExcelXmlReader::Create();

    // load a prepared Excel file
    reader->SetStream(stream);
    bool success = reader->Open();
    this->Verify(success);
    if (success)
    {
        this->Verify(reader->GetNumTables() == 2);
        this->Verify(reader->GetTableName() == "table0");
        this->Verify(reader->GetTableName(1) == "table1");
        this->Verify(reader->GetNumRows() == 3);
        this->Verify(reader->GetNumRows(1) == 5);
        this->Verify(reader->GetNumColumns() == 2);
        this->Verify(reader->GetNumColumns(1) == 3);
        this->Verify(reader->HasColumn("T0C0"));
        this->Verify(reader->HasColumn("T0C1"));
        this->Verify(reader->HasColumn("T1C0", 1));
        this->Verify(reader->HasColumn("T1C1", 1));
        this->Verify(reader->HasColumn("T1C2", 1));
        this->Verify(InvalidIndex == reader->FindColumnIndex("T0C2"));
        this->Verify(1 == reader->FindColumnIndex("T0C1"));
        this->Verify(2 == reader->FindColumnIndex("T1C2", 1));
        this->Verify(reader->GetElement(1, 0) == "T0C0R1");
        this->Verify(reader->GetElement(2, 0) == "T0C0R2");
        this->Verify(reader->GetElement(1, 1) == "T0C1R1");
        this->Verify(reader->GetElement(2, 1) == "T0C1R2");
        this->Verify(reader->GetElement(2, "T0C1") == "T0C1R2");
        this->Verify(reader->GetElement(1, 0, 1) == "T1C0R1");
        this->Verify(reader->GetElement(4, 2, 1) == "T1C2R4");
        reader->Close();
    }
}

} // namespace Test
