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
void
ResourceTableSet::Create(const ResourceTableCreateInfo& createInfo)
{
    const SizeT numTables = CoreGraphics::GetNumBufferedFrames();
    this->tables.Resize(numTables);
    const char* name = createInfo.name;
    for (IndexT i = 0; i < numTables; i++)
    {
        this->tables[i] = CreateResourceTable(createInfo);
        if (createInfo.name != nullptr)
            CoreGraphics::ObjectSetName(this->tables[i], Util::String::Sprintf("%s %d", createInfo.name, i).AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSet::Destroy()
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
const CoreGraphics::ResourceTableId
ResourceTableSet::Get()
{
    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();
    return this->tables[bufferIndex];
}

} // namespace CoreGraphics
