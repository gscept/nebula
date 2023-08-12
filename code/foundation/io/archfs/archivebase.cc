//------------------------------------------------------------------------------
//  archivebase.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/archfs/archivebase.h"

namespace IO
{
__ImplementClass(IO::ArchiveBase, 'ARCB', Core::RefCounted);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ArchiveBase::ArchiveBase() :
    isValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ArchiveBase::~ArchiveBase()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
    Setup the archive object from an URI pointing to an archive file. This
    method may return false if something went wrong (archive file not
    found, or wrong format).
*/
bool
ArchiveBase::Setup(const URI& archiveFileURI, const Util::String& rootPath)
{
    n_assert(!this->IsValid());
    this->uri = archiveFileURI;
    this->isValid = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ArchiveBase::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
    List all files in an archive directory. Override this method in a subclass!
*/
Array<String>
ArchiveBase::ListFiles(const String& dirPathInArchive, const String& pattern) const
{
    Array<String> emptyArray;
    return emptyArray;
}

//------------------------------------------------------------------------------
/**
    List all directories in an archive directory. Override this method in a 
    subclass!
*/
Array<String>
ArchiveBase::ListDirectories(const String& dirPathInArchive, const String& pattern) const
{
    Array<String> emptyArray;
    return emptyArray;
}

//------------------------------------------------------------------------------
/**
    This method should convert a "file:" URI into an URI suitable for
    an archive specific stream class.

    Override this method in a subclass!
*/
URI
ArchiveBase::ConvertToArchiveURI(const URI& fileURI) const
{
    return fileURI;
}

//------------------------------------------------------------------------------
/**
    This method should convert an absolute file system path into a
    local path in the archive suitable for ListFiles() and ListDirectories().

    Override this method in a subclass!
*/
String
ArchiveBase::ConvertToPathInArchive(const String& absPath) const
{
    return absPath;
}

} // namespace IO
