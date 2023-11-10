//------------------------------------------------------------------------------
//  @file resourcetable.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "resourcetable.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
ResourceTableSet::ResourceTableSet(const ResourceTableCreateInfo& createInfo)
{
    const SizeT numTables = CoreGraphics::GetNumBufferedFrames();
    this->tables.Resize(numTables);
    for (IndexT i = 0; i < numTables; i++)
    {
        this->tables[i] = CreateResourceTable(createInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
ResourceTableSet::ResourceTableSet(ResourceTableSet&& rhs)
{
    this->tables = rhs.tables;
    rhs.tables.Clear();
}

//------------------------------------------------------------------------------
/**
*/
ResourceTableSet::~ResourceTableSet()
{
    for (IndexT i = 0; i < this->tables.Size(); i++)
    {
        DestroyResourceTable(this->tables[i]);
    }
    this->tables.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSet::ForEach(std::function<void(const ResourceTableId, const IndexT)> func)
{
    for (IndexT i = 0; i < this->tables.Size(); i++)
    {
        func(this->tables[i], i);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSet::operator=(ResourceTableSet&& rhs)
{
    this->tables = rhs.tables;
    rhs.tables.Clear();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId
ResourceTableSet::Get()
{
    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();
    return this->tables[bufferIndex];
}

} // namespace CoreGraphics
