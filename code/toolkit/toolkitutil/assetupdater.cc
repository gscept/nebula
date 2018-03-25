//------------------------------------------------------------------------------
//  assetupdater.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "assetupdater.h"
#include "io/ioserver.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
AssetUpdater::AssetUpdater()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetUpdater::Update(Logger& logger, const AssetRegistry& diffSet)
{
    logger.Print(">> Synchronizing local files...\n");

    IoServer* ioServer = IoServer::Instance();
    if (this->localRootPath.IsEmpty())
    {
        logger.Error("No local root path set!\n");
        return false;
    }
    if (this->remoteRootPath.IsEmpty())
    {
        logger.Error("No remote root path set!\n");
        return false;
    }
    SizeT numFilesCopied = 0;
    SizeT numFilesDeleted = 0;
    SizeT numFilesTouched = 0;
    SizeT numErrors = 0;

    // for each entry in the difference set...
    IndexT i;
    for (i = 0; i < diffSet.GetNumAssets(); i++)
    {
        const AssetFile& curEntry = diffSet.GetAssetByIndex(i);
        URI localFileUri(this->localRootPath + curEntry.GetPath().AsString());
        URI localDirUri(this->localRootPath + curEntry.GetPath().AsString().ExtractDirName());
        URI remoteFileUri(this->remoteRootPath + curEntry.GetPath().AsString());

        if ((curEntry.GetState() == AssetFile::New) ||
            (curEntry.GetState() == AssetFile::Changed))
        {
            // need to copy file from server
            logger.Print("COPY %s\n", curEntry.GetPath().Value());
            if (!ioServer->DirectoryExists(localDirUri))
            {
                ioServer->CreateDirectory(localDirUri);
            }
            if (ioServer->CopyFile(remoteFileUri, localFileUri))
            {
                // set the local file's time stamp to the remote file's time stamp
                ioServer->SetFileWriteTime(localFileUri, curEntry.GetTimeStamp());
                numFilesCopied++;
            }
            else
            {
                logger.Error("Failed to copy file '%s' from build server!\n", curEntry.GetPath().Value());
                numErrors++;
            }
        }
        else if (curEntry.GetState() == AssetFile::Touched)
        {
            // only update the local time stamp
            // logger.Print("TOUCH %s\n", curEntry.GetPath().Value().AsCharPtr());
            ioServer->SetFileWriteTime(localFileUri, curEntry.GetTimeStamp());
            numFilesTouched++;
        }
        else if (curEntry.GetState() == AssetFile::Deleted)
        {
            // need to delete local copy
            logger.Print("DELETE %s\n", curEntry.GetPath().Value());
            if (ioServer->DeleteFile(localFileUri))
            {
                numFilesDeleted++;
            }
            else
            {
                logger.Error("Failed to delete local file '%s'!\n", curEntry.GetPath().Value());
                numErrors++;
            }
        }
    }    

    // print summary:
    logger.Print("\nAFTER SYNC: %d copied, %d touched, %d deleted, %d errors.\n", numFilesCopied, numFilesTouched, numFilesDeleted, numErrors);
    return true;
}

} // namespace ToolkitUtil
