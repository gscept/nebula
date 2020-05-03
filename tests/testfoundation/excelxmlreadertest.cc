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

    n_assert(false);
    // load a prepared Excel file
    reader->SetStream(stream);
    bool success = reader->Open();
    VERIFY(success);
    if (success)
    {
        VERIFY(reader->GetNumTables() == 2);
        VERIFY(reader->GetTableName() == "table0");
        VERIFY(reader->GetTableName(1) == "table1");
        VERIFY(reader->GetNumRows() == 3);
        VERIFY(reader->GetNumRows(1) == 5);
        VERIFY(reader->GetNumColumns() == 2);
        VERIFY(reader->GetNumColumns(1) == 3);
        VERIFY(reader->HasColumn("T0C0"));
        VERIFY(reader->HasColumn("T0C1"));
        VERIFY(reader->HasColumn("T1C0", 1));
        VERIFY(reader->HasColumn("T1C1", 1));
        VERIFY(reader->HasColumn("T1C2", 1));
        VERIFY(InvalidIndex == reader->FindColumnIndex("T0C2"));
        VERIFY(1 == reader->FindColumnIndex("T0C1"));
        VERIFY(2 == reader->FindColumnIndex("T1C2", 1));
        VERIFY(reader->GetElement(1, 0) == "T0C0R1");
        VERIFY(reader->GetElement(2, 0) == "T0C0R2");
        VERIFY(reader->GetElement(1, 1) == "T0C1R1");
        VERIFY(reader->GetElement(2, 1) == "T0C1R2");
        VERIFY(reader->GetElement(2, "T0C1") == "T0C1R2");
        VERIFY(reader->GetElement(1, 0, 1) == "T1C0R1");
        VERIFY(reader->GetElement(4, 2, 1) == "T1C2R4");
        reader->Close();
    }
}

} // namespace Test
