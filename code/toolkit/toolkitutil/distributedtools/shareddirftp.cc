//------------------------------------------------------------------------------
//  shareddirftp.cc
//  (C) 2009 RadonLabs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shareddirftp.h"
#include "io/memorystream.h"
#include "io/ioserver.h"
#include "io/textwriter.h"
#include "io/textreader.h"
#include "toolkitutil/applauncher.h"
#include "io/console.h"
#include "timing/timer.h"

using namespace IO;
using namespace Util;
using namespace ToolkitUtil;

namespace DistributedTools
{
    __ImplementClass(DistributedTools::SharedDirFTP,'SDTP',DistributedTools::SharedDirControl)
//------------------------------------------------------------------------------
/**
    Constructor	
*/
SharedDirFTP::SharedDirFTP() :
    needVerboseCommand(false)
{
}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
SharedDirFTP::~SharedDirFTP()
{
}

//------------------------------------------------------------------------------
/**
    copy file to shared dir (src being absolute path, dst relative to control dir)
*/
void
SharedDirFTP::CopyFileToSharedDir(const Util::String & src, const Util::String & dst)
{
    // create the subdirectory if it does not exist
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    String dstPath;
    dstPath.Format("%s/%s",this->sharedDir.LocalPath().AsCharPtr(),dst.AsCharPtr());
    this->ScriptCreateSubDirectory(writer, dstPath);
    this->ScriptPutFile(writer, src);
    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    copy all files from src to shared dir (not recursive, only toplevel)
    (src being absolute path, dst relative to control dir)
*/
void
SharedDirFTP::CopyFilesToSharedDir(const Util::String & src, const Util::String & dst)
{
    // create the subdirectory if it does not exist
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    String dstPath;
    dstPath.Format("%s/%s",this->sharedDir.LocalPath().AsCharPtr(),dst.AsCharPtr());
    this->ScriptCreateSubDirectory(writer, dstPath);
    Array<String> files = IoServer::Instance()->ListFiles(URI(src),"*.*");
    String path;
    IndexT i;
    for(i = 0; i < files.Size(); i++)
    {
        path.Format("%s/%s",src.AsCharPtr(),files[i].AsCharPtr());
        this->ScriptPutFile(writer, path);	
    }
    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    copy everything (including sub dirs) from src into
    shared dirs guid folder (src being absolute path)
*/
void
SharedDirFTP::CopyDirectoryContentToSharedDir(const Util::String & src)
{
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    this->ScriptPutDirectoryContent(writer, src,this->sharedDir.LocalPath());
    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    copy file from shared dir to dst (dst being absolute, src relative to control dir)
*/
void
SharedDirFTP::CopyFileFromSharedDir(const Util::String & src, const Util::String & dst)
{
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    this->ScriptGetFiles(writer, src,dst);
    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    copy all files from shared dir sub dir to dst (not recursive, only toplevel)
    (dst being absolute, src relative to control dir)
*/
void
SharedDirFTP::CopyFilesFromSharedDir(const Util::String & src, const Util::String & dst)
{
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    this->ScriptGetFiles(writer, src,AssignRegistry::Instance()->ResolveAssignsInString(dst));
    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    copy everything (including sub dirs) from shared dirs guid folder
    into dst (dst being absolute path)
*/
void
SharedDirFTP::CopyDirectoryContentFromSharedDir(const Util::String & dst)
{
    // get hierarchy of shared dir
    FTPFile file = this->GetFtpDirectoryFileHierachy(this->sharedDir.LocalPath());
    
    // call get command for each file in given ftp hierarchy
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    
    this->ScriptCreateSubDirectory(writer,this->sharedDir.LocalPath());
    this->ScriptGetFileHierachy(writer, file,AssignRegistry::Instance()->ResolveAssignsInString(dst));
    
    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    removes specified file (relative to control dir)
*/
void
SharedDirFTP::RemoveFileInSharedDir(const Util::String & filepath)
{
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    
    writer->WriteFormatted("delete %s/%s\r\n",
        this->sharedDir.LocalPath().AsCharPtr(),
        AssignRegistry::Instance()->ResolveAssignsInString(filepath).AsCharPtr()
        );

    this->RunScript(this->CloseWriterAndGetStream(writer));
}
    
//------------------------------------------------------------------------------
/**
    removes specified directory if empty (relative to control dir)
*/
void
SharedDirFTP::RemoveDirectoryInSharedDir(const Util::String & dirpath)
{
    Ptr<TextWriter> writer = this->CreateScriptWriter();

    writer->WriteFormatted("rmdir %s/%s\r\n",
        this->sharedDir.LocalPath().AsCharPtr(),
        AssignRegistry::Instance()->ResolveAssignsInString(dirpath).AsCharPtr()
        );

    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    remove all files and directories of specified
    directory (relative to control dir) recursively 
*/
void
SharedDirFTP::RemoveDirectoryContent(const Util::String & dir)
{
    // get hierarchy of shared dir
    FTPFile file = this->GetFtpDirectoryFileHierachy(URI(dir).LocalPath());

    Ptr<TextWriter> writer = this->CreateScriptWriter();

    String dirPath = URI(dir).LocalPath();
    // trim unexpected slashes from the path
    dirPath.Trim("\\/");
    writer->WriteFormatted("cd %s\r\n",dirPath.AsCharPtr());
    this->ScriptRemoveFileHierachy(writer, file);
    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    checks if a subdirectory exists, which name equals the given guid
*/
bool
SharedDirFTP::ContainsGuidSubDir(const Util::Guid & guid)
{
    // get hierarchy of shared dir
    FTPFile file = this->GetFtpDirectoryFileHierachy(this->path.LocalPath(),false);
    
    IndexT idx;
    for(idx = 0; idx < file.content.Size(); idx++)
    {
    	if(file.content[idx].name == guid.AsString())
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    checks if directory is empty (relative to control dir)
*/
bool
SharedDirFTP::DirIsEmpty(const Util::String & dir)
{
    // get hierarchy of shared dir
    String path;
    path.Format("%s/%s",this->sharedDir.LocalPath().AsCharPtr(),dir.AsCharPtr());
    
    FTPFile file = this->GetFtpDirectoryFileHierachy(path);
    return file.content.Size() == 0;

}
    
//------------------------------------------------------------------------------
/**
    returns all subdirectories of specified directory (relative to control dir)
*/
Util::Array<Util::String>
SharedDirFTP::ListDirectories(const Util::String & dir)
{
    // get hierarchy of shared dir
    String path;
    path.Format("%s/%s",this->sharedDir.LocalPath().AsCharPtr(),dir.AsCharPtr());
    
    FTPFile file = this->GetFtpDirectoryFileHierachy(path);
    
    Array<String> res;

    IndexT idx;
    for(idx = 0; idx < file.content.Size(); idx++)
    {
        if(file.content[idx].type == Directory)
        {
            res.Append(file.content[idx].name);
        }
    }
    return res;
}

//------------------------------------------------------------------------------
/**
    returns all files inside the specified directory (relative to control dir)
*/
Util::Array<Util::String>
SharedDirFTP::ListFiles(const Util::String & dir)
{
    // get hierarchy of shared dir
    String path;
    path.Format("%s/%s",this->sharedDir.LocalPath().AsCharPtr(),dir.AsCharPtr());

    FTPFile file = this->GetFtpDirectoryFileHierachy(path);

    Array<String> res;

    IndexT idx;
    for(idx = 0; idx < file.content.Size(); idx++)
    {
        if(file.content[idx].type == File)
        {
            res.Append(file.content[idx].name);
        }
    }
    return res;
}

//------------------------------------------------------------------------------
/**
    creates control directory from guid
*/
void
SharedDirFTP::CreateControlDir()
{
    n_assert(this->sharedDir.IsValid());
    
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    this->ScriptCreateSubDirectory(writer, this->sharedDir.LocalPath());
    
    this->RunScript(this->CloseWriterAndGetStream(writer));
}
    
//------------------------------------------------------------------------------
/**
    removes control dir
*/
void
SharedDirFTP::RemoveControlDir()
{
    String rootPath = this->sharedDir.LocalPath().ExtractDirName();
    String guidDir = this->sharedDir.LocalPath().ExtractFileName();
    Ptr<TextWriter> writer = this->CreateScriptWriter();
    // handle the case, that the root path is a "/". In that case the "cd" command
    // would result in a wrong path on the server, because of "cd /" points to the servers
    // root directory
    if(rootPath == "/")
    {
        writer->WriteFormatted("rmdir %s\r\n",guidDir.AsCharPtr());
    }
    else
    {
        writer->WriteFormatted("cd %s\r\nrmdir %s\r\n",rootPath.AsCharPtr(),guidDir.AsCharPtr());
    }
    this->RunScript(this->CloseWriterAndGetStream(writer));
}

//------------------------------------------------------------------------------
/**
    Returns a file hierachy object that describes the structure below the
    given ftp directory
*/
SharedDirFTP::FTPFile
SharedDirFTP::GetFtpDirectoryFileHierachy(const Util::String & path, bool searchSubdirectories)
{
    // browse to dir and return listing
    bool receivedDir = false;
    FTPFile file;
    Timing::Timer timer;
    timer.Start();
    while(!receivedDir && timer.GetTime() < 90)
    {
        Ptr<TextWriter> dirListScript = this->CreateScriptWriter();
        this->ScriptCreateSubDirectory(dirListScript, path);
        dirListScript->WriteString("dir\r\n");

        Ptr<TextReader> reader = TextReader::Create();
        reader->SetStream(this->RunScript(this->CloseWriterAndGetStream(dirListScript)));
        
        file.name = path.ExtractFileName();
        file.type = Directory;
        if(reader->Open())
        {
            while(!reader->Eof())
            {
                String line = reader->ReadLine();
                if(line.FindStringIndex("d",0)==0)
                {
                    if(line.Length() > 57)
                    {
                        // directory name begins always at char 57
                        String dirName = line.ExtractToEnd(56);
                        // strip whitespaces
                        dirName.Trim(" \r\n");
                        if(searchSubdirectories)
                        {
                            String subPath;
                            subPath.Format("%s/%s",path.AsCharPtr(),dirName.AsCharPtr());
                            file.content.Append(this->GetFtpDirectoryFileHierachy(subPath));
                        }
                        else
                        {
                            FTPFile subFile;
                            subFile.type = Directory;
                            subFile.name = dirName;
                            file.content.Append(subFile);
                        }
                    }
                }
                else if(line.FindStringIndex("-",0)==0)
                {
                    // there can be only a filename, if the string is longer than 57 chars....
                    if(line.Length() > 57)
                    {
                        // ... because file name begins always at char 57
                        String fileName = line.ExtractToEnd(56);
                        // strip whitespaces
                        fileName.Trim(" \r\n");
                        FTPFile subFile;
                        subFile.type = File;
                        subFile.name = fileName;
                        file.content.Append(subFile);
                    }
                }
                else if(line.FindStringIndex("226",0)==0)
                {
                    receivedDir = true;
                }
            }
            reader->Close();
        }
        
        // if no "Directory Sent OK" received, retry in 100 milliseconds...
        if(!receivedDir)
        {
            Timing::Sleep(0.1);
        }
    }
    timer.Stop();
    
    return file;
}

//------------------------------------------------------------------------------
/**
    This will call a status script on the current ftp executable. Based on
    the output of the executable it can be verified, if a verbose command
    is required for full output. This can happen in some cases:

    "For whatever reason the ftp client is "Verbose mode on" by default
    when started from the command line but is in "Verbose mode
    off" by default when started with Shell.exec."
    
    http://www.tech-archive.net/Archive/InetSDK/microsoft.public.inetsdk.programming.scripting.vbscript/2004-03/0004.htm
*/
void
SharedDirFTP::CheckExecutableStatus()
{
    this->needVerboseCommand = false;
    
    Ptr<TextReader> reader = TextReader::Create();
    Ptr<TextWriter> dirListScript = this->CreateScriptWriter();
    dirListScript->WriteLine("status");
    reader->SetStream(this->RunScript(this->CloseWriterAndGetStream(dirListScript)));
    if(reader->Open())
    {
        while(!reader->Eof())
        {
            String line = reader->ReadLine();
            line.ToLower();
            IndexT vIdx = line.FindStringIndex("verbose:");
            if(vIdx != InvalidIndex && line.Length() > vIdx+13)
            {
                String value = line.ExtractRange(vIdx + 8,4);
                if(
                    value.FindStringIndex("off") != InvalidIndex ||
                    value.FindStringIndex("aus") != InvalidIndex
                    )
                {
                    this->needVerboseCommand = true;
                }
            }
        }
        reader->Close();
    }
}

//------------------------------------------------------------------------------
/**
	Creates an opened textwriter object to write scripts on it.
*/
Ptr<TextWriter>
SharedDirFTP::CreateScriptWriter()
{
    Ptr<TextWriter> writer = TextWriter::Create();
    Ptr<MemoryStream> stream = MemoryStream::Create();
    stream->SetAccessMode(Stream::ReadWriteAccess);
    writer->SetStream(stream.upcast<Stream>());
    if(!writer->Open())
    {
        Console::Instance()->Error("Could not open memory stream.");
    }
    return writer;
}

//------------------------------------------------------------------------------
/**
    Closes given textwriter object and returns the stream with the written
    script in it.
*/
const Ptr<Stream> &
SharedDirFTP::CloseWriterAndGetStream(const Ptr<IO::TextWriter> & writer)
{
    writer->Close();
    return writer->GetStream();
}

//------------------------------------------------------------------------------
/**
    writes a temporary ftp script file and run it.
    Returns the output stream of the ftp script including the commands.
*/
Ptr<Stream>
SharedDirFTP::RunScript(const Ptr<Stream> & script)
{
    Ptr<IoServer> ioServer = IoServer::Instance();
    
    // write script to a temporary textfile
    URI uri;
    Guid filenameGuid;
    filenameGuid.Generate();
    uri.Set("temp:distributedtools/ftpscript/");
    URI dirUri(uri);
    // create the script directory
    ioServer->CreateDirectory(dirUri);
    
    String scriptName;
    scriptName.Format("%s.ftp",filenameGuid.AsString().AsCharPtr());
    uri.AppendLocalPath(scriptName);
    Ptr<Stream> stream = ioServer->CreateStream(uri);
    Ptr<TextWriter> writer = TextWriter::Create();
    writer->SetStream(stream);

    Ptr<TextReader> reader = TextReader::Create();
    reader->SetStream(script);

    if(writer->Open() && reader->Open())
    {
        // write login part
        writer->WriteLine(this->username);
        writer->WriteLine(this->password);
        
        // enable verbose if needed
        if(this->needVerboseCommand)
        {
            writer->WriteLine("verbose");
        }

        // switching to binary mode
        writer->WriteLine("binary");
        
        // write script
        while(!reader->Eof())
        {
            writer->WriteLine(reader->ReadLine());
        }
        
        reader->Close();

        // write logout part
        writer->WriteLine("bye");
        
        writer->Close();
    }
    else
    {
        if(writer->IsOpen())
        {
            writer->Close();
        }
        if(reader->IsOpen())
        {
            reader->Close();
        }
    }
    writer = nullptr;
    reader = nullptr;
    // run an ftp application with the created script as parameter
    AppLauncher appLauncher;
    appLauncher.SetWorkingDirectory(this->exeLocation.LocalPath().ExtractDirName());
    appLauncher.SetExecutable(this->exeLocation.LocalPath());
    String args;
    
    // Extracting host from path...
    // Scheme://UserInfo@Host:Port/LocalPath#Fragment?Query
    args.Format("-i -w:12288 -s:%s %s",uri.LocalPath().AsCharPtr(),this->path.Host().AsCharPtr());
    
    appLauncher.SetArguments(args);
    Ptr<MemoryStream> output = MemoryStream::Create();
    output->SetAccessMode(Stream::ReadWriteAccess);
    appLauncher.SetStdoutCaptureStream(output.upcast<Stream>());
    // wait until script file is found... wait 90 seconds
    Timing::Timer timer;
    timer.Start();
    while(!ioServer->FileExists(uri) && timer.GetTime() < 90)
    {
        Timing::Sleep(0.1);
    }
    timer.Stop();
    if(ioServer->FileExists(uri))
    {
        appLauncher.LaunchWait();
    }
    else
    {
        Console::Instance()->Error("Could not find ftp script:\n%s",uri.LocalPath().AsCharPtr());
    }
    
    // delete the script file
    timer.Reset();
    timer.Start();
    while(ioServer->FileExists(uri) && timer.GetTime() < 90)
    {
        // try to delete the file within 90 seconds.
        if(!ioServer->DeleteFile(uri))
        {
            Timing::Sleep(0.1);
        }
    }
    timer.Stop();
    if(ioServer->FileExists(uri))
    {
        Console::Instance()->Warning("Could not delete script file from Temp directory:\n%s",uri.LocalPath().AsCharPtr());
    }

    // ... and the script directory if possible
    if(ioServer->DirectoryExists(dirUri))
    {
        ioServer->DeleteDirectory(dirUri);
    }

    return output.upcast<Stream>();
}

//------------------------------------------------------------------------------
/**
	Creates a script, that creates a directory hierachy on the current server location
*/
void
SharedDirFTP::ScriptCreateSubDirectory(const Ptr<IO::TextWriter> & writer, const Util::String & path)
{
    String localPath(path);
    localPath.ConvertBackslashes();
    Array<String> directories = localPath.Tokenize("/");

    String tmps;
    IndexT i;
    for(i = 0; i < directories.Size(); i++)
    {
        writer->WriteFormatted("mkdir %s\r\ncd %s\r\n",directories[i].AsCharPtr(),directories[i].AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Creates a Script, that puts a file to the current server location
*/
void
SharedDirFTP::ScriptPutFile(const Ptr<IO::TextWriter> & writer, const Util::String &src)
{
    writer->WriteFormatted("put \"%s\" \"%s\"\r\n",URI(src).LocalPath().AsCharPtr(),src.ExtractFileName().AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Creates a Script, that puts a list of files to the current server location
*/
void
SharedDirFTP::ScriptPutFiles(const Ptr<IO::TextWriter> & writer, const Array<String> &srcArray)
{
    IndexT i;
    String putCmd;
    for(i = 0; i < srcArray.Size(); i++)
    {
    	this->ScriptPutFile(writer, srcArray[i]);
    }
}

//------------------------------------------------------------------------------
/**
    Creates a script, that puts the whole directory content to the ftp location,
    including subdirectories.
*/
void
SharedDirFTP::ScriptPutDirectoryContent(const Ptr<IO::TextWriter> & writer, const Util::String & src,const Util::String & dst)
{
    this->ScriptCreateSubDirectory(writer,dst);

    // list files
    Array<String> files = IoServer::Instance()->ListFiles(URI(src),"*.*");
    String filePath;
    Array<String> filePaths;
    IndexT i;
    for(i = 0; i < files.Size(); i++)
    {
    	filePath.Format("%s/%s",src.AsCharPtr(),files[i].AsCharPtr());
        filePaths.Append(filePath);
    }
    this->ScriptPutFiles(writer,filePaths);

    // list subdirectories
    Array<String> dirs = IoServer::Instance()->ListDirectories(URI(src),"*.*");
    for(i = 0; i < dirs.Size(); i++)
    {
        filePath.Format("%s/%s",src.AsCharPtr(),dirs[i].AsCharPtr());
        this->ScriptPutDirectoryContent(writer,filePath,dirs[i]);
        writer->WriteString("cd ..\r\n");
    }
}

//------------------------------------------------------------------------------
/**
	Script that gets a file in the given ftp location
    and copies it to the given destination path
*/
void
SharedDirFTP::ScriptGetFile(const Ptr<IO::TextWriter> & writer, const Util::String &src, const Util::String &dst)
{
    this->ScriptCreateSubDirectory(writer, src);
    writer->WriteFormatted("\r\nlcd %s\r\nget %s\r\n",
        URI(dst).LocalPath().AsCharPtr(),
        src.ExtractFileName().AsCharPtr()
        );
}

//------------------------------------------------------------------------------
/**
    Script that gets all files in the given ftp location
    and copies the content to the given destination path.
*/
void
SharedDirFTP::ScriptGetFiles(const Ptr<IO::TextWriter> & writer, const Util::String & src,const Util::String & dst)
{
    this->ScriptCreateSubDirectory(writer, src);
    writer->WriteFormatted("\r\nlcd %s\r\nmget *?\r\n",
        URI(dst).LocalPath().AsCharPtr()
        );
}

//------------------------------------------------------------------------------
/**
    Script that copies files from a given ftp structure to a local directory.
    Creates missing directories.
*/
void
SharedDirFTP::ScriptGetFileHierachy(const Ptr<IO::TextWriter> & writer, const DistributedTools::SharedDirFTP::FTPFile &src, const Util::String &dst)
{
    // create directory if it doe snot exist
    if(IoServer::Instance()->CreateDirectory(URI(dst)))
    {
        // move to local dir
        writer->WriteString("lcd \"");
        writer->WriteString(dst);
        writer->WriteString("\"\r\n");

        // add files
        IndexT idx;
        for(idx = 0; idx < src.content.Size(); idx++)
        {
            if(src.content[idx].type == File)
            {
                writer->WriteString("get \"");
                writer->WriteString(src.content[idx].name);
                writer->WriteString("\"\r\n");
            }
        }

        // check sub directories
        for(idx = 0; idx < src.content.Size(); idx++)
        {
            if(src.content[idx].type == Directory)
            {
                // go one step deeper
                writer->WriteFormatted("cd %s\r\n",src.content[idx].name.AsCharPtr());

                String dirName;
                dirName.Format("%s/%s",dst.AsCharPtr(),src.content[idx].name.AsCharPtr());
                this->ScriptGetFileHierachy(writer,src.content[idx],dirName);
                
                // return to parent dir
                writer->WriteString("cd ..\r\n");
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Script that removes all files and directories from the ftp server
    that are in the given file hierarchy
*/
void
SharedDirFTP::ScriptRemoveFileHierachy(const Ptr<IO::TextWriter> & writer, const FTPFile & src)
{
    // remove files
    IndexT idx;
    for(idx = 0; idx < src.content.Size(); idx++)
    {
        if(src.content[idx].type == File)
        {
            writer->WriteString("delete \"");
            writer->WriteString(src.content[idx].name);
            writer->WriteString("\"\r\n");
        }
    }

    // remove directories
    for(idx = 0; idx < src.content.Size(); idx++)
    {
        if(src.content[idx].type == Directory)
        {
            writer->WriteFormatted("cd %s\r\n",src.content[idx].name.AsCharPtr());
            this->ScriptRemoveFileHierachy(writer, src.content[idx]);
            writer->WriteFormatted("\r\ncd ..\r\nrmdir %s\r\n",src.content[idx].name.AsCharPtr());
        }
    }
}

} // namespace DistributedTools
