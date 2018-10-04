//------------------------------------------------------------------------------
//  zipdirentry.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/zipfs/zipdirentry.h"

namespace IO
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ZipDirEntry::ZipDirEntry()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Adds a new file entry object to the internal dictionary. NOTE:
    this method will not check whether the entry already exists for 
    performance reasons (doing this would force the dictionary to be
    sorted after every insert).

    The method returns a reference of the actually added file entry.
*/
ZipFileEntry*
ZipDirEntry::AddFileEntry(const StringAtom& name)
{
    ZipFileEntry dummy;
    this->fileEntries.Append(dummy);
    this->fileIndexMap.Add(name, this->fileEntries.Size() - 1);
    return &(this->fileEntries.Back());
}

//------------------------------------------------------------------------------
/**
    Adds a new directory entry object to the internal dictionary. NOTE:
    this method will not check whether the entry already exists for 
    performance reasons (doing this would force the dictionary to be
    sorted after every insert).

    The method returns a reference to the actually added DirEntry. 
*/
ZipDirEntry*
ZipDirEntry::AddDirEntry(const StringAtom& name)
{
    ZipDirEntry dummy;
    this->dirEntries.Append(dummy);
    this->dirEntries.Back().SetName(name);
    this->dirIndexMap.Add(name, this->dirEntries.Size() - 1);
    return &(this->dirEntries.Back());
}

//------------------------------------------------------------------------------
/**
*/
ZipFileEntry*
ZipDirEntry::FindFileEntry(const StringAtom& name) const
{
    n_assert(name.IsValid());
    IndexT index = this->fileIndexMap.FindIndex(name);
    if (InvalidIndex == index)
    {
        return 0;
    }
    else
    {
        return &(this->fileEntries[this->fileIndexMap.ValueAtIndex(index)]);
    }
}

//------------------------------------------------------------------------------
/**
*/
ZipDirEntry*
ZipDirEntry::FindDirEntry(const StringAtom& name) const
{
    n_assert(name.IsValid());
    IndexT index = this->dirIndexMap.FindIndex(name);
    if (InvalidIndex == index)
    {
        return 0;
    }
    else
    {
        return &(this->dirEntries[this->dirIndexMap.ValueAtIndex(index)]);
    }
}

} // namespace IO
