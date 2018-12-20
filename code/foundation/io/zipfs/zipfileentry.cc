//------------------------------------------------------------------------------
//  zipfileentry.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/zipfs/zipfileentry.h"

namespace IO
{
using namespace Util;
using namespace Threading;

//------------------------------------------------------------------------------
/**
*/
ZipFileEntry::ZipFileEntry() :
    archiveCritSect(0),
    zipFileHandle(0),
    uncompressedSize(0)
{
    Memory::Clear(&this->filePosInfo, sizeof(this->filePosInfo));
}

//------------------------------------------------------------------------------
/**
*/
ZipFileEntry::~ZipFileEntry()
{
    this->zipFileHandle = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
ZipFileEntry::Setup(const StringAtom& n, unzFile h, CriticalSection* critSect)
{
    n_assert(0 != h);
    n_assert(0 == this->zipFileHandle);
    n_assert(0 != critSect);

    this->name = n;
    this->zipFileHandle = h;

    // store pointer to archive's critical section
    this->archiveCritSect = critSect;

    // store file position
    int res = unzGetFilePos(this->zipFileHandle, &this->filePosInfo);
    n_assert(UNZ_OK == res);

    // get other data about the file
    unz_file_info fileInfo;
    res = unzGetCurrentFileInfo(this->zipFileHandle, &fileInfo, 0, 0, 0, 0, 0, 0);
    n_assert(UNZ_OK == res);
    this->uncompressedSize = fileInfo.uncompressed_size;
}

//------------------------------------------------------------------------------
/**
*/
bool
ZipFileEntry::Open(const String& password)
{
    // critical section active until close is called or this function fails
    this->archiveCritSect->Enter();

    // set current file to this file
    int res = unzGoToFilePos(this->zipFileHandle, const_cast<unz_file_pos*>(&this->filePosInfo));
    if (UNZ_OK != res) 
    {
        this->archiveCritSect->Leave();
        return false;
    }

    // open the current file with optional password
    if (password.IsValid())
    {
        res = unzOpenCurrentFilePassword(this->zipFileHandle, password.AsCharPtr());
    }
    else
    {
        res = unzOpenCurrentFile(this->zipFileHandle);
    }

    if (UNZ_OK != res) 
    {
        this->archiveCritSect->Leave();
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ZipFileEntry::Close()
{
    // close the file
    int res = unzCloseCurrentFile(this->zipFileHandle);
    n_assert(UNZ_OK == res);

    // release the critical section
    this->archiveCritSect->Leave();
}

//------------------------------------------------------------------------------
/**
*/
bool
ZipFileEntry::Read(void* buf, Stream::Size numBytes) const
{
    n_assert(0 != this->zipFileHandle);
    n_assert(0 != buf);
    n_assert(0 != this->archiveCritSect);

    // read uncompressed data 
    int readResult = unzReadCurrentFile(this->zipFileHandle, buf, numBytes);    
    if (numBytes != readResult) return false;

    return true;
}

} // namespace ZipFileEntry
