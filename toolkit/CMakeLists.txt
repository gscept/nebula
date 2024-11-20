#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# old and mostly obsolete stuff
#add_subdirectory(archiver3)
#add_subdirectory(audiobatcher3)
#add_subdirectory(gamebatcher)
#add_subdirectory(modelbatcher3)
#add_subdirectory(fbxbatcher3)
#add_subdirectory(textool)
#add_subdirectory(levelviewer)
#add_subdirectory(texturebatcher3)
#add_subdirectory(shaderbatcher)
#add_subdirectory(shaderc)
#add_subdirectory(assetc)

fips_ide_group(Toolkit)
add_subdirectory(toolkit-common)
add_subdirectory(toolkitutil)
add_subdirectory(assetbatcher)

fips_ide_group(Editor)
add_subdirectory(editor)