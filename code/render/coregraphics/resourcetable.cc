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
    this->outdated.Resize(numTables);
    this->outdated.Fill(false);
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
    this->outdated.Clear();
    this->update = nullptr;
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
    return this->Get(bufferIndex);
}

//------------------------------------------------------------------------------
/**
    By the time a frame is recorded, NewFrame has waited for the GPU to finish the frame
    that used this buffer index before, so nothing is still reading the table
*/
const CoreGraphics::ResourceTableId
ResourceTableSet::Get(const IndexT bufferIndex)
{
    const CoreGraphics::ResourceTableId table = this->tables[bufferIndex];
    if (this->outdated[bufferIndex])
    {
        this->update(table);
        this->outdated[bufferIndex] = false;
    }
    return table;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSet::SetUpdate(std::function<void(const ResourceTableId)> func)
{
    this->update = std::move(func);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSet::Invalidate()
{
    n_assert(this->update != nullptr);
    this->outdated.Fill(true);
}

} // namespace CoreGraphics
