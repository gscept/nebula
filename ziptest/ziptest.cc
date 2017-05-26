//------------------------------------------------------------------------------
//  ziptest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ziptestapplication.h"

#include "zlib/unzip.h"
#include "io/stream.h"

static const char *ZIP_ARCHIVE = "c:/nebula3/export_win32.zip";
static const char *FILE_IN_ARCHIVE = "export_win32/shaders/shaders.dic";
//static const char *FILE_IN_ARCHIVE = "export_win32/test";
//static const char *FILE_IN_ARCHIVE = "export_win32/textures/baked/skelett_schwer_baked.dds";
static const int CASESENSITIVITY = 0;

class ZipArchive
{
public:
    ZipArchive() : file(NULL) {}

    bool Open(const char *archive_name)
    {
        n_assert(!this->file);
        this->file = unzOpen(archive_name);
        return (this->file != NULL);
    }

    void Close()
    {
        n_assert(this->file);
        if(UNZ_OK != unzClose(this->file)) n_assert(false);
        this->file = NULL;
    }

    bool SetCurrentFile(const char *fileInArchive)
    {
        n_assert(this->file);
        return (UNZ_OK == unzLocateFile(this->file, fileInArchive, CASESENSITIVITY));
    }

    bool OpenCurrentFile()
    {
        n_assert(this->file);
        return (UNZ_OK == unzOpenCurrentFile(this->file));
    }

    void CloseCurrentFile()
    {
        n_assert(this->file);
        if(UNZ_OK != unzCloseCurrentFile(file)) n_assert(false);
    }

    int ReadCurrentFile(void *buf, int len)
    {
        n_assert(this->file);
        return unzReadCurrentFile(this->file, buf, len);
    }

    unz_file_info GetCurrentFileInfo()
    {
        n_assert(this->file);
        char filename_inzip[256];
        unz_file_info ret;
        if(UNZ_OK != unzGetCurrentFileInfo(this->file, &ret, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0)) n_assert(false);
        return ret;
    }
/*
    z_off_t TellCurrentFile()
    {
        n_assert(this->file);
        return unztell(this->file);
    }
*/
    void SeekCurrentFile(IO::Stream::Offset offset, IO::Stream::SeekOrigin origin)
    {
        n_assert(this->file);
        const z_off_t curPos = unztell(this->file);
        unz_file_info fileInfo = GetCurrentFileInfo();

        z_off_t absSeekPos;
        switch(origin)
        {
            case IO::Stream::Begin:
                absSeekPos = offset;
                break;
            case IO::Stream::Current:
                absSeekPos = curPos + offset;
                break;
            case IO::Stream::End:
                absSeekPos = (long)((long long)fileInfo.uncompressed_size - (long long)offset);
                break;
        }
        n_assert(absSeekPos >= curPos);
        if(absSeekPos == curPos) return;
        const long readBytes = absSeekPos - curPos;
        n_assert(readBytes > 0);
        char *dummyBuffer = (char *) malloc(readBytes);
        n_assert(dummyBuffer);
        ReadCurrentFile(dummyBuffer, readBytes);
        free(dummyBuffer);
        dummyBuffer = NULL;
    }

private:
    unzFile file;
};


int main()
{

    static const int BYTES_TO_WRITE = 16392 * 20;
    n_assert(!(BYTES_TO_WRITE % 4));
    FILE *fp = fopen("c:/temp/test", "wb");
    for(unsigned int i = 0; i < BYTES_TO_WRITE; i += 4)
    {
        fwrite(&i, 4, 1, fp);
    }
    fclose(fp);
    fp = NULL;


    ZipArchive archive;
    if(!archive.Open(ZIP_ARCHIVE)) n_assert(false);

    if(!archive.SetCurrentFile(FILE_IN_ARCHIVE)) n_assert(false);

    unz_file_info info = archive.GetCurrentFileInfo();
    printf("size   compressed: %lu\n", info.compressed_size);
    printf("size uncompressed: %lu\n", info.uncompressed_size);
    printf("\n");
    char *buffer = (char*)malloc(info.uncompressed_size);
    char *buffer_current = buffer;

    if(!archive.OpenCurrentFile()) n_assert(false);

    int bytesRequested, bytesRead;

    bytesRequested = 2;
    bytesRead = archive.ReadCurrentFile(buffer_current, bytesRequested);
    n_assert(bytesRead == bytesRequested);
    *(buffer_current + bytesRequested) = '\0';
    printf(buffer_current);
    buffer_current += bytesRead;

    archive.SeekCurrentFile(13, IO::Stream::Current);
    buffer_current += 13;

    bytesRequested = 12;
    bytesRead = archive.ReadCurrentFile(buffer_current, bytesRequested);
    n_assert(bytesRead == bytesRequested);
    *(buffer_current + bytesRequested) = '\0';
    printf(buffer_current);
    buffer_current += bytesRead;

/*        
    bytesRequested = 2;
    bytesRead = archive.ReadCurrentFile(buffer_current, bytesRequested);
    n_assert(bytesRead == bytesRequested);
    buffer_current += bytesRead;

    bytesRequested = 3;
    bytesRead = archive.ReadCurrentFile(buffer_current, bytesRequested);
    n_assert(bytesRead == bytesRequested);
    buffer_current += bytesRead;

    bytesRequested = 16392;
    bytesRead = archive.ReadCurrentFile(buffer_current, bytesRequested);
    n_assert(bytesRead == bytesRequested);
    buffer_current += bytesRead;

    bytesRequested = 16392;
    bytesRead = archive.ReadCurrentFile(buffer_current, bytesRequested);
    n_assert(bytesRead == bytesRequested);
    buffer_current += bytesRead;

    bytesRequested = 16392;
    bytesRead = archive.ReadCurrentFile(buffer_current, bytesRequested);
    n_assert(bytesRead == bytesRequested);
    buffer_current += bytesRead;

    bytesRequested = 16392;
    bytesRead = archive.ReadCurrentFile(buffer_current, bytesRequested);
    n_assert(bytesRead == bytesRequested);
    buffer_current += bytesRead;
*/

    archive.CloseCurrentFile();
    archive.Close();

    free(buffer);
    buffer = NULL;

/*
    unzFile file = unzOpen(ZIP_ARCHIVE);
    if(!file) n_assert(false);

    if(UNZ_OK != unzLocateFile(file, FILE_IN_ARCHIVE, CASESENSITIVITY)) n_assert(false);

    char filename_inzip[256];
    unz_file_info file_info;
    if(UNZ_OK != unzGetCurrentFileInfo(file, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0)) n_assert(false);

    printf("size: %lu\n", file_info.compressed_size);
    char *buffer = (char*)malloc(file_info.compressed_size);
    
    if(UNZ_OK != unzOpenCurrentFile(file)) n_assert(false);

    char *buffer_current = buffer;
    int bytes = UNZ_OK;
    do
    {
        int bytes_left = file_info.compressed_size - (buffer_current - buffer);
        n_assert(bytes_left >= 0);
        bytes = unzReadCurrentFile(file, buffer_current, bytes_left);
        if(bytes < 0) n_assert(false);

        buffer_current += bytes;
    }
    while(bytes > 0);


    if(UNZ_OK != unzCloseCurrentFile(file)) n_assert(false);

    if(UNZ_OK != unzClose(file)) n_assert(false);

    free(buffer);
    buffer = NULL;
*/

    return 0;
}

#if 0
//------------------------------------------------------------------------------
/**
*/
void
__cdecl main()
{
    App::ZipTestApplication app;
    app.SetCompanyName("Radon Labs GmbH");
    app.SetAppID("ZipTest");
    if (app.Open())
    {
        app.Run();
        app.Close();
    }
    app.Exit();
}
#endif
