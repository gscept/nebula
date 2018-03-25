#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------

fips_ide_group(toolkit)
fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation)

    IF(NOT N_BUILD_FIRSTTIME)
    fips_add_subdirectory(archiver3)
    fips_add_subdirectory(audiobatcher3)
    fips_add_subdirectory(gamebatcher)
    fips_add_subdirectory(modelbatcher3)
    fips_add_subdirectory(fbxbatcher3)
    #fips_add_subdirectory(levelviewer)
    fips_add_subdirectory(texturebatcher3)
    fips_add_subdirectory(assetbatcher)
    #fips_add_subdirectory(raytrace)
    #add_subdirectory(toolkitutil)
    #fips_add_subdirectory(editorfeatures)
    IF(MSVC)
    #fips_add_subdirectory(setup)
    ENDIF()
    fips_add_subdirectory(shaderbatcher)
    ENDIF()

    fips_add_subdirectory(toolkitutil)
    fips_add_subdirectory(idldocument)
    fips_add_subdirectory(idlcompiler)
