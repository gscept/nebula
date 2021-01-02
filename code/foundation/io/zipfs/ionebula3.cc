/* ionebula3.c -- IO base function header for compress/uncompress .zip
   files using zlib + zip or unzip API

    ZLib File IO functions for Nebula3

   (C) 2007 Radon Labs GmbH   
   (C) 2020 Individual contributors, see AUTHORS file
*/
#include "foundation/stdneb.h"
#include "io/filestream.h"
#include "io/ioserver.h"
#include "io/archfs/archivefilesystem.h"
#include "minizip/ioapi.h"
#include "ionebula3.h"

//------------------------------------------------------------------------------
/**
*/
voidpf ZCALLBACK nebula3_open_file_func (voidpf opaque, const void* filename64, int mode)
{
    const char * filename = (const char*)filename64;
    n_assert(mode == (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_EXISTING));
    Ptr<IO::Stream> fileStream = IO::IoServer::Instance()->CreateStream(filename);
    fileStream->SetAccessMode(IO::Stream::ReadAccess);
    fileStream->SetURI(filename);
    if (fileStream->Open())
    {
        fileStream->AddRef();
        return fileStream.get();
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
uint32_t ZCALLBACK nebula3_read_file_func (voidpf opaque, voidpf stream, void* buf, uint32_t size)
{
    uint32_t ret = 0;
    if (NULL != stream)
    {
        IO::Stream* fileStream = (IO::Stream*) stream;
        ret = fileStream->Read(buf, size);
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t ZCALLBACK nebula3_write_file_func (voidpf opaque, voidpf stream, const void* buf, uint32_t size)
{
    n_error("nebula3_write_file_func(): Writing to ZIP archives not supported!");
    return -1;
}

//------------------------------------------------------------------------------
/**
*/
uint64_t ZCALLBACK nebula3_tell_file_func (voidpf opaque, voidpf stream)
{
    uint64_t ret = -1;
    if (NULL != stream)
    {
        IO::Stream* fileStream = (IO::Stream*) stream;
        ret = fileStream->GetPosition();
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
long ZCALLBACK nebula3_seek_file_func (voidpf opaque, voidpf stream, uint64_t offset, int origin)
{
    long ret = -1;
    if (NULL != stream)
    {
        IO::Stream* fileStream = (IO::Stream*) stream;
        IO::Stream::SeekOrigin neb3Origin;
        switch (origin)
        {
            case ZLIB_FILEFUNC_SEEK_CUR:
                neb3Origin = IO::Stream::Current;
                break;
            case ZLIB_FILEFUNC_SEEK_END:
                neb3Origin = IO::Stream::End;
                break;
            case ZLIB_FILEFUNC_SEEK_SET:
                neb3Origin = IO::Stream::Begin;
                break;
            default:
                return -1;
        }
        // FIXME: hmm... should we return -1 if going past the valid file area??
        fileStream->Seek(offset, neb3Origin);
        ret = 0;
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
int ZCALLBACK nebula3_close_file_func (voidpf opaque, voidpf stream)
{
    int ret = -1;
    if (NULL != stream)
    {
        IO::Stream* fileStream = (IO::Stream*) stream;
        fileStream->Close();
        fileStream->Release();
        fileStream = 0;
        ret = 0;
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
int ZCALLBACK nebula3_error_file_func (voidpf opaque, voidpf stream)
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void fill_nebula3_filefunc (zlib_filefunc64_def* pzlib_filefunc_def)
{
    pzlib_filefunc_def->zopen64_file = nebula3_open_file_func;
    pzlib_filefunc_def->zread_file = nebula3_read_file_func;
    pzlib_filefunc_def->zwrite_file = nebula3_write_file_func;
    pzlib_filefunc_def->ztell64_file = nebula3_tell_file_func;
    pzlib_filefunc_def->zseek64_file = nebula3_seek_file_func;
    pzlib_filefunc_def->zclose_file = nebula3_close_file_func;
    pzlib_filefunc_def->zerror_file = nebula3_error_file_func;
    pzlib_filefunc_def->opaque=NULL;
}
