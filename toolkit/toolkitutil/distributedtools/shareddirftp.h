#pragma once
//------------------------------------------------------------------------------
/**
    @class DistributedTools::SharedDirFTP

    A helper class that encapsulates all administrational activities for
    the shared directory, used by slave applications in distributed build.
    
    This is a subclass of SharedDirControl. These operations are only
    applicable for a shared directory accessed by the File Transfer Protocol.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "distributedtools/shareddircontrol.h"
#include "util/guid.h"
#include "io/uri.h"
#include "io/textwriter.h"
#include "io/textreader.h"

//------------------------------------------------------------------------------
namespace DistributedTools
{
class SharedDirFTP : public DistributedTools::SharedDirControl
{
__DeclareClass(DistributedTools::SharedDirFTP)
public:  
    /// constructor
    SharedDirFTP();
    /// destructor
    ~SharedDirFTP();

    /// copy a single file to shared dir (src being absolute path, dst relative to control dir)
    void CopyFileToSharedDir(const Util::String & src, const Util::String & dst);
    /// copy all files from src to shared dir (src being absolute path, dst relative to control dir)
    void CopyFilesToSharedDir(const Util::String & src, const Util::String & dst);
    /// copy everything from src to shared dir (src being absolute path)
    void CopyDirectoryContentToSharedDir(const Util::String & src);

    /// copy a single file from shared dir to dst (dst being absolute, src relative to control dir)
    void CopyFileFromSharedDir(const Util::String & src, const Util::String & dst);
    /// copy all files from shared dir sub dir to dst (dst being absolute, src relative to control dir)
    void CopyFilesFromSharedDir(const Util::String & src, const Util::String & dst);
    /// copy everything from shared dir sub dir to dst (dst being absolute)
    void CopyDirectoryContentFromSharedDir(const Util::String & dst);

    /// removes specified file (relative to control dir)
    void RemoveFileInSharedDir(const Util::String & filepath);
    /// removes specified directory if empty (relative to control dir)
    void RemoveDirectoryInSharedDir(const Util::String & dirpath);
    /// removes all content inside of given directory (relative to control dir)
    void RemoveDirectoryContent(const Util::String & dir);

    /// checks if a subdirectory exists, which name equals the given guid
    bool ContainsGuidSubDir(const Util::Guid & guid);
    /// checks if directory is empty (relative to control dir)
    bool DirIsEmpty(const Util::String & dir);
    /// returns all subdirectories of specified directory (relative to control dir)
    Util::Array<Util::String> ListDirectories(const Util::String & dir);
    /// returns all files inside the specified directory (relative to control dir)
    Util::Array<Util::String> ListFiles(const Util::String & dir);

    void SetUserName(const Util::String & name);
    void SetPassword(const Util::String & pass);
    void SetExeLocation(const IO::URI & location);
    
private:
    
    /// type of an ftp file structure
    enum FTPFileType
    {
        File,
        Directory
    };
    /// a simple structure to create a file hierarchy
    struct FTPFile
    {
        FTPFileType type;
        Util::String name;
        Util::Array<FTPFile> content;
    };

    /// creates control directory from guid
    void CreateControlDir();
    /// removes control dir
    void RemoveControlDir();

    /// returns an object that describes the file system below the given ftp directory
    FTPFile GetFtpDirectoryFileHierachy(const Util::String & path, bool searchSubdirectories = true);
    /// Checks status of current ftp executable and verifies if a verbose command is needed
    void CheckExecutableStatus();

    /// creates a textwriter object for writing scripts
    Ptr<IO::TextWriter> CreateScriptWriter();
    /// closes given textwriter and returns the stream with the written script in it
    const Ptr<IO::Stream> & CloseWriterAndGetStream(const Ptr<IO::TextWriter> & writer);
    /// creates a ftp script file and run it, after this delete that file. Returns output stream.
    Ptr<IO::Stream> RunScript(const Ptr<IO::Stream> & script);

    /// script: creates a directory below the shared dir, creates all missing subdirectories
    void ScriptCreateSubDirectory(const Ptr<IO::TextWriter> & writer, const Util::String & path);
    /// script: put a file to the current ftp location
    void ScriptPutFile(const Ptr<IO::TextWriter> & writer, const Util::String & src);
    /// script: put list of files to the current ftp location
    void ScriptPutFiles(const Ptr<IO::TextWriter> & writer, const Util::Array<Util::String> & srcArray);
    /// script: put directory content to the current ftp location. Copy subdirectories content as well.
    void ScriptPutDirectoryContent(const Ptr<IO::TextWriter> & writer, const Util::String & src,const Util::String & dst);
    /// script: get file on the ftp server and copy it to the dst dir.
    void ScriptGetFile(const Ptr<IO::TextWriter> & writer, const Util::String & src,const Util::String & dst);
    /// script: get directory content on the ftp server and copy it to the dst dir.
    void ScriptGetFiles(const Ptr<IO::TextWriter> & writer, const Util::String & src,const Util::String & dst);
    /// script: get all files in the given file hierachy, creates local directories
    void ScriptGetFileHierachy(const Ptr<IO::TextWriter> & writer, const FTPFile & src,const Util::String & dst);
    /// script: removes all files in the given remote file hierachy
    void ScriptRemoveFileHierachy(const Ptr<IO::TextWriter> & writer, const FTPFile & src);

    Util::String username;
    Util::String password;
    IO::URI exeLocation;
    bool needVerboseCommand;
};

//------------------------------------------------------------------------------
/**
    set username
*/
inline
void
SharedDirFTP::SetUserName(const Util::String & name)
{
    this->username = name;
}
//------------------------------------------------------------------------------
/**
    set password
*/
inline
void
SharedDirFTP::SetPassword(const Util::String & pass)
{
    this->password = pass;
}
//------------------------------------------------------------------------------
/**
    set location of ftp.exe
*/
inline
void
SharedDirFTP::SetExeLocation(const IO::URI & location)
{
    this->exeLocation = location;
    this->CheckExecutableStatus();
}

} // namespace DistributedTools
