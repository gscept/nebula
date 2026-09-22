#pragma once
//------------------------------------------------------------------------------
/** 
    @class IO::Path
    
    A path is an abstraction around file paths. It encapsulates the folder path, an optional 
    file name and accompanying file type.

    A path lives entirely within Nebulas file system, meaning all paths are relative to content folders, 
    either in 'work' or 'export'.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "util/dictionary.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace IO
{

class Path
{
public:
    /// Constructor
    Path();
    /// Destructor
    ~Path();

    /// Copy constructor
    Path(const Path& path);

    /// Parse from serialized Path
    static Path Parse(const Util::String& serialized);
    /// Construct from folder
    static Path Folder(const Util::String& folder);
    /// Construct from file type, combined folder and file path
    static Path FolderAndFile(const Util::String& type, const Util::String& folderAndFile);
    /// Construct from file type, combined folder and file path
    static Path FolderAndFile(const char* type, const Util::String& folderAndFile);
    /// Construct from folder, file and type separately
    static Path File(const Util::String& folder, const Util::String& file, const Util::String& type);
    /// Construct from folder, file and type separately
    static Path File(const Util::String& folder, const Util::String& file, const char* type);

    /// Clear the path
    void Clear();
    /// Returns true if path is empty
    bool IsEmpty() const;
    /// Returns true if the path is a folder
    bool IsFolder() const;
    /// Returns true if the path is a file
    bool IsFile() const;

    /// Serialize to string
    const Util::String AsString();

    /// Comparison test
    bool operator==(const Path& path) const;
    /// Less test
    bool operator<(const Path& path) const;
    /// Greater test
    bool operator>(const Path& path) const;
    /// Append to folder path
    Path operator/(const char* folder) const;
    /// Append to folder path
    Path operator/(const Util::String& folder) const;
    /// Append to folder path
    Path operator/(const Path& folder) const;
    /// Convert to a URI seated in the provided folder
    IO::URI WorkURI(const char* workFolder) const;
    /// Convert to a folder in 'export'
    IO::URI ExportURI() const;

    /// Set the folder
    void SetFolder(const Util::String& folder);
    /// Clear the folder
    void ClearFolder();
    /// Set the file
    void SetFile(const Util::String& file, const Util::String& type);
    /// Clear the file
    void ClearFile();

    /// Get folder and file path
    const Util::String& GetFolderAndFile() const;
    /// Get file
    const Util::String& GetFile() const;
    /// Get folder
    const Util::String& GetFolder() const;
    /// Get type
    const Util::String& GetType() const;

private:
    /// Build fileAndFolder
    void Build();

    bool isFile;
    Util::String folder;
    Util::String file;
    Util::String type;

    Util::String folderAndFile;
};

} // namespace IO
