//------------------------------------------------------------------------------
//  pathtest.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "pathtest.h"
#include "io/path.h"

namespace Test
{
__ImplementClass(Test::PathTest, 'PATT', Test::TestCase);

using namespace IO;

//------------------------------------------------------------------------------
/**
*/
void
PathTest::Run()
{
    // a URN splits into folder, file and type, and serializes back to the same string
    Path mesh = Path::Parse("system:box:msh");
    VERIFY(mesh.IsFile());
    VERIFY(mesh.GetFolder() == "system");
    VERIFY(mesh.GetFile() == "box");
    VERIFY(mesh.GetType() == "msh");
    VERIFY(mesh.GetFolderAndFile() == "system/box");
    VERIFY(mesh.AsString() == "system:box:msh");

    // files in the root folder have no folder
    Path root = Path::Parse(":box:msh");
    VERIFY(root.IsFile());
    VERIFY(root.GetFolderAndFile() == "box");

    // an export URI is not a URN
    VERIFY(!Path::Parse("msh:system/box.nvx").IsFile());
    VERIFY(!Path::Parse("").IsFile());

    // several systems may register the same export mapping, which is only stored once
    SizeT const numExportMappings = Path::ExportExtensions.Size();
    Path::AddExportMapping("pathtest", "ext");
    Path::AddExportMapping("pathtest", "ext");
    VERIFY(Path::ExportExtensions.Size() == numExportMappings + 1);

    Path test = Path::File("folder", "file", "pathtest");
    VERIFY(test.GetExportFile() == "folder/file.ext");

    // the same holds for work mappings
    SizeT const numWorkMappings = Path::WorkExtensions.Size();
    Path::AddWorkMapping("pathtest", "work");
    Path::AddWorkMapping("pathtest", "work");
    VERIFY(Path::WorkExtensions.Size() == numWorkMappings + 1);
    VERIFY(test.GetWorkFile() == "file.work");
}

} // namespace Test
