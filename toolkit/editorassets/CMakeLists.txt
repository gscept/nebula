#-------------------------------------------------------------------------------
# editorassets
#
# The editor's asset side that knows no asset type: the type registry, the live batcher, and the
# panels built on them. A subsystem registers its own types and their editors, so this module stays
# the same whatever a game is made of.
#-------------------------------------------------------------------------------
nebula_begin_module(editorassets)
target_include_directories(editorassets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_precompile_headers(editorassets PRIVATE [["foundation/stdneb.h"]])
fips_ide_group(Editor)

fips_files(
    assettype.h
    assettype.cc
    assetdatabase.h
    assetdatabase.cc
    assetbrowser.h
    assetbrowser.cc
    assetbrowserpanel.h
    assetbrowserpanel.cc
    assetpicker.h
    assetpicker.cc
    assetdocument.h
    assetdocument.cc
    asseteditorpanel.h
    asseteditorpanel.cc
    assetfield.h
    assetfield.cc
    resourcefield.h
    resourcefield.cc
    importpanel.h
    importpanel.cc
    assetsfeatureunit.h
    assetsfeatureunit.cc
    livebatcher.h
    livebatcher.cc
    batcherpanel.h
    batcherpanel.cc
)

# dynui for the import's texture preview and the files dropped on the window
fips_deps(foundation resource render application imgui dynui neweditor toolkitutil)

nebula_end_module()
