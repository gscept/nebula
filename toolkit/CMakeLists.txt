#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------

fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#fips_add_subdirectory(archiver3)
#fips_add_subdirectory(audiobatcher3)
#fips_add_subdirectory(gamebatcher)
#fips_add_subdirectory(modelbatcher3)
#fips_add_subdirectory(fbxbatcher3)
#fips_add_subdirectory(textool)
#fips_add_subdirectory(levelviewer)
#fips_add_subdirectory(texturebatcher3)
#fips_add_subdirectory(raytrace)
#fips_add_subdirectory(shaderbatcher)

fips_ide_group(toolkit)
fips_add_subdirectory(toolkit-common)

# these need fbx
IF (NOT N_MINIMAL_TOOLKIT)
	#fips_add_subdirectory(shaderc)
	fips_add_subdirectory(toolkitutil)
	fips_add_subdirectory(assetbatcher)
	fips_add_subdirectory(assetc)
ENDIF()

fips_ide_group(Editor)
fips_add_subdirectory(editor)