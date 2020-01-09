//------------------------------------------------------------------------------
//  excelxmlreader.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/excelxmlreader.h"

namespace IO
{
__ImplementClass(IO::ExcelXmlReader, 'EXRD', IO::StreamReader);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ExcelXmlReader::ExcelXmlReader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ExcelXmlReader::~ExcelXmlReader()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
    Open the Excel-XML-stream and completely parse its content.
*/
bool
ExcelXmlReader::Open()
{
    n_assert(this->tableNames.Size() == 0);
    n_assert(this->tables.Size() == 0);

    if (StreamReader::Open())
    {
        // parse the Excel XML stream
        if (this->ParseExcelXmlStream())
        {
            // since the Excel file is now loaded, we can close the original stream
            if (!this->streamWasOpen)
            {
                this->stream->Close();
            }

            // all ok
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ExcelXmlReader::Close()
{
    this->tableNames.Clear();
    this->tables.Clear();
    StreamReader::Close();
}

//------------------------------------------------------------------------------
/**
    This loads the Excel-XML file into a TinyXML document and reads the
    content into 2D string tables.
*/
bool
ExcelXmlReader::ParseExcelXmlStream()
{
    bool success = false;

    // setup an XML stream reader
    Ptr<XmlReader> xmlReader = XmlReader::Create();
    xmlReader->SetStream(this->stream);
    if (xmlReader->Open())
    {
        if (this->ParseTables(xmlReader))
        {
            success = true;
        }
        xmlReader->Close();
    }
    else
    {
        n_error("ExcelXmlReader::ParseExcelXmlStream(): failed to load '%s'!", this->stream->GetURI().AsString().AsCharPtr());
        success = false;
    }
    return success;
}

//------------------------------------------------------------------------------
/**
    Setup the table data from a loaded Excel XML file.
*/
bool
ExcelXmlReader::ParseTables(const Ptr<XmlReader>& xmlReader)
{
    n_assert(this->tables.Size() == 0);
    n_assert(this->tableNames.Size() == 0);

    // make sure it's actually an Excel XML file
    if (!xmlReader->HasNode("/Workbook"))
    {
        n_error("ExcelXmlReader: '%s' is not an Excel XML file!", this->stream->GetURI().AsString().AsCharPtr());
        return false;
    }
    xmlReader->SetToNode("/Workbook");

    // pre-define a few string objects for the following loop
    String worksheetString("Worksheet");
    String nameAttrString("ss:Name");
    String tableString("Table");
    String rowString("Row");
    String cellString("Cell");
    String dataString("Data");
    String cellDataString("Cell/Data");
    String contentString;

    // count number of tables in the file
    SizeT numTables = 0;
    if (xmlReader->SetToFirstChild(worksheetString)) do
    {
        numTables++;
    }
    while (xmlReader->SetToNextChild(worksheetString));
    n_assert(numTables > 0);
    this->tableNames.Reserve(numTables);
    this->tables.Reserve(numTables);

    // for each table...
    IndexT curTableIndex = 0;

    if (xmlReader->SetToFirstChild(worksheetString)) do
    {
        // get name of current work sheet
        String tableName = xmlReader->GetString(nameAttrString.AsCharPtr());
        this->tableNames.Append(tableName);
        n_assert((this->tableNames.Size() - 1) == curTableIndex);
        xmlReader->SetToFirstChild(tableString);

        // count the number of valid rows and columns and valid columns, we cannot
        // simply use the expendedRowCount and expandedColumnCount
        // attributes because they include table cells without any data
        IndexT numColumns = 0;
        IndexT numRows = 0;
        if (xmlReader->SetToFirstChild(rowString))do
        {
            // first row? if yes detect column count,
            // a column is only valid if it has data in the first row
            if (0 == numRows)
            {
                n_assert(0 == numColumns);
                if (xmlReader->SetToFirstChild(cellString)) do
                {
                    if (xmlReader->HasNode(dataString))
                    {
                        numColumns++;
                    }
                    else
                    {
                        // stop at the first column which doesn't have data in the first row
                        xmlReader->SetToParent();
                        break;
                    }
                }
                while (xmlReader->SetToNextChild(cellString));
            }

            // increment row count, a row is only valid if it has
            // valid data in the first column
            if (xmlReader->HasNode(cellDataString))
            {
                numRows++;
            }
            else
            {
                // stop at the first row which doesn't have data in the first column
                xmlReader->SetToParent();
                break;
            }
        }
        while (xmlReader->SetToNextChild(rowString));

        // setup 2D string table with row and column count and read actual data
        if ((numRows > 0) && (numColumns > 0))
        {
            FixedTable<String> newTable;
            this->tables.Append(newTable);
            n_assert((this->tables.Size() - 1) == curTableIndex);
            this->tables[curTableIndex].SetSize(numColumns, numRows);

            // read the actual table data
            IndexT curRowIndex = 0;
            if (xmlReader->SetToFirstChild(rowString))do
            {
                IndexT curColumnIndex = 0;
                if (xmlReader->SetToFirstChild(cellString)) do
                {
                    // check if cells are skipped
                    if (xmlReader->HasAttr("ss:Index"))
                    {
                        // jump to column
                        curColumnIndex = xmlReader->GetInt("ss:Index")-1;
                    }

                    // leave early if we are past the column count
                    if (curColumnIndex >= numColumns)
                    {
                        xmlReader->SetToParent();
                        break;
                    }

                    // check if data node exists
					if (xmlReader->HasNode(dataString))
					{
						xmlReader->SetToNode(dataString);
						if (xmlReader->HasContent())
						{
							this->tables[curTableIndex].Set(curColumnIndex, curRowIndex, xmlReader->GetContent());
						}
	                    xmlReader->SetToParent();
					}
					// if no data note exists, fill with empty string
					else
					{
						this->tables[curTableIndex].Set(curColumnIndex, curRowIndex, "");
					}
                    curColumnIndex++;
                }
                while (xmlReader->SetToNextChild(cellString));
                curRowIndex++;
                if (curRowIndex == numRows)
                {
                    xmlReader->SetToParent();
                    break;
                }
            }
            while (xmlReader->SetToNextChild(rowString));
        }
        xmlReader->SetToParent(); // of "Table"
        curTableIndex++;
    }
    while (xmlReader->SetToNextChild(worksheetString));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ExcelXmlReader::GetNumTables() const
{
    n_assert(this->IsOpen());
    return this->tables.Size();
}

//------------------------------------------------------------------------------
/**
*/
const String&
ExcelXmlReader::GetTableName(IndexT tableIndex) const
{
    n_assert(this->IsOpen());
    return this->tableNames[tableIndex];
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ExcelXmlReader::GetNumRows(IndexT tableIndex) const
{
    n_assert(this->IsOpen());
    return this->tables[tableIndex].Height();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ExcelXmlReader::GetNumColumns(IndexT tableIndex) const
{
    n_assert(this->IsOpen());
    return this->tables[tableIndex].Width();
}

//------------------------------------------------------------------------------
/**
    NOTE: this method is slow because it does a linear search over the column
    names.
*/
bool
ExcelXmlReader::HasColumn(const String& columnName, IndexT tableIndex) const
{
    n_assert(this->IsOpen());
    return InvalidIndex != this->FindColumnIndex(columnName, tableIndex);
}

//------------------------------------------------------------------------------
/**
    NOTE: this method is slow because it does a linear search over the column
    names.
*/
IndexT
ExcelXmlReader::FindColumnIndex(const String& columnName, IndexT tableIndex) const
{
    n_assert(columnName.IsValid());
    n_assert(this->IsOpen());
    const FixedTable<String>& table = this->tables[tableIndex];
    IndexT colIndex;
    for (colIndex = 0; colIndex < table.Width(); colIndex++)
    {
        if (table.At(colIndex, 0) == columnName)
        {
            return colIndex;
        }
    }
    // fallthrough: not found
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
const String&
ExcelXmlReader::GetElement(IndexT rowIndex, IndexT columnIndex, IndexT tableIndex) const
{
    n_assert(this->IsOpen());
    return this->tables[tableIndex].At(columnIndex, rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
const String&
ExcelXmlReader::GetElement(IndexT rowIndex, const String& columnName, IndexT tableIndex) const
{
    n_assert(this->IsOpen());
    IndexT columnIndex = this->FindColumnIndex(columnName, tableIndex);
    return this->tables[tableIndex].At(columnIndex, rowIndex);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
ExcelXmlReader::GetTableIndex(const Util::String& tableName)
{
    return this->tableNames.FindIndex(tableName);
}

} // namespace IO
