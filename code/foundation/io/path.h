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

    // TODO: switch from using a string for the type, and use an enum
    enum FileTypes
    {
        Texture,
        Model,
        Material,
        Mesh
    };

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
    const Util::String AsString() const;

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
    /// Convert to a URI seated in the provided folder
    IO::URI WorkURI(const Util::String& workFolder) const;
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

    /// Get file with work extension
    Util::String GetWorkFile() const;
    /// Get file with export extension
    Util::String GetExportFile() const;

    /// Add an export binding using namespace -> file extension to use with ExportURI. Adding the same binding again is allowed, binding a namespace to another extension is not
    static void AddExportMapping(const Util::StringAtom& ns, const Util::StringAtom& extension);
    /// Add a work binding using namespace -> file extension to use with WorkURI. Adding the same binding again is allowed, binding a namespace to another extension is not
    static void AddWorkMapping(const Util::StringAtom& ns, const Util::StringAtom& extension);
    /// Set the work folder
    static void SetWorkRoot(const Util::StringAtom& root);

    static Util::Dictionary<Util::StringAtom, Util::StringAtom> ExportExtensions, WorkExtensions;
    static Util::StringAtom WorkRoot;

private:
    /// Build fileAndFolder
    void Build();

    bool isFile;
    Util::String folder;
    Util::String file;
    Util::String type;

    Util::String folderAndFile;
};


//------------------------------------------------------------------------------
/**
    Several systems read the same namespace, such as graphics and physics both reading meshes,
    and each registers the mappings it needs regardless of setup order. They have to agree though.
*/
inline void
Path::AddExportMapping(const Util::StringAtom& ns, const Util::StringAtom& extension)
{
    IndexT i = Path::ExportExtensions.FindIndex(ns);
    if (i != InvalidIndex)
    {
        n_assert_fmt(Path::ExportExtensions.ValueAtIndex(i) == extension,
            "Export namespace '%s' is already mapped to '%s', can't map it to '%s'",
            ns.Value(), Path::ExportExtensions.ValueAtIndex(i).Value(), extension.Value());
        return;
    }
    Path::ExportExtensions.Add(ns, extension);
}

//------------------------------------------------------------------------------
/**
    Several systems read the same namespace and each registers the mappings it needs regardless of
    setup order. They have to agree though.
*/
inline void
Path::AddWorkMapping(const Util::StringAtom& ns, const Util::StringAtom& extension)
{
    IndexT i = Path::WorkExtensions.FindIndex(ns);
    if (i != InvalidIndex)
    {
        n_assert_fmt(Path::WorkExtensions.ValueAtIndex(i) == extension,
            "Work namespace '%s' is already mapped to '%s', can't map it to '%s'",
            ns.Value(), Path::WorkExtensions.ValueAtIndex(i).Value(), extension.Value());
        return;
    }
    Path::WorkExtensions.Add(ns, extension);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Path::SetWorkRoot(const Util::StringAtom& root)
{
    Path::WorkRoot = root;
}

} // namespace IO

//------------------------------------------------------------------------------
/**
*/
IO::Path operator""_path(const char* c, std::size_t s);
