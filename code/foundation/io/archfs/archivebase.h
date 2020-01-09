#pragma once
//------------------------------------------------------------------------------
/** 
    @class IO::ArchiveBase
    
    Base class of file archives. Subclasses of this class implemented support
    for specific archive formats, like zip.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace IO
{
class ArchiveBase : public Core::RefCounted
{
    __DeclareClass(ArchiveBase);
public:
    /// constructor
    ArchiveBase();
    /// destructor
    virtual ~ArchiveBase();

    /// setup the archive from an URI (without file extension)
    bool Setup(const URI& archiveURI);
    /// discard the archive
    void Discard();
    /// return true if archive is valid
    bool IsValid() const;
    /// get the URI of the archive
    const URI& GetURI() const;

    /// list all files in a directory in the archive
    Util::Array<Util::String> ListFiles(const Util::String& dirPathInArchive, const Util::String& pattern) const;
    /// list all subdirectories in a directory in the archive
    Util::Array<Util::String> ListDirectories(const Util::String& dirPathInArchive, const Util::String& pattern) const;
    /// convert a "file:" URI into a archive-specific URI pointing into this archive
    URI ConvertToArchiveURI(const URI& fileURI) const;
    /// convert an absolute path to local path inside archive, returns empty string if absPath doesn't point into this archive
    Util::String ConvertToPathInArchive(const Util::String& absPath) const;

protected:
    bool isValid;
    URI uri;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ArchiveBase::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline const URI&
ArchiveBase::GetURI() const
{
    return this->uri;
}

} // namespace IO
//------------------------------------------------------------------------------


    