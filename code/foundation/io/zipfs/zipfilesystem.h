#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ZipFileSystem
    
    An archive filesystem wrapper for ZIP files. 

    Uses the zlib and the minizip package under the hood.

    Limitations:
    * No write access.
    * No seek on compressed data, the ZipFileSystem will generally decompress 
      an entire file into memory at once, so that the ZipStreamClass can
      provide random access on the decompressed data. Thus the typical 
      "audio streaming scenario" is not possible from zip files (that's
      what XACT's sound banks is there for anyway ;)

    How to fix the no-seek problem:
    * zlib processes datas in chunks, and cannot seek randomly within 
      a chunk, and the chunk size is dependent on the compress application
      being used to create the zip file(?), if those internals are known,
      it would be possible to write a chunked filesystem which keeps
      buffered chunks around for each client, probably not worth the effort.
    * Another appoach would be to split stream-files into "chunk-files"
      before compressing, and to read the next complete chunk files
      when new data is needed. This approach doesn't require changes to 
      the strip filesystem.
      
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/archfs/archivefilesystembase.h"

//------------------------------------------------------------------------------
namespace IO
{
class ZipArchive;

class ZipFileSystem : public ArchiveFileSystemBase
{
    __DeclareClass(ZipFileSystem);
    __DeclareInterfaceSingleton(ZipFileSystem);
public:
    /// constructor
    ZipFileSystem();
    /// destructor
    virtual ~ZipFileSystem();

    /// setup the archive file system
    void Setup();
    /// discard the archive file system
    void Discard();

    /// find first archive which contains the file path
    Ptr<Archive> FindArchiveWithFile(const URI& fileUri) const;
    /// find first archive which contains the directory path
    Ptr<Archive> FindArchiveWithDir(const URI& dirUri) const;
};

} // namespace IO
//------------------------------------------------------------------------------
    
    
    