//------------------------------------------------------------------------------
//  assetregistry.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "assetregistry.h"
#include "io/ioserver.h"
#include "io/uri.h"
#include "io/stream.h"
#include "io/binarywriter.h"
#include "io/binaryreader.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
AssetRegistry::AssetRegistry() :
    numScannedFiles(0),
    numAddedFiles(0),
    numChangedFiles(0),
    numIdenticalFiles(0),
    numDeletedFiles(0),
    numTouchedFiles(0),
    numErrorFiles(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This method loads the registry file (if it exists), updates it by
    scanning the actual file system, and saves back the registry file.
*/
bool
AssetRegistry::UpdateLocal(Logger& logger)
{
    IoServer* ioServer = IoServer::Instance();
    this->updateLocalTimer.Start();

    // first try to load registry file (may not exist yet)
    if (!this->rootPath.IsValid())
    {
        logger.Error("No root path set!\n");
        return false;
    }
    if (!this->registryPath.IsValid())
    {
        logger.Error("No asset registry file set!\n");
        return false;
    }
    URI registryURI = this->rootPath + this->registryPath;
    if (ioServer->FileExists(registryURI))
    {
        logger.Print(">> Loading local asset registry file...\n");
        if (!this->LoadRegistryFile(logger, registryURI))
        {
            return false;
        }
    }
    logger.Print("%d files in registry file\n", this->registry.Size());

    // update registry by parsing asset directories
    this->numScannedFiles = 0;
    this->numAddedFiles = 0;
    this->numChangedFiles = 0;
    this->numDeletedFiles = 0;
    this->numIdenticalFiles = 0;
    this->numTouchedFiles = 0;
    this->numErrorFiles = 0;
    logger.Print(">> Scanning local asset files...\n");
    if (!this->UpdateRegistry(logger))
    {
        return false;
    }

    // remove files which no longer exist from the registry
    // (all files which are still in the Unknown state after
    // the local file scan)
    this->RemoveObsoleteFilesFromRegistry(logger);

    // save back the registry
    logger.Print(">> Save registry file...\n");
    if (!this->SaveRegistryFile(logger, registryURI))
    {
        return false;
    }
    this->updateLocalTimer.Stop();

    // print summary
    logger.Print("\nAFTER LOCAL SCAN: %d identical, %d new, %d changed, %d removed\n", this->numIdenticalFiles, this->numAddedFiles, this->numChangedFiles, this->numDeletedFiles);
    logger.Print("Local update time: %.3f seconds\n", this->updateLocalTimer.GetTime());

    return true;
}

//------------------------------------------------------------------------------
/**
    This method only loads the registry file (usually from the remote
    build server directory).
*/
bool
AssetRegistry::UpdateRemote(Logger& logger)
{
    IoServer* ioServer = IoServer::Instance();

    // first try to load registry file (may not exist yet)
    if (!this->rootPath.IsValid())
    {
        logger.Error("No root path set!\n");
        return false;
    }
    if (!this->registryPath.IsValid())
    {
        logger.Error("No asset registry file set!\n");
        return false;
    }
    URI registryURI = this->rootPath + this->registryPath;
    if (ioServer->FileExists(registryURI))
    {
        logger.Print(">> Loading remote asset registry file...\n");
        if (this->LoadRegistryFile(logger, registryURI))
        {
            return true;
        }
    }
    else
    {
        logger.Error("Failed to load remote registry file: %s!\n", registryURI.AsString().AsCharPtr());
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    This iterates through all asset directories and builds a registry
    with all files. If a file already exists in the registry it's file time
    is checked against the value in the registry, and if it is different
    the CRC checksum will be re-computed.
*/
bool
AssetRegistry::UpdateRegistry(Logger& logger)
{
    bool retval = true;
    IoServer* ioServer = IoServer::Instance();

    // for each asset directory
    IndexT i;
    for (i = 0; i < this->directories.Size(); i++)
    {
        if (ioServer->DirectoryExists(this->rootPath + this->directories[i]))
        {
            retval &= this->RecurseUpdateRegistry(logger, this->directories[i]);
        }
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
    Iterate over files in directory and add them to the registry (if 
    they don't exist yet), then recurse into sub directories.
*/
bool
AssetRegistry::RecurseUpdateRegistry(Logger& logger, const String& dir)
{
    IoServer* ioServer = IoServer::Instance();
    URI dirURI = this->rootPath + dir;
    if (!ioServer->DirectoryExists(dirURI))
    {
        logger.Error("Asset directory not found: '%s'!\n", dir.AsCharPtr());
        return false;
    }
    // logger.Print("> %s\n", dir.AsCharPtr());

    // iterate over files
    Array<String> files = ioServer->ListFiles(dirURI, "*");
    IndexT i;
    for (i = 0; i < files.Size(); i++)
    {
        this->numScannedFiles++;
        String filePath;
        filePath.Format("%s/%s", dir.AsCharPtr(), files[i].AsCharPtr());
        StringAtom filePathAtom(filePath);
        if (!this->registry.Contains(filePathAtom))
        {
            this->AddAssetFile(logger, filePathAtom);
        }
        else
        {
            this->UpdateAssetFile(logger, filePathAtom);
        }
    }

    // enter subdirectories
    Array<String> subDirs = ioServer->ListDirectories(dirURI, "*");
    for (i = 0; i < subDirs.Size(); i++)
    {
        if ((subDirs[i] != "CVS") && (subDirs[i] != ".svn"))
        {
            String subDirPath;
            subDirPath.Format("%s/%s", dir.AsCharPtr(), subDirs[i].AsCharPtr());
            this->RecurseUpdateRegistry(logger, subDirPath);
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    This method is called after the local asset files have been scanned
    and the registry has been updated. Any file in the registry which
    still has the Unknown state no longer exists in the file system, thus
    this method will remove it from the registry.
*/
void
AssetRegistry::RemoveObsoleteFilesFromRegistry(Logger& logger)
{
    IndexT i;
    for (i = this->registry.Size() - 1; i != InvalidIndex; i--)
    {
        if (this->registry.ValueAtIndex(i).GetState() == AssetFile::Unknown)
        {
            this->numDeletedFiles++;
            this->registry.EraseAtIndex(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Add a new file to the file registry. This will read the file modification
    time and compute a crc checksum from the file.
*/
void
AssetRegistry::AddAssetFile(Logger& logger, const StringAtom& filePath)
{
    String fullPath = this->rootPath + filePath.Value();

    IoServer* ioServer = IoServer::Instance();
    AssetFile assetFile;
    assetFile.SetPath(filePath);
    assetFile.SetTimeStamp(ioServer->GetFileWriteTime(fullPath));
    assetFile.SetChecksum(ioServer->ComputeFileCrc(fullPath));
    assetFile.SetState(AssetFile::New);
    this->registry.Add(filePath, assetFile);
    this->numAddedFiles++;
}

//------------------------------------------------------------------------------
/**
    Update an existing entry in the registry. This first checks whether the
    time stamp of the actual file is newer then the time stamp recorded in 
    the registry (it should never be older!), and if the actual time stamp
    is newer, will update the crc checksum.
*/
void
AssetRegistry::UpdateAssetFile(Logger& logger, const StringAtom& filePath)
{
    IoServer* ioServer = IoServer::Instance();
    AssetFile& assetFile = this->registry[filePath];
    n_assert(assetFile.GetPath() == filePath);
    
    String fullPath = this->rootPath + filePath.Value();

    FileTime fileTime = ioServer->GetFileWriteTime(fullPath);
    if (fileTime != assetFile.GetTimeStamp())
    {
        // need to update the crc checksum
        assetFile.SetChecksum(ioServer->ComputeFileCrc(fullPath));
        assetFile.SetTimeStamp(fileTime);
        assetFile.SetState(AssetFile::Changed);
        this->numChangedFiles++;
    }
    else
    {
        assetFile.SetState(AssetFile::Unchanged);
        this->numIdenticalFiles++;
    }
}

//------------------------------------------------------------------------------
/**
    Load the registry file.
*/
bool
AssetRegistry::LoadRegistryFile(Logger& logger, const URI& uri)
{
    n_assert(this->registry.IsEmpty());

    IoServer* ioServer = IoServer::Instance();
    Ptr<Stream> stream = ioServer->CreateStream(uri);
    Ptr<BinaryReader> reader = BinaryReader::Create();
    reader->SetMemoryMappingEnabled(true);
    reader->SetStream(stream);
    if (reader->Open())
    {
        // read header
        FourCC magic = reader->ReadUInt();
        if (magic != FourCC('NREG'))
        {
            logger.Error("Invalid registry file format (%s)\n", uri.AsString().AsCharPtr());
            return false;
        }
        SizeT numItems = reader->ReadUInt();
        if (numItems > 0)
        {
            this->registry.Reserve(numItems);
            this->registry.BeginBulkAdd();
            IndexT i;
            for (i = 0; i < numItems; i++)
            {
                FileTime timeStamp;
                AssetFile assetFile;
                assetFile.SetPath(reader->ReadString());		
                uint highbits = reader->ReadUInt();
				uint lowbits = reader->ReadUInt();
                timeStamp.SetBits(lowbits, highbits);
                assetFile.SetTimeStamp(timeStamp);
                assetFile.SetChecksum(reader->ReadUInt());
                this->registry.Add(assetFile.GetPath(), assetFile);
            }
            this->registry.EndBulkAdd();
        }
        reader->Close();
        return true;
    }
    else
    {
        logger.Error("Could not open registry file '%s' for reading!\n", uri.AsString().AsCharPtr());
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Public method to save the registry file back.
*/
bool
AssetRegistry::SaveRegistry(Logger& logger)
{
    URI registryURI = this->rootPath + this->registryPath;
    return this->SaveRegistryFile(logger, registryURI);
}

//------------------------------------------------------------------------------
/**
    Save the registry file (private).
*/
bool
AssetRegistry::SaveRegistryFile(Logger& logger, const URI& uri)
{
    IoServer* ioServer = IoServer::Instance();
    Ptr<Stream> stream = ioServer->CreateStream(uri);
    Ptr<BinaryWriter> writer = BinaryWriter::Create();
    writer->SetStream(stream);
    if (writer->Open())
    {
        writer->WriteUInt(FourCC('NREG').AsUInt());
        writer->WriteInt(this->registry.Size());
        IndexT i;
        for (i = 0; i < this->registry.Size(); i++)
        {
            const AssetFile& assetFile = this->registry.ValueAtIndex(i);
            writer->WriteString(assetFile.GetPath().Value());
            writer->WriteUInt(assetFile.GetTimeStamp().GetHighBits());
            writer->WriteUInt(assetFile.GetTimeStamp().GetLowBits());
            writer->WriteUInt(assetFile.GetChecksum());
        }
        writer->Close();
        return true;
    }
    else
    {
        logger.Error("Could not open registry file '%s' for writing!\n", uri.AsString().AsCharPtr());
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    This method takes a local and a remote asset registry (represents the
    asset files on a local client, and on a remote build server), and builds
    a difference map, which contains for each local asset file a state
    (New, Deleted, Changed). Identical files will not be stored in the
    registry.
*/
bool
AssetRegistry::BuildDifference(Logger& logger, AssetRegistry& localRegistry, const AssetRegistry& remoteRegistry)
{
    n_assert(this->registry.IsEmpty());
    logger.Print(">> Differencing local and remote assets...\n");

    // reset stats counters
    this->numScannedFiles = 0;
    this->numAddedFiles = 0;
    this->numChangedFiles = 0;
    this->numIdenticalFiles = 0;
    this->numDeletedFiles = 0;
    this->numTouchedFiles = 0;
    this->numErrorFiles = 0;

    // keep track of local registry states
    FixedArray<AssetFile::State> localStates(localRegistry.registry.Size(), AssetFile::Unknown);

    // for each file in the remote registry...
    this->registry.Reserve(remoteRegistry.registry.Size());
    this->registry.BeginBulkAdd();
    IndexT remoteIndex;
    for (remoteIndex = 0; remoteIndex < remoteRegistry.registry.Size(); remoteIndex++)
    {
        const AssetFile& remoteAsset = remoteRegistry.registry.ValueAtIndex(remoteIndex);
        AssetFile::State assetState = AssetFile::Unknown;

        // find the same asset in the local registry
        IndexT localIndex = localRegistry.registry.FindIndex(remoteAsset.GetPath());
        if (InvalidIndex == localIndex)
        {
            // a new file on the build server which doesn't exist yet locally
            assetState = AssetFile::New;
            this->numAddedFiles++;
        }
        else
        {
            // the file already exists locally, but we need to check whether 
            // the content has changed, or the build server version is actually
            // more recent then our own version (since copying is faster then
            // exporting)
            AssetFile& localAsset = localRegistry.registry.ValueAtIndex(localIndex);
            if (localAsset.GetChecksum() != remoteAsset.GetChecksum())
            {
                // the local file must be updated
                assetState = AssetFile::Changed;
                localAsset.SetTimeStamp(remoteAsset.GetTimeStamp());
                localAsset.SetChecksum(remoteAsset.GetChecksum());
                this->numChangedFiles++;
            }
            else if (localAsset.GetTimeStamp() < remoteAsset.GetTimeStamp())
            {
                // only the local time stamp must be updated
                assetState = AssetFile::Touched;
                localAsset.SetTimeStamp(remoteAsset.GetTimeStamp());
                this->numTouchedFiles++;
            }
            else
            {
                // the local file is uptodate
                assetState = AssetFile::Unchanged;
                this->numIdenticalFiles++;
            }
            // update the localStates array which keeps track of valid assets
            localStates[localIndex] = assetState;
        }

        // add a new item to our registry (only if the file is new or changed)
        if ((AssetFile::Changed == assetState) || (AssetFile::New == assetState) || (AssetFile::Touched == assetState))
        {
            AssetFile diffAsset;
            diffAsset.SetPath(remoteAsset.GetPath());
            diffAsset.SetTimeStamp(remoteAsset.GetTimeStamp());
            diffAsset.SetChecksum(remoteAsset.GetChecksum());
            diffAsset.SetState(assetState);
            this->registry.Add(diffAsset.GetPath(), diffAsset);
        }
    }

    // now we need to scan the localStates array for assets which don't exist anymore
    // in the remote registry
    IndexT i;
    for (i = 0; i < localStates.Size(); i++)
    {
        if (AssetFile::Unknown == localStates[i])
        {
            // this asset no longer exists, need to write a "Delete" item to our registry
            AssetFile deadAsset;
            deadAsset.SetPath(localRegistry.registry.ValueAtIndex(i).GetPath());
            deadAsset.SetState(AssetFile::Deleted);
            this->registry.Add(deadAsset.GetPath(), deadAsset);
            this->numDeletedFiles++;
        }
    }
    this->registry.EndBulkAdd();

    // print stats
    logger.Print("\nAFTER DIFFING: %d identical, %d touched, %d new, %d changed, %d removed\n", this->numIdenticalFiles, this->numTouchedFiles, this->numAddedFiles, this->numChangedFiles, this->numDeletedFiles);
    return true;
}

} // namespace ToolkitUtil
