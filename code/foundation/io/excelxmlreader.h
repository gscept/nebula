#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ExcelXmlReader
    
    A stream reader class which reads Excel XML files.
    NOTE: the strings returned by this class will be in UTF-8 format!
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"
#include "io/xmlreader.h"
#include "util/string.h"
#include "util/array.h"
#include "util/fixedtable.h"

//------------------------------------------------------------------------------
namespace IO
{
class ExcelXmlReader : public StreamReader
{
    __DeclareClass(ExcelXmlReader);
public:
    /// constructor
    ExcelXmlReader();
    /// destructor
    virtual ~ExcelXmlReader();

    /// begin reading from the stream
    virtual bool Open();
    /// end reading from the stream
    virtual void Close();

    /// get the number of tables in the file
    SizeT GetNumTables() const;
    /// get the name of the table at given table index
    const Util::String& GetTableName(IndexT tableIndex = 0) const;
    /// get index of table by name
    IndexT GetTableIndex(const Util::String& tableName);
    /// get number of rows in table
    SizeT GetNumRows(IndexT tableIndex = 0) const;
    /// get number of columns in table
    SizeT GetNumColumns(IndexT tableIndex = 0) const;
    /// return true if table has named column
    bool HasColumn(const Util::String& columnName, IndexT tableIndex = 0) const;
    /// get column index by name, returns InvalidIndex if column doesn't exist
    IndexT FindColumnIndex(const Util::String& columnName, IndexT tableIndex = 0) const;
    /// get cell content by row index and column index
    const Util::String& GetElement(IndexT rowIndex, IndexT columnIndex, IndexT tableIndex = 0) const;
    /// get cell content by row index and column name (SLOW!)
    const Util::String& GetElement(IndexT rowIndex, const Util::String& columnName, IndexT tableIndex = 0) const;

private:
    /// parse the Excel-XML-stream
    bool ParseExcelXmlStream();
    /// parse table contents
    bool ParseTables(const Ptr<IO::XmlReader>& xmlReader);

    Util::Array<Util::String> tableNames;
    Util::Array<Util::FixedTable<Util::String> > tables;
};

} // namespace IO
//------------------------------------------------------------------------------
