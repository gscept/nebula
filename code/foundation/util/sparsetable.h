#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::SparseTable
  
    A 2D sparse table where many entries may be redundant and support
    for multiple entries per cell.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "util/array.h"
#include "util/fixedtable.h"
#include "util/dictionary.h"
#include "util/stringatom.h"

//------------------------------------------------------------------------------
namespace Util
{
template<class TYPE> class SparseTable
{
public:
    /// constructor
    SparseTable();

    /// setup the sparse table
    void BeginSetup(const Array<StringAtom>& columnNames, const Array<StringAtom>& rowNames, SizeT numUnique=0);
    /// add a single new unique entry
    void AddSingle(IndexT colIndex, IndexT rowIndex, const TYPE& elm);
    /// add a new multiple entry
    void AddMultiple(IndexT colIndex, IndexT rowIndex, const TYPE* firstElm, SizeT numElements);
    /// add a reference to another column/index
    void AddReference(IndexT colIndex, IndexT rowIndex, IndexT refColIndex, IndexT refRowIndex);
    /// add a direct reference using an index into the unique element array
    void SetEntryDirect(IndexT colIndex, IndexT rowIndex, ushort startIndex, ushort numElements);
    /// finish setting up the sparse table
    void EndSetup();
    /// clear object
    void Clear();

    /// get number of columns in the sparse table
    SizeT GetNumColumns() const;
    /// get number of rows in the sparse table
    SizeT GetNumRows() const;
    /// return true if column exists
    bool HasColumn(const StringAtom& colName) const;
    /// return true if row exists
    bool HasRow(const StringAtom& rowName) const;
    /// return column index by name
    IndexT GetColumnIndexByName(const StringAtom& colName) const;
    /// return row index by name
    IndexT GetRowIndexByName(const StringAtom& rowName) const;
    /// get current number of unique elements
    SizeT GetNumUniqueElements() const;

    /// get entry at given index
    const TYPE* GetElements(IndexT colIndex, IndexT rowIndex, SizeT& outNumElements) const;
    /// lookup entry by row/column names
    const TYPE* LookupElements(const StringAtom& colName, const StringAtom& rowName, SizeT& outNumElements) const;

private:
    /// a table entry in the sparse table
    struct TableEntry
    {
        TableEntry() : startIndex(0xffff), numElements(0) {};

        ushort startIndex;      // index into uniqueElements array
        ushort numElements;     // number of elements in uniqueElements array
    };

    Array<TYPE> uniqueElements;
    FixedTable<TableEntry> tableEntries;
    Dictionary<StringAtom, IndexT> colIndexMap;
    Dictionary<StringAtom, IndexT> rowIndexMap;
    bool inSetup;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
SparseTable<TYPE>::SparseTable() :
    inSetup(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SparseTable<TYPE>::Clear()
{
    n_assert(!this->inSetup);
    this->uniqueElements.Clear();
    this->tableEntries.SetSize(0, 0);
    this->colIndexMap.Clear();
    this->rowIndexMap.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SparseTable<TYPE>::BeginSetup(const Array<StringAtom>& columnNames, const Array<StringAtom>& rowNames, SizeT numUnique)
{
    n_assert(!this->inSetup);
    this->inSetup = true;
    if (numUnique > 0)
    {
        this->uniqueElements.Reserve(numUnique);
    }
    this->tableEntries.SetSize(columnNames.Size(), rowNames.Size());
    this->colIndexMap.Reserve(columnNames.Size());
    this->rowIndexMap.Reserve(rowNames.Size());

    this->colIndexMap.BeginBulkAdd();
    IndexT i;
    for (i = 0; i < columnNames.Size(); i++)
    {
        this->colIndexMap.Add(columnNames[i], i);
    }
    this->colIndexMap.EndBulkAdd();

    this->rowIndexMap.BeginBulkAdd();
    for (i = 0; i < rowNames.Size(); i++)
    {
        this->rowIndexMap.Add(rowNames[i], i);
    }
    this->rowIndexMap.EndBulkAdd();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SparseTable<TYPE>::AddSingle(IndexT colIndex, IndexT rowIndex, const TYPE& elm)
{
    n_assert(this->inSetup);
    TableEntry& entry = this->tableEntries.At(colIndex, rowIndex);
    entry.startIndex = this->uniqueElements.Size();
    entry.numElements = 1;
    this->uniqueElements.Append(elm);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SparseTable<TYPE>::AddMultiple(IndexT colIndex, IndexT rowIndex, const TYPE* firstElm, SizeT numElms)
{
    n_assert(this->inSetup);
    TableEntry& entry = this->tableEntires.At(colIndex, rowIndex);
    entry.startIndex = this->uniqueElements.Size();
    entry.numElements = elms.Size();
    IndexT i;
    for (i = 0; i < numElms; i++)
    {
        this->uniqueElements.Append(firstElm[i]);
    }
}

//------------------------------------------------------------------------------
/**
    NOTE: forward references are not allowed!
*/
template<class TYPE> void
SparseTable<TYPE>::AddReference(IndexT colIndex, IndexT rowIndex, IndexT refColIndex, IndexT refRowIndex)
{
    n_assert(this->inSetup);
    n_assert((refColIndex <= colIndex) && (refRowIndex <= rowIndex));
    TableEntry& entry = this->tableEntries.At(refColIndex, refRowIndex);
    this->tableEntries.Set(colIndex, rowIndex, entry);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SparseTable<TYPE>::SetEntryDirect(IndexT colIndex, IndexT rowIndex, ushort startIndex, ushort numElements)
{
    n_assert(this->inSetup);
    TableEntry& entry = this->tableEntries.At(refColIndex, refRowIndex);
    entry.startIndex = startIndex;
    entry.numElements = numElements;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
SparseTable<TYPE>::EndSetup()
{
    n_assert(this->inSetup);
    this->inSetup = false;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> SizeT
SparseTable<TYPE>::GetNumColumns() const
{
    return this->colIndexMap.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> SizeT
SparseTable<TYPE>::GetNumRows() const
{
    return this->rowIndexMap.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool
SparseTable<TYPE>::HasColumn(const StringAtom& colName) const
{
    return this->colIndexMap.Contains(colName);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool
SparseTable<TYPE>::HasRow(const StringAtom& rowName) const
{
    return this->rowIndexMap.Contains(rowName);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> IndexT
SparseTable<TYPE>::GetColumnIndexByName(const StringAtom& colName) const
{
    return this->colIndexMap[colName];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> IndexT
SparseTable<TYPE>::GetRowIndexByName(const StringAtom& rowName) const
{
    return this->rowIndexMap[rowName];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> const TYPE*
SparseTable<TYPE>::GetElements(IndexT colIndex, IndexT rowIndex, SizeT& outNumElements) const
{
    const TableEntry& tableEntry = this->tableEntries.At(colIndex, rowIndex);
    if (tableEntry.numElements > 0)
    {
        const TYPE* elm = &(this->uniqueElements[tableEntry.startIndex]);
        outNumElements = tableEntry.numElements;
        return elm;
    }
    else
    {
        outNumElements = 0;
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> const TYPE*
SparseTable<TYPE>::LookupElements(const StringAtom& colName, const StringAtom& rowName, SizeT& outNumElements) const
{
    IndexT colIndex = this->colIndexMap[colName];
    IndexT rowIndex = this->rowIndexMap[rowName];
    return this->GetElements(colIndex, rowIndex, outNumElements);
}

} // namespace Util 
//------------------------------------------------------------------------------
