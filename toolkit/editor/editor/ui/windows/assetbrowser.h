#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::AssetBrowser

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "io/filetime.h"
#include "util/array.h"
#include "util/string.h"
#include "io/uri.h"

namespace Presentation
{

class ScanFolderJob;
class AssetEditor;
class AssetBrowser : public BaseWindow
{
    __DeclareClass(AssetBrowser)
public:
    AssetBrowser();
    ~AssetBrowser();

    void Update();
    void Run(SaveMode save) override;
private:

    struct FileEntry
    {
        enum class Type
        {
            FBX,
            GLTF,
            Model,
            Mesh,
            Texture,
            Surface,
            Audio,
            Text,
            Skeleton,
            Animation,
            Frame,
            Shader,
            Physics,
            NavMesh,
            Other
        };
        Util::String name;
        Util::String extension;
        IO::URI path;
        IO::Stream::Size size;
        IO::FileTime modifiedTime;
        Type type;
        uint folder;
    };
    struct FileTreeNode
    {
        Util::String name;
        IO::URI path;
        Util::Array<uint> children;
        Util::Array<uint> files;
        uint hash;
        uint parentHash;
    };
    struct FileTree
    {
        IO::URI path;
        uint rootHash;
    };
    
    Util::Dictionary<Util::String, FileTree> fileTrees;

    Util::Dictionary<uint, FileTreeNode> nodes;
    Util::Dictionary<uint, FileEntry> files;
    void ScanFolder(const Util::String & treeName, const IO::URI& folderPath, bool useArchive);
    void DisplayFileTree();

private:
    static const uint MaterialHash = "sur"_hash;
    static const uint ModelHash = "n3"_hash;
    static const uint MeshHash = "nvx"_hash;
    static const uint SkeletonHash = "nsk"_hash;
    static const uint TextureHash = "dds"_hash;

    enum class FileViewMode
    {
        List,
        Details,
        Icons
    };
        
    uint activeFolder = -1;
    uint activeFile = -1;
    Util::String activeFileTree = "export";

    FileViewMode fileViewMode = FileViewMode::Details;
    ///
    void DisplayFileTreeFolderHierarchy(const FileTreeNode& node, int depth);
    ///
    void DisplaySelectedFolder(const Util::String& filter);
    /// Determine file type from file extension
    static FileEntry::Type DetermineFileType(const Util::String& extension);
    /// Recursively scan a directory and populate the FileTreeNode
    void ScanFolderRecursive(const IO::IoServer* ioServer, const IO::URI& folderPath, uint nodeHash, bool useArchive);
    friend class ScanFolderJob;
    Ptr<ScanFolderJob> currentScanJob;
};
__RegisterClass(AssetBrowser)

} // namespace Presentation

