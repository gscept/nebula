//------------------------------------------------------------------------------
//  ziparchive.cc
//  (C) Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/zipfs/ziparchive.h"
#include "io/zipfs/zipfileentry.h"
#include "io/zipfs/zipdirentry.h"
#include "io/assignregistry.h"
#include "io/zipfs/ionebula3.h"

namespace IO
{
__ImplementClass(IO::ZipArchive, 'ZPAR', IO::ArchiveBase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ZipArchive::ZipArchive() :
    zipFileHandle(0)
{
    fill_nebula3_filefunc(&this->zlibIoFuncs);
}

//------------------------------------------------------------------------------
/**
*/
ZipArchive::~ZipArchive()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
    This opens the zip archive and reads the table of content as a 
    tree of ZipDirEntry and ZipFileEntry objects.
*/
bool
ZipArchive::Setup(const URI& zipFileURI, const String& rootPathOverride)
{
    n_assert(!this->IsValid());
    n_assert(0 == this->zipFileHandle);

    if (ArchiveBase::Setup(zipFileURI, rootPathOverride))
    {
        // extract the root location of the zip archive
        if (!rootPathOverride.IsEmpty())
        {
            this->rootPath = AssignRegistry::Instance()->ResolveAssigns(rootPathOverride).LocalPath() + "/";
        }
        else
        {
            this->rootPath = this->uri.LocalPath().ExtractDirName();
        }

        // open the zip file
        URI absPath = AssignRegistry::Instance()->ResolveAssigns(this->uri);
        String realPath = absPath.AsString();
        realPath.Append(".zip");
        this->zipFileHandle = unzOpen2_64(realPath.AsCharPtr(), &this->zlibIoFuncs);
        if (0 == this->zipFileHandle)
        {
            return false;
        }

        // read the table of contents
        this->ParseTableOfContents();    
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    This closes the zip archive, releasing the table of contents and
    closing the zip file.
*/
void
ZipArchive::Discard()
{
    n_assert(this->IsValid());

    unzClose(this->zipFileHandle);
    this->zipFileHandle = 0;

    ArchiveBase::Discard();
}

//------------------------------------------------------------------------------
/**
    Internal method which parses the table of contents of the into a tree
    of ZipDirEntry and ZipFileEntry objects.
*/
void
ZipArchive::ParseTableOfContents()
{
    n_assert(this->IsValid());

    // for each entry of the zip file...
    int walkRes = unzGoToFirstFile(this->zipFileHandle);
    if (UNZ_OK == walkRes) do
    {
        // get info about current file
        char curFileName[512];
        int fileInfoRes = unzGetCurrentFileInfo(this->zipFileHandle,
                                0,
                                curFileName,
                                sizeof(curFileName),
                                0, 0, 0, 0);
        n_assert(UNZ_OK == fileInfoRes);
        this->AddEntry(curFileName);
        walkRes = unzGoToNextFile(this->zipFileHandle);
    }
    while (UNZ_OK == walkRes);
    if (UNZ_END_OF_LIST_OF_FILE != walkRes)
    {
        n_error("ZipArchive: error in parsing zip file '%s'!\n", this->uri.AsString().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    This will create a new ZipFileEntry or ZipDirEntry object and sort it into 
    the entry tree. Missing ZipDirEntry objects in the path will be created as 
    needed.
*/
void
ZipArchive::AddEntry(const String& path)
{
    n_assert(path.IsValid());

    // first check if the path end with a slash, if
    // yes it's a directory
    bool isDirectory = false;
    char lastChar = path[path.Length() - 1];
    if ((lastChar == '/') || (lastChar == '\\'))
    {
        isDirectory = true;
    }

    // tokenize the path into directory and filename components
    Array<String> pathTokens;
    path.Tokenize("/\\", pathTokens);
    ZipDirEntry* dirEntry = &(this->rootEntry);
    if (pathTokens.Size() > 1)
    {
        // find directory, create missing directory entries on the way
        IndexT i;
        for (i = 0; i < (pathTokens.Size() - 1); i++)
        {
            StringAtom curToken = pathTokens[i];
            ZipDirEntry* childDirEntry = dirEntry->FindDirEntry(curToken);
            if (0 == childDirEntry)
            {
                // need to create new dir entry
                childDirEntry = dirEntry->AddDirEntry(curToken);
            }
            dirEntry = childDirEntry;
        }
    }

    // create final entry and add to last dir entry
    StringAtom finalName(pathTokens.Back());
    if (isDirectory)
    {
        dirEntry->AddDirEntry(finalName);
    }
    else
    {
        ZipFileEntry* finalFileEntry = dirEntry->AddFileEntry(finalName);
        finalFileEntry->Setup(finalName, this->zipFileHandle, &this->archiveCritSect);
    }
}

//------------------------------------------------------------------------------
/**
    Test if an absolute path points into the zip archive and
    return a locale path into the zip archive. This will not test, whether
    the file or directory inside the zip archive actually exists, only
    if the path points INTO the zip archive by checking against
    the location directory of the zip archive. 
*/
String
ZipArchive::ConvertToPathInArchive(const Util::String& absPath) const
{
    // test if the absolute path starts with our root path
    IndexT rootPathIndex = absPath.FindStringIndex(this->rootPath, 0);
    if (0 == rootPathIndex)
    {
        // strip the root path from the absolute path
        String localPath = absPath;
        localPath.SubstituteString(this->rootPath, "");
        return localPath;
    }
    // path doesn't point into this archive
    return "";
}

//------------------------------------------------------------------------------
/**
*/
const ZipFileEntry*
ZipArchive::FindFileEntry(const String& pathInZipArchive) const
{
    // convert to local path into zip archive and split into components,
    // fail if string doesn't point into archive
    Array<String> pathTokens;
    if (0 == pathInZipArchive.Tokenize("/\\", pathTokens))
    {
        return 0;
    }
        
    // walk directory entries
    const ZipDirEntry* dirEntry = &this->rootEntry;
    if (pathTokens.Size() > 1)
    {
        IndexT i;
        for (i = 0; i < (pathTokens.Size() - 1); i++)
        {
            ZipDirEntry* subDirEntry = dirEntry->FindDirEntry(pathTokens[i]);
            if (0 != subDirEntry)
            {
                dirEntry = subDirEntry;
            }
            else
            {
                // missing subdirectory
                return 0;
            }
        }
    }

    // find the final file entry
    const ZipFileEntry* fileEntry = dirEntry->FindFileEntry(pathTokens.Back());
    return fileEntry;
}

//------------------------------------------------------------------------------
/**
*/
ZipFileEntry*
ZipArchive::FindFileEntry(const String& pathInZipArchive)
{
    const ZipArchive *a = static_cast<const ZipArchive*>(this);
    return const_cast<ZipFileEntry*>(a->FindFileEntry(pathInZipArchive));
}

//------------------------------------------------------------------------------
/**
*/
const ZipDirEntry*
ZipArchive::FindDirEntry(const String& pathInZipArchive) const
{
    // convert to local path into zip archive and split into components,
    // fail if string doesn't point into archive
    Array<String> pathTokens;
    if (0 == pathInZipArchive.Tokenize("/\\", pathTokens))
    {
        return 0;
    }

    // walk directory entries
    const ZipDirEntry* dirEntry = &(this->rootEntry);
    IndexT i;
    for (i = 0; i < pathTokens.Size(); i++)
    {
        ZipDirEntry* subDirEntry = dirEntry->FindDirEntry(pathTokens[i]);
        if (0 != subDirEntry)
        {
            dirEntry = subDirEntry;
        }
        else
        {
            // missing subdirectory
            return 0;
        }
    }
    return dirEntry;
}

//------------------------------------------------------------------------------
/**
*/
Array<String>
ZipArchive::ListFiles(const String& dirPathInZipArchive, const String& pattern) const
{
    Array<String> result;
    const ZipDirEntry* dirEntry = this->FindDirEntry(dirPathInZipArchive);
    if (0 != dirEntry)
    {
        const Array<ZipFileEntry>& files = dirEntry->GetFileEntries();
        String fileName;
        IndexT i;
        for (i = 0; i < files.Size(); i++)
        {
            fileName = files[i].GetName().Value();
            if (String::MatchPattern(fileName, pattern))
            {
                result.Append(fileName);
            }
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<String>
ZipArchive::ListDirectories(const String& dirPathInZipArchive, const String& pattern) const
{
    Array<String> result;
    const ZipDirEntry* dirEntry = this->FindDirEntry(dirPathInZipArchive);
    if (0 != dirEntry)
    {
        const Array<ZipDirEntry>& subDirs = dirEntry->GetDirEntries();
        String subDirName;
        IndexT i;
        for (i = 0; i < subDirs.Size(); i++)
        {
            subDirName = subDirs[i].GetName().Value();
            if (String::MatchPattern(subDirName, pattern))
            {
                result.Append(subDirName);
            }
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
    This method takes a normal "file:" scheme URI and convertes it into
    a "zip:" scheme URI which points to the file in this zip archive. This
    is used by the IoServer for transparent file access into zip archives.
*/
URI
ZipArchive::ConvertToArchiveURI(const URI& fileURI) const
{
    n_assert(fileURI.LocalPath().IsValid());

    // localize path into archive, fail hard if URI doesn't point into archive
    String localPath = this->ConvertToPathInArchive(fileURI.LocalPath());
    if (!localPath.IsValid())
    {
        n_error("ZipArchive::ConvertToZipURI(): file '%s' doesn't point into this zip archive (%s)!\n",
            fileURI.AsString().AsCharPtr(), this->uri.AsString().AsCharPtr());
    }

    URI zipURI = this->uri;
    zipURI.SetScheme("zip");
    String query;
    query.Append("file=");
    query.Append(localPath);
    zipURI.SetQuery(query);
    return zipURI;
}

} // namespace IO
