#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ZipDirEntry

    A directory entry in a zip arcive. The ZipDirEntry class is thread-safe,
    all public methods can be invoked from on the same object from different
    threads.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "io/zipfs/zipfileentry.h"

//------------------------------------------------------------------------------
namespace IO    
{
class ZipDirEntry
{
public:
    /// constructor
    ZipDirEntry();
    
    /// get the name of the dir entry
    const Util::StringAtom& GetName() const;
    /// find a direct child file entry, return 0 if not exists
    ZipFileEntry* FindFileEntry(const Util::StringAtom& name) const;
    /// find a direct child directory entry, return 0 if not exists
    ZipDirEntry* FindDirEntry(const Util::StringAtom& name) const;
    /// get directory entries
    const Util::Array<ZipDirEntry>& GetDirEntries() const;
    /// get file entries
    const Util::Array<ZipFileEntry>& GetFileEntries() const;
    
private:
    friend class ZipArchive;

    /// set the name of the dir entry
    void SetName(const Util::StringAtom& n);
    /// add a file child entry
    ZipFileEntry* AddFileEntry(const Util::StringAtom& name);
    /// add a directory child entry
    ZipDirEntry* AddDirEntry(const Util::StringAtom& name);

    Util::StringAtom name;
    Util::Array<ZipFileEntry> fileEntries;
    Util::Array<ZipDirEntry> dirEntries;
    Util::Dictionary<Util::StringAtom, IndexT> fileIndexMap;
    Util::Dictionary<Util::StringAtom, IndexT> dirIndexMap;
};

//------------------------------------------------------------------------------
/**
*/
inline void
ZipDirEntry::SetName(const Util::StringAtom& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
ZipDirEntry::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<ZipDirEntry>&
ZipDirEntry::GetDirEntries() const
{
    return this->dirEntries;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<ZipFileEntry>&
ZipDirEntry::GetFileEntries() const
{
    return this->fileEntries;
}


} // namespace IO
//------------------------------------------------------------------------------

    