#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#add_subdirectory(archiver3)
#add_subdirectory(audiobatcher3)
#add_subdirectory(gamebatcher)
#add_subdirectory(modelbatcher3)
#add_subdirectory(fbxbatcher3)
#add_subdirectory(textool)
#add_subdirectory(levelviewer)
#add_subdirectory(texturebatcher3)
#add_subdirectory(raytrace)
#add_subdirectory(shaderbatcher)

fips_ide_group(toolkit)
add_subdirectory(toolkit-common)

# these need fbx
IF (NOT N_MINIMAL_TOOLKIT)
	#add_subdirectory(shaderc)
	add_subdirectory(toolkitutil)
	add_subdirectory(assetbatcher)
	add_subdirectory(assetc)
ENDIF()

fips_ide_group(Editor)
add_subdirectory(editor)